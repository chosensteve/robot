#define S0    0
#define S1    1
#define S2    2
#define S3    3
#define EN    13
#define Z_OP  14

#define SIDE_LEFT   4
#define SIDE_RIGHT  5
bool useSideSensors = false;

#define ENC_LA  6
#define ENC_LB  7
#define ENC_RA  8
#define ENC_RB  9

#define PWMA  11
#define AIN1  24
#define AIN2  25

#define PWMB  10
#define BIN1  26
#define BIN2  27

float Kp_learn      = 5.0;
float Ki_learn      = 0.0;
float Kd_learn      = 2.5;
int   learnSpeed    = 120;

float Kp_replay     = 3.5;
float Ki_replay     = 0.0;
float Kd_replay     = 2.0;
int   replayFast    = 220;
int   replayMedium  = 160;
int   replaySlow    = 100;

int   maxSpeed      = 255;
int   deadband      = 5;
float integralMax   = 5000.0;
float smoothAlpha   = 0.3;

struct PathSegment {
  int16_t leftTicks;
  int16_t rightTicks;
  int16_t avgError;
};

#define MAX_SEGMENTS  4000
PathSegment path[MAX_SEGMENTS];
int pathLength = 0;

#define SEGMENT_TICKS  50
#define LOOKAHEAD      8

volatile long encLeftCount  = 0;
volatile long encRightCount = 0;

void encLeftISR() {
  if (digitalReadFast(ENC_LB)) encLeftCount++;
  else                          encLeftCount--;
}

void encRightISR() {
  if (digitalReadFast(ENC_RB)) encRightCount++;
  else                          encRightCount--;
}

float integral   = 0;
float lastError  = 0;
float smoothPos  = 750.0;

int  calMin[16];
int  calMax[16];
bool calibrated = false;

enum Mode { IDLE, LEARN, REPLAY };
Mode currentMode = IDLE;

long segLeftStart   = 0;
long segRightStart  = 0;
long segErrorSum    = 0;
int  segSampleCount = 0;

int  replayIndex      = 0;
long replayLeftStart  = 0;
long replayRightStart = 0;

void motorLeft(int spd) {
  if (spd >= 0) {
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, HIGH);
    analogWrite(PWMA, constrain(spd, 0, maxSpeed));
  } else {
    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);
    analogWrite(PWMA, constrain(-spd, 0, maxSpeed));
  }
}

void motorRight(int spd) {
  if (spd >= 0) {
    digitalWrite(BIN1, LOW);
    digitalWrite(BIN2, HIGH);
    analogWrite(PWMB, constrain(spd, 0, maxSpeed));
  } else {
    digitalWrite(BIN1, HIGH);
    digitalWrite(BIN2, LOW);
    analogWrite(PWMB, constrain(-spd, 0, maxSpeed));
  }
}

void stopMotors() {
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
}

int readMux(int channel) {
  digitalWrite(S0, channel & 1);
  digitalWrite(S1, (channel >> 1) & 1);
  digitalWrite(S2, (channel >> 2) & 1);
  digitalWrite(S3, (channel >> 3) & 1);
  delayMicroseconds(15);
  return analogRead(Z_OP);
}

bool readSideLeft()  { return digitalRead(SIDE_LEFT)  == LOW; }
bool readSideRight() { return digitalRead(SIDE_RIGHT) == LOW; }

void calibrate() {
  for (int i = 0; i < 16; i++) {
    int v = readMux(i);
    calMin[i] = v;
    calMax[i] = v;
  }

  unsigned long start = millis();
  while (millis() - start < 3000) {
    for (int i = 0; i < 16; i++) {
      int v = readMux(i);
      if (v < calMin[i]) calMin[i] = v;
      if (v > calMax[i]) calMax[i] = v;
    }
  }

  calibrated = true;
}

int getPosition() {
  long weightedSum = 0;
  long activeCount = 0;

  for (int i = 0; i < 16; i++) {
    int raw = readMux(i);
    int val;

    if (calibrated) {
      int range = calMax[i] - calMin[i];
      if (range < 50) range = 50;
      val = (long)(raw - calMin[i]) * 1000 / range;
      val = constrain(val, 0, 1000);
    } else {
      val = (raw > 1500) ? 1000 : 0;
    }

    if (val > 300) {
      weightedSum += (long)i * 100;
      activeCount++;
    }
  }

  if (activeCount == 0) return -1;
  return weightedSum / activeCount;
}

float computePID(int pos, float kp, float ki, float kd) {
  smoothPos = smoothPos * (1.0 - smoothAlpha) + pos * smoothAlpha;
  int error = 750 - (int)smoothPos;
  if (abs(error) < deadband) error = 0;

  float P = error;
  integral += error;
  integral = constrain(integral, -integralMax, integralMax);
  float D = error - lastError;
  lastError = error;

  return (kp * P) + (ki * integral) + (kd * D);
}

void resetPID() {
  integral  = 0;
  lastError = 0;
  smoothPos = 750.0;
}

int classifyCurvature(int16_t avgError) {
  int absErr = abs(avgError);
  if (absErr < 40)  return 0;
  if (absErr < 150) return 1;
  return 2;
}

int lookaheadCurvature(int startIdx, int count) {
  int worst = 0;
  for (int i = 0; i < count && (startIdx + i) < pathLength; i++) {
    int c = classifyCurvature(path[startIdx + i].avgError);
    if (c > worst) worst = c;
  }
  return worst;
}

int curvatureToSpeed(int curvatureClass) {
  switch (curvatureClass) {
    case 0:  return replayFast;
    case 1:  return replayMedium;
    default: return replaySlow;
  }
}

void printPath() {}

void startLearn() {
  pathLength     = 0;
  segLeftStart   = encLeftCount;
  segRightStart  = encRightCount;
  segErrorSum    = 0;
  segSampleCount = 0;
  resetPID();
  currentMode = LEARN;
}

void startReplay() {
  if (pathLength == 0) return;
  replayIndex      = 0;
  replayLeftStart  = encLeftCount;
  replayRightStart = encRightCount;
  resetPID();
  currentMode = REPLAY;
}

void enterIdle() {
  stopMotors();
  resetPID();
  currentMode = IDLE;
}

void loopLearn() {
  int pos = getPosition();

  if (pos == -1) {
    if (lastError > 0) { motorLeft(learnSpeed); motorRight(-learnSpeed); }
    else               { motorLeft(-learnSpeed); motorRight(learnSpeed); }
    return;
  }

  if (useSideSensors) {
    readSideLeft();
    readSideRight();
  }

  float pidOut = computePID(pos, Kp_learn, Ki_learn, Kd_learn);
  int error = 750 - (int)smoothPos;

  int leftSpeed  = constrain(learnSpeed + (int)pidOut, -maxSpeed, maxSpeed);
  int rightSpeed = constrain(learnSpeed - (int)pidOut, -maxSpeed, maxSpeed);

  motorLeft(leftSpeed);
  motorRight(rightSpeed);

  segErrorSum += error;
  segSampleCount++;

  long leftTraveled  = encLeftCount  - segLeftStart;
  long rightTraveled = encRightCount - segRightStart;
  long avgTraveled   = (abs(leftTraveled) + abs(rightTraveled)) / 2;

  if (avgTraveled >= SEGMENT_TICKS) {
    if (pathLength < MAX_SEGMENTS) {
      path[pathLength].leftTicks  = (int16_t)leftTraveled;
      path[pathLength].rightTicks = (int16_t)rightTraveled;
      path[pathLength].avgError   = (segSampleCount > 0)
                                    ? (int16_t)(segErrorSum / segSampleCount)
                                    : 0;
      pathLength++;
    } else {
      enterIdle();
      return;
    }

    segLeftStart   = encLeftCount;
    segRightStart  = encRightCount;
    segErrorSum    = 0;
    segSampleCount = 0;
  }
}

void loopReplay() {
  if (replayIndex >= pathLength) {
    enterIdle();
    return;
  }

  int pos = getPosition();

  if (pos == -1) {
    if (lastError > 0) { motorLeft(replaySlow); motorRight(-replaySlow); }
    else               { motorLeft(-replaySlow); motorRight(replaySlow); }
    return;
  }

  int upcomingCurvature = lookaheadCurvature(replayIndex, LOOKAHEAD);
  int targetSpeed = curvatureToSpeed(upcomingCurvature);

  int currentCurv = classifyCurvature(path[replayIndex].avgError);
  int currentSegSpeed = curvatureToSpeed(currentCurv);
  if (currentSegSpeed < targetSpeed) targetSpeed = currentSegSpeed;

  float pidOut = computePID(pos, Kp_replay, Ki_replay, Kd_replay);

  int leftSpeed  = constrain(targetSpeed + (int)pidOut, -maxSpeed, maxSpeed);
  int rightSpeed = constrain(targetSpeed - (int)pidOut, -maxSpeed, maxSpeed);

  motorLeft(leftSpeed);
  motorRight(rightSpeed);

  long leftTraveled  = encLeftCount  - replayLeftStart;
  long rightTraveled = encRightCount - replayRightStart;
  long avgTraveled   = (abs(leftTraveled) + abs(rightTraveled)) / 2;

  long expectedTicks = (abs(path[replayIndex].leftTicks)
                      + abs(path[replayIndex].rightTicks)) / 2;

  if (avgTraveled >= expectedTicks) {
    replayLeftStart  = encLeftCount;
    replayRightStart = encRightCount;
    replayIndex++;
  }
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);

  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(EN, OUTPUT);
  digitalWrite(EN, LOW);

  pinMode(SIDE_LEFT,  INPUT);
  pinMode(SIDE_RIGHT, INPUT);

  pinMode(ENC_LA, INPUT_PULLUP);
  pinMode(ENC_LB, INPUT_PULLUP);
  pinMode(ENC_RA, INPUT_PULLUP);
  pinMode(ENC_RB, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENC_LA), encLeftISR,  RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_RA), encRightISR, RISING);

  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(PWMA, OUTPUT);

  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(PWMB, OUTPUT);

  calibrate();
}

void loop() {
  if (Serial.available()) {
    char cmd = toupper(Serial.read());
    switch (cmd) {
      case 'L': startLearn();  break;
      case 'R': startReplay(); break;
      case 'S': enterIdle();   break;
      case 'P': printPath();   break;
    }
  }

  switch (currentMode) {
    case LEARN:   loopLearn();   break;
    case REPLAY:  loopReplay();  break;
    case IDLE:    stopMotors();  break;
  }

  delay(5);
}
