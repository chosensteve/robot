/*
 * ============================================================
 *  LINE FOLLOWING ROBOT — Teensy 4.1
 *  16-channel Mux Sensor  |  Dual Motor Drivers  |  PID
 * ============================================================
 *
 *  WIRING:
 *  ──────
 *  SENSOR ARRAY (16ch mux)
 *    S0 → GPIO 0    S1 → GPIO 1    S2 → GPIO 2    S3 → GPIO 3
 *    EN → GPIO 13   Z_OP → GPIO 14 (A0)
 *
 *  SIDE SENSORS (optional)
 *    Left  → GPIO 4 (digital)
 *    Right → GPIO 5 (digital)
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

// --- Optional side sensors ---
#define SIDE_LEFT   4
#define SIDE_RIGHT  5

// --- Motor driver 1 — LEFT (TB6612 A-channel) ---
#define AIN1  24
#define AIN2  25
#define PWMA  11

// --- Motor driver 2 — RIGHT (TB6612 A-channel) ---
#define BIN1  26      // AIN1 on driver 2
#define BIN2  27      // AIN2 on driver 2
#define PWMB  10      // PWMA on driver 2

// NOTE: STBY on both drivers wired to 3.3V — always active

// ===================== TUNING PARAMETERS =====================
float Kp = 5.0;        // Proportional gain — primary steering response
float Ki = 0.0;        // Integral gain     — corrects lingering offset
float Kd = 2.5;        // Derivative gain   — dampens oscillation

int   baseSpeed    = 150;   // Cruise speed (0-255)
int   maxSpeed     = 255;   // Ceiling for motor PWM
int   turnBoost    = 0;     // Extra speed added during hard turns

int   deadband     = 5;
float integralMax  = 5000.0;
float smoothAlpha  = 0.3;

// ===================== PID STATE =====================
float integral   = 0;
float lastError  = 0;
float smoothPos  = 750.0;

// ===================== CALIBRATION =====================
int   calMin[16];
int   calMax[16];
bool  calibrated = false;

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

// ===================== CALIBRATION ROUTINE =====================
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

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000);

  analogReadResolution(12);

  // Mux pins
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(EN, OUTPUT);
  digitalWrite(EN, LOW);    // Enable the mux (active-low)

  // Side sensors (optional)
  pinMode(SIDE_LEFT,  INPUT);
  pinMode(SIDE_RIGHT, INPUT);

  // Left motor
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(PWMA, OUTPUT);

  // Right motor
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(PWMB, OUTPUT);

  // Calibrate sensors
  calibrate();

  Serial.println(">> LFR READY — PID control active");
}

// ===================== MAIN LOOP =====================
void loop() {
  int pos = getPosition();

  // ---- LINE LOST ----
  if (pos == -1) {
    if (lastError > 0) {
      motorLeft(baseSpeed);
      motorRight(-baseSpeed);
    } else {
      motorLeft(-baseSpeed);
      motorRight(baseSpeed);
    }
    return;
  }

  // ---- EXPONENTIAL SMOOTHING ----
  smoothPos = smoothPos * (1.0 - smoothAlpha) + pos * smoothAlpha;

  // ---- COMPUTE ERROR ----
  int error = 750 - (int)smoothPos;
  if (abs(error) < deadband) error = 0;

  // ---- PID CALCULATION ----
  float P = error;
  integral += error;
  integral = constrain(integral, -integralMax, integralMax);
  float D = error - lastError;
  lastError = error;

  float pidOutput = (Kp * P) + (Ki * integral) + (Kd * D);

  // ---- APPLY TO MOTORS ----
  int leftSpeed  = baseSpeed + (int)pidOutput;
  int rightSpeed = baseSpeed - (int)pidOutput;

  if (abs(error) > 300) {
    leftSpeed  += (error > 0) ? turnBoost : -turnBoost;
    rightSpeed += (error > 0) ? -turnBoost : turnBoost;
  }

  leftSpeed  = constrain(leftSpeed,  -maxSpeed, maxSpeed);
  rightSpeed = constrain(rightSpeed, -maxSpeed, maxSpeed);

  motorLeft(leftSpeed);
  motorRight(rightSpeed);

  // ---- SERIAL DIAGNOSTICS ----
  static unsigned long lastPrint = 0;
  if (abs(error) > 20 && millis() - lastPrint > 50) {
    lastPrint = millis();
    Serial.print("POS:"); Serial.print(pos);
    Serial.print(" SM:");  Serial.print((int)smoothPos);
    Serial.print(" E:");   Serial.print(error);
    Serial.print(" P:");   Serial.print(P);
    Serial.print(" I:");   Serial.print(integral);
    Serial.print(" D:");   Serial.print(D, 1);
    Serial.print(" PID:"); Serial.print(pidOutput, 1);
    Serial.print(" L:");   Serial.print(leftSpeed);
    Serial.print(" R:");   Serial.println(rightSpeed);
  }

  delay(5);
}
