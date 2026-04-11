/*
 * ============================================================
 *  LINE FOLLOWING ROBOT — Teensy 4.1  —  PATH LEARNING
 *  16-channel Mux Sensor | Dual Motor Drivers | PID + Encoders
 * ============================================================
 *
 *  HOW IT WORKS:
 *  ─────────────
 *  1. CALIBRATE  — sensor auto-calibration on power-up
 *  2. LEARN      — robot follows the line at safe speed using PID.
 *                  Encoders record the track profile (distance + curvature).
 *                  Learning ends when you send 'S' over serial.
 *  3. REPLAY     — robot replays the recorded path at a FASTER speed.
 *                  It looks ahead at upcoming segments; slows for
 *                  curves, accelerates on straights. PID still runs
 *                  in the background for fine correction.
 *
 *  SERIAL COMMANDS:
 *    'L' — start LEARN mode
 *    'R' — start REPLAY mode
 *    'S' — stop / return to IDLE
 *    'P' — print recorded path to serial
 *
 *  WIRING (as per your setup):
 *  ──────────────────────────
 *  SENSOR ARRAY (16ch mux)
 *    S0 → GPIO 0    S1 → GPIO 1    S2 → GPIO 2    S3 → GPIO 3
 *    EN → GPIO 13   Z_OP → GPIO 14 (A0)
 *
 *  SIDE SENSORS (optional)
 *    Left  → GPIO 4 (digital)
 *    Right → GPIO 5 (digital)
 *
 *  ENCODERS
 *    A_M1 → GPIO 6    B_M1 → GPIO 7   (left motor)
 *    A_M2 → GPIO 8    B_M2 → GPIO 9   (right motor)
 *
 *  MOTOR DRIVER 1 (left)
 *    PWMA → GPIO 11   AIN1 → GPIO 24   AIN2 → GPIO 25
 *    STBY → tie to 3.3V
 *
 *  MOTOR DRIVER 2 (right)
 *    PWMA → GPIO 10   AIN1 → GPIO 26   AIN2 → GPIO 27
 *    STBY → tie to 3.3V
 */

// ===================== PIN DEFINITIONS =====================

// --- 16-channel mux sensor array ---
#define S0    0
#define S1    1
#define S2    2
#define S3    3
#define EN    13      // Active-LOW enable for the mux
#define Z_OP  14      // A0 on Teensy 4.1 — analog input from mux

// --- Optional side sensors (digital, active-low = line detected) ---
#define SIDE_LEFT   4
#define SIDE_RIGHT  5
bool useSideSensors = false;  // Set true to enable side sensor logic

// --- Encoders ---
#define ENC_LA  6     // Left encoder channel A  (A_M1)
#define ENC_LB  7     // Left encoder channel B  (B_M1)
#define ENC_RA  8     // Right encoder channel A  (A_M2)
#define ENC_RB  9     // Right encoder channel B  (B_M2)

// --- Motor driver 1 — LEFT (TB6612 A-channel) ---
#define PWMA  11      // Left motor speed
#define AIN1  24      // Left motor direction
#define AIN2  25      // Left motor direction

// --- Motor driver 2 — RIGHT (TB6612 A-channel) ---
#define PWMB  10      // Right motor speed
#define BIN1  26      // Right motor direction (AIN1 on driver 2)
#define BIN2  27      // Right motor direction (AIN2 on driver 2)

// NOTE: STBY on both drivers should be wired directly to 3.3V
//       (no GPIO needed — drivers are always enabled)

// ===================== TUNING — LEARN MODE =====================
float Kp_learn      = 5.0;
float Ki_learn      = 0.0;
float Kd_learn      = 2.5;
int   learnSpeed    = 120;    // Safe, slow speed for learning the track

// ===================== TUNING — REPLAY MODE =====================
float Kp_replay     = 3.5;   // Softer P since path profile handles steering
float Ki_replay     = 0.0;
float Kd_replay     = 2.0;
int   replayFast    = 220;   // Top speed on straights during replay
int   replayMedium  = 160;   // Speed on gentle curves
int   replaySlow    = 100;   // Speed on sharp curves

// ===================== SHARED TUNING =====================
int   maxSpeed      = 255;
int   deadband      = 5;
float integralMax   = 5000.0;
float smoothAlpha   = 0.3;

// ===================== PATH RECORDING =====================
struct PathSegment {
  int16_t  leftTicks;     // Left encoder ticks in this segment
  int16_t  rightTicks;    // Right encoder ticks in this segment
  int16_t  avgError;      // Average PID error (curvature signature)
};

// Teensy 4.1 has 1MB RAM — we can store a LOT of segments
#define MAX_SEGMENTS  4000
PathSegment path[MAX_SEGMENTS];
int pathLength = 0;

// How many total encoder ticks (avg of L+R) before we close a segment
#define SEGMENT_TICKS  50

// Lookahead: how many segments ahead to scan for upcoming curves
#define LOOKAHEAD  8

// ===================== ENCODER STATE (interrupt-driven) =====================
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

// ===================== PID STATE =====================
float integral   = 0;
float lastError  = 0;
float smoothPos  = 750.0;

// ===================== CALIBRATION =====================
int   calMin[16];
int   calMax[16];
bool  calibrated = false;

// ===================== MODE STATE =====================
enum Mode { IDLE, LEARN, REPLAY };
Mode currentMode = IDLE;

// Segment accumulation variables (used during LEARN)
long segLeftStart   = 0;
long segRightStart  = 0;
long segErrorSum    = 0;
int  segSampleCount = 0;

// Replay tracking
int  replayIndex    = 0;
long replayLeftStart  = 0;
long replayRightStart = 0;

// ===================== MOTOR HELPERS =====================
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

// ===================== SENSOR READ =====================
int readMux(int channel) {
  digitalWrite(S0, channel & 1);
  digitalWrite(S1, (channel >> 1) & 1);
  digitalWrite(S2, (channel >> 2) & 1);
  digitalWrite(S3, (channel >> 3) & 1);
  delayMicroseconds(15);
  return analogRead(Z_OP);
}

// ===================== SIDE SENSOR READ =====================
// Returns true if the side sensor detects a line
// Adjust logic (HIGH/LOW) based on your sensor's active state
bool readSideLeft()  { return digitalRead(SIDE_LEFT)  == LOW; }
bool readSideRight() { return digitalRead(SIDE_RIGHT) == LOW; }

// ===================== CALIBRATION =====================
void calibrate() {
  Serial.println(">> CALIBRATING — sweep robot over the line...");

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
  Serial.println(">> Calibration complete.");
  for (int i = 0; i < 16; i++) {
    Serial.print("  S"); Serial.print(i);
    Serial.print(" min="); Serial.print(calMin[i]);
    Serial.print(" max="); Serial.println(calMax[i]);
  }
}

// ===================== GET LINE POSITION =====================
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

// ===================== PID COMPUTE =====================
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

// ===================== RESET PID STATE =====================
void resetPID() {
  integral  = 0;
  lastError = 0;
  smoothPos = 750.0;
}

// ===================== PATH ANALYSIS =====================
int classifyCurvature(int16_t avgError) {
  int absErr = abs(avgError);
  if (absErr < 40)  return 0;  // Straight
  if (absErr < 150) return 1;  // Gentle curve
  return 2;                     // Sharp curve
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

// ===================== PRINT PATH =====================
void printPath() {
  Serial.println("=== RECORDED PATH ===");
  Serial.print("Segments: "); Serial.println(pathLength);
  Serial.println("IDX\tL_TICK\tR_TICK\tAVG_ERR\tCURVE");

  for (int i = 0; i < pathLength; i++) {
    Serial.print(i);                    Serial.print('\t');
    Serial.print(path[i].leftTicks);    Serial.print('\t');
    Serial.print(path[i].rightTicks);   Serial.print('\t');
    Serial.print(path[i].avgError);     Serial.print('\t');

    int c = classifyCurvature(path[i].avgError);
    if (c == 0)      Serial.println("STRAIGHT");
    else if (c == 1) Serial.println("GENTLE");
    else             Serial.println("SHARP");
  }
  Serial.println("=== END PATH ===");
}

// ===================== START LEARN =====================
void startLearn() {
  pathLength     = 0;
  segLeftStart   = encLeftCount;
  segRightStart  = encRightCount;
  segErrorSum    = 0;
  segSampleCount = 0;
  resetPID();
  currentMode = LEARN;
  Serial.println(">> MODE: LEARN — following line, recording path...");
  Serial.print("   Segment size: "); Serial.print(SEGMENT_TICKS);
  Serial.println(" ticks");
}

// ===================== START REPLAY =====================
void startReplay() {
  if (pathLength == 0) {
    Serial.println(">> ERROR: No path recorded! Run LEARN first.");
    return;
  }
  replayIndex      = 0;
  replayLeftStart  = encLeftCount;
  replayRightStart = encRightCount;
  resetPID();
  currentMode = REPLAY;
  Serial.println(">> MODE: REPLAY — running recorded path at high speed");
  Serial.print("   Path segments: "); Serial.println(pathLength);
}

// ===================== STOP =====================
void enterIdle() {
  stopMotors();
  resetPID();
  currentMode = IDLE;
  Serial.println(">> MODE: IDLE");
}

// ===================== LEARN LOOP =====================
void loopLearn() {
  int pos = getPosition();

  // Line lost — spin to recover
  if (pos == -1) {
    if (lastError > 0) { motorLeft(learnSpeed); motorRight(-learnSpeed); }
    else               { motorLeft(-learnSpeed); motorRight(learnSpeed); }
    return;
  }

  // Optional: check side sensors for edge detection
  if (useSideSensors) {
    if (readSideLeft()) {
      // Line detected far left — bias the position
      Serial.println("[LEARN] Side LEFT triggered");
    }
    if (readSideRight()) {
      // Line detected far right — bias the position
      Serial.println("[LEARN] Side RIGHT triggered");
    }
  }

  // Compute PID
  float pidOut = computePID(pos, Kp_learn, Ki_learn, Kd_learn);
  int error = 750 - (int)smoothPos;

  int leftSpeed  = learnSpeed + (int)pidOut;
  int rightSpeed = learnSpeed - (int)pidOut;
  leftSpeed  = constrain(leftSpeed,  -maxSpeed, maxSpeed);
  rightSpeed = constrain(rightSpeed, -maxSpeed, maxSpeed);

  motorLeft(leftSpeed);
  motorRight(rightSpeed);

  // ---- ACCUMULATE SEGMENT DATA ----
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

      if (pathLength % 100 == 0) {
        Serial.print("   Recorded "); Serial.print(pathLength);
        Serial.println(" segments...");
      }
    } else {
      Serial.println(">> PATH FULL! Stopping learn.");
      enterIdle();
      return;
    }

    segLeftStart   = encLeftCount;
    segRightStart  = encRightCount;
    segErrorSum    = 0;
    segSampleCount = 0;
  }

  // Diagnostics
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 200) {
    lastPrint = millis();
    Serial.print("[LEARN] seg:"); Serial.print(pathLength);
    Serial.print(" enc L:"); Serial.print(encLeftCount);
    Serial.print(" R:"); Serial.print(encRightCount);
    Serial.print(" err:"); Serial.println(error);
  }
}

// ===================== REPLAY LOOP =====================
void loopReplay() {
  if (replayIndex >= pathLength) {
    Serial.println(">> REPLAY COMPLETE — lap finished!");
    enterIdle();
    return;
  }

  int pos = getPosition();

  // Line lost — spin to recover using PID memory
  if (pos == -1) {
    if (lastError > 0) { motorLeft(replaySlow); motorRight(-replaySlow); }
    else               { motorLeft(-replaySlow); motorRight(replaySlow); }
    return;
  }

  // ---- DETERMINE SPEED FROM PATH PROFILE ----
  int upcomingCurvature = lookaheadCurvature(replayIndex, LOOKAHEAD);
  int targetSpeed = curvatureToSpeed(upcomingCurvature);

  int currentCurv = classifyCurvature(path[replayIndex].avgError);
  int currentSegSpeed = curvatureToSpeed(currentCurv);
  if (currentSegSpeed < targetSpeed) targetSpeed = currentSegSpeed;

  // ---- PID CORRECTION (background fine-tuning) ----
  float pidOut = computePID(pos, Kp_replay, Ki_replay, Kd_replay);

  int leftSpeed  = targetSpeed + (int)pidOut;
  int rightSpeed = targetSpeed - (int)pidOut;
  leftSpeed  = constrain(leftSpeed,  -maxSpeed, maxSpeed);
  rightSpeed = constrain(rightSpeed, -maxSpeed, maxSpeed);

  motorLeft(leftSpeed);
  motorRight(rightSpeed);

  // ---- ADVANCE SEGMENT based on encoder distance ----
  long leftTraveled  = encLeftCount  - replayLeftStart;
  long rightTraveled = encRightCount - replayRightStart;
  long avgTraveled   = (abs(leftTraveled) + abs(rightTraveled)) / 2;

  long expectedTicks = (abs(path[replayIndex].leftTicks)
                      + abs(path[replayIndex].rightTicks)) / 2;

  if (avgTraveled >= expectedTicks) {
    replayLeftStart  = encLeftCount;
    replayRightStart = encRightCount;
    replayIndex++;

    if (replayIndex % 50 == 0) {
      Serial.print("[REPLAY] seg "); Serial.print(replayIndex);
      Serial.print("/"); Serial.print(pathLength);
      Serial.print(" spd:"); Serial.println(targetSpeed);
    }
  }

  // Diagnostics
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 100) {
    lastPrint = millis();
    Serial.print("[REPLAY] seg:"); Serial.print(replayIndex);
    Serial.print("/"); Serial.print(pathLength);
    Serial.print(" spd:"); Serial.print(targetSpeed);
    Serial.print(" curv:"); Serial.print(upcomingCurvature);
    Serial.print(" L:"); Serial.print(leftSpeed);
    Serial.print(" R:"); Serial.println(rightSpeed);
  }
}

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000);

  // Teensy 4.1 — 12-bit ADC
  analogReadResolution(12);

  // Mux sensor pins
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(EN, OUTPUT);
  digitalWrite(EN, LOW);    // Enable the mux (active-low)

  // Optional side sensors
  pinMode(SIDE_LEFT,  INPUT);
  pinMode(SIDE_RIGHT, INPUT);

  // Encoder pins — input with internal pullup
  pinMode(ENC_LA, INPUT_PULLUP);
  pinMode(ENC_LB, INPUT_PULLUP);
  pinMode(ENC_RA, INPUT_PULLUP);
  pinMode(ENC_RB, INPUT_PULLUP);

  // Attach interrupts on channel A (rising edge)
  attachInterrupt(digitalPinToInterrupt(ENC_LA), encLeftISR,  RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_RA), encRightISR, RISING);

  // Left motor driver
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(PWMA, OUTPUT);

  // Right motor driver
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(PWMB, OUTPUT);

  // NOTE: STBY pins on both motor drivers should be wired to 3.3V
  //       No GPIO control needed — drivers are always active

  // Calibrate sensors
  calibrate();

  Serial.println();
  Serial.println("============================================");
  Serial.println("  LFR PATH LEARNER — Teensy 4.1");
  Serial.println("============================================");
  Serial.println("  L = Learn    R = Replay");
  Serial.println("  S = Stop     P = Print path");
  Serial.println("============================================");
  Serial.print("  Side sensors: ");
  Serial.println(useSideSensors ? "ENABLED" : "DISABLED");
  Serial.println();
}

// ===================== MAIN LOOP =====================
void loop() {
  // Handle serial commands
  if (Serial.available()) {
    char cmd = toupper(Serial.read());
    switch (cmd) {
      case 'L': startLearn();   break;
      case 'R': startReplay();  break;
      case 'S': enterIdle();    break;
      case 'P': printPath();    break;
    }
  }

  // Run current mode
  switch (currentMode) {
    case LEARN:   loopLearn();   break;
    case REPLAY:  loopReplay();  break;
    case IDLE:    stopMotors();  break;
  }

  delay(5);
}
