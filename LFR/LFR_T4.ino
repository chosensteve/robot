#define LIN1  24
#define LIN2  25
#define RIN1  37
#define RIN2  36

#define ELA   6
#define ELB   7
#define ERA   8
#define ERB   9

#define S0    0
#define S1    1
#define S2    2
#define S3    3
#define EN    13
#define ZOP   14

float kp     = 5.0;
float ki     = 0.0;
float kd     = 2.5;

int   base   = 150;
int   maxSpd = 255;

int   calMin[16];
int   calMax[16];
bool  calDone = false;

float intg    = 0;
float lastErr = 0;
float smooth  = 750.0;
float alpha   = 0.3;
float iMax    = 5000.0;
int   db      = 5;

void motorLeft(int s) {
  if (s >= 0) {
    analogWrite(LIN1, constrain(s, 0, maxSpd));
    analogWrite(LIN2, 0);
  } else {
    analogWrite(LIN1, 0);
    analogWrite(LIN2, constrain(-s, 0, maxSpd));
  }
}

void motorRight(int s) {
  if (s >= 0) {
    analogWrite(RIN1, constrain(s, 0, maxSpd));
    analogWrite(RIN2, 0);
  } else {
    analogWrite(RIN1, 0);
    analogWrite(RIN2, constrain(-s, 0, maxSpd));
  }
}

void stopMotors() {
  analogWrite(LIN1, 0);
  analogWrite(LIN2, 0);
  analogWrite(RIN1, 0);
  analogWrite(RIN2, 0);
}

int readMux(int ch) {
  digitalWrite(S0, ch & 1);
  digitalWrite(S1, (ch >> 1) & 1);
  digitalWrite(S2, (ch >> 2) & 1);
  digitalWrite(S3, (ch >> 3) & 1);
  delayMicroseconds(15);
  return analogRead(ZOP);
}

void calibrate() {
  for (int i = 0; i < 16; i++) {
    int v = readMux(i);
    calMin[i] = v;
    calMax[i] = v;
  }

  unsigned long t = millis();
  while (millis() - t < 3000) {
    for (int i = 0; i < 16; i++) {
      int v = readMux(i);
      if (v < calMin[i]) calMin[i] = v;
      if (v > calMax[i]) calMax[i] = v;
    }
  }

  calDone = true;
}

int getPosition() {
  long wSum = 0;
  long wTot = 0;

  for (int i = 0; i < 16; i++) {
    int raw = readMux(i);
    int val;

    if (calDone) {
      int rng = calMax[i] - calMin[i];
      if (rng < 50) rng = 50;
      val = constrain((long)(raw - calMin[i]) * 1000 / rng, 0, 1000);
    } else {
      val = (raw > 2048) ? 1000 : 0;
    }

    if (val > 300) {
      wSum += (long)i * 1000 * val;
      wTot += val;
    }
  }

  if (wTot == 0) return -1;
  return (int)(wSum / wTot);
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

  pinMode(ELA, INPUT_PULLUP);
  pinMode(ELB, INPUT_PULLUP);
  pinMode(ERA, INPUT_PULLUP);
  pinMode(ERB, INPUT_PULLUP);

  pinMode(LIN1, OUTPUT);
  pinMode(LIN2, OUTPUT);
  pinMode(RIN1, OUTPUT);
  pinMode(RIN2, OUTPUT);

  stopMotors();
  calibrate();
}

void loop() {
  int pos = getPosition();

  if (pos == -1) {
    if (lastErr > 0) {
      motorLeft(base);
      motorRight(-base);
    } else {
      motorLeft(-base);
      motorRight(base);
    }
    return;
  }

  smooth = smooth * (1.0 - alpha) + pos * alpha;

  int err = 7500 - (int)smooth;
  if (abs(err) < db) err = 0;

  float P = err;

  intg += err;
  intg = constrain(intg, -iMax, iMax);

  if ((err > 0 && lastErr < 0) || (err < 0 && lastErr > 0)) {
    intg = 0;
  }

  float D = err - lastErr;
  lastErr = err;

  float pid = (kp * P) + (ki * intg) + (kd * D);

  int lSpd = constrain(base + (int)pid, -maxSpd, maxSpd);
  int rSpd = constrain(base - (int)pid, -maxSpd, maxSpd);

  motorLeft(lSpd);
  motorRight(rSpd);

  static unsigned long lp = 0;
  if (millis() - lp > 50) {
    lp = millis();
    Serial.print(pos);  Serial.print('\t');
    Serial.print(err);  Serial.print('\t');
    Serial.print(lSpd); Serial.print('\t');
    Serial.println(rSpd);
  }

  delay(5);
}
