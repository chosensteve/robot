/*
 * ============================================================
 *  LINE FOLLOWING ROBOT — Arduino Nano
 *  5-Sensor Analog Array  |  Dual Motor Drivers  |  PID
 * ============================================================
 *
 *  WIRING:
 *  ──────
 *  SENSOR ARRAY (5 analog IR sensors)
 *    Out 1 → A0    Out 2 → A1    Out 3 → A2
 *    Out 4 → A3    Out 5 → A4
 *
 *  MOTOR DRIVER 1 — LEFT
 *    PWM   → D3   (PWMA1 + PWMB1 tied together)
 *    DIR1  → D4   (AIN1_1 + BIN1_1 tied together)
 *    DIR2  → D2   (AIN2_1 + BIN2_1 tied together)
 *    → Both A and B channels of driver 1 are paralleled
 *      to double current capacity for the left motor.
 *
 *  MOTOR DRIVER 2 — RIGHT
 *    PWM   → D5   (PWMA2 + PWMB2 tied together)
 *    DIR1  → D7   (AIN1_2 + BIN1_2 tied together)
 *    DIR2  → D8   (AIN2_2 + BIN2_2 tied together)
 *    → Both A and B channels of driver 2 are paralleled
 *      to double current capacity for the right motor.
 *
 *  NOTE: STBY on both drivers should be tied to 5V (always active).
 */

// ===================== PIN DEFINITIONS =====================

// --- 5-sensor analog array ---
#define SENSOR_1  A0
#define SENSOR_2  A1
#define SENSOR_3  A2
#define SENSOR_4  A3
#define SENSOR_5  A4

const uint8_t sensorPins[5] = { SENSOR_1, SENSOR_2, SENSOR_3, SENSOR_4, SENSOR_5 };
#define NUM_SENSORS  5

// --- Motor driver 1 — LEFT ---
#define LEFT_PWM   3    // D3  — PWM speed (paralleled PWMA1 + PWMB1)
#define LEFT_DIR1  4    // D4  — direction (paralleled AIN1_1 + BIN1_1)
#define LEFT_DIR2  2    // D2  — direction (paralleled AIN2_1 + BIN2_1)

// --- Motor driver 2 — RIGHT ---
#define RIGHT_PWM  5    // D5  — PWM speed (paralleled PWMA2 + PWMB2)
#define RIGHT_DIR1 7    // D7  — direction (paralleled AIN1_2 + BIN1_2)
#define RIGHT_DIR2 8    // D8  — direction (paralleled AIN2_2 + BIN2_2)

// NOTE: STBY pins on both drivers should be tied to 5V

// ===================== TUNING PARAMETERS =====================
float Kp = 8.0;         // Proportional gain — primary steering response
float Ki = 0.0;         // Integral gain     — corrects lingering offset
float Kd = 4.0;         // Derivative gain   — dampens oscillation

int   baseSpeed    = 150;    // Cruise speed (0–255)
int   maxSpeed     = 255;    // Ceiling for motor PWM
int   turnBoost    = 30;     // Extra speed added during hard turns

int   deadband     = 3;      // Errors smaller than this treated as zero
float integralMax  = 5000.0; // Anti-windup clamp
float smoothAlpha  = 0.3;    // Exponential smoothing factor (0–1)

// Sensor threshold: values above this count as "seeing the line"
// Adjust after calibration — depends on surface & sensor height
int   lineThreshold = 300;   // Post-calibration normalized value (0–1000)

// ===================== PID STATE =====================
float integral   = 0;
float lastError  = 0;
float smoothPos  = 200.0;    // Center for 5 sensors (0–400 range)

// ===================== CALIBRATION =====================
int   calMin[NUM_SENSORS];
int   calMax[NUM_SENSORS];
bool  calibrated = false;

// ===================== MOTOR HELPERS =====================

// Drive left motor: positive = forward, negative = reverse
void motorLeft(int spd) {
  if (spd >= 0) {
    digitalWrite(LEFT_DIR1, LOW);
    digitalWrite(LEFT_DIR2, HIGH);
    analogWrite(LEFT_PWM, constrain(spd, 0, maxSpeed));
  } else {
    digitalWrite(LEFT_DIR1, HIGH);
    digitalWrite(LEFT_DIR2, LOW);
    analogWrite(LEFT_PWM, constrain(-spd, 0, maxSpeed));
  }
}

// Drive right motor: positive = forward, negative = reverse
void motorRight(int spd) {
  if (spd >= 0) {
    digitalWrite(RIGHT_DIR1, LOW);
    digitalWrite(RIGHT_DIR2, HIGH);
    analogWrite(RIGHT_PWM, constrain(spd, 0, maxSpeed));
  } else {
    digitalWrite(RIGHT_DIR1, HIGH);
    digitalWrite(RIGHT_DIR2, LOW);
    analogWrite(RIGHT_PWM, constrain(-spd, 0, maxSpeed));
  }
}

void stopMotors() {
  analogWrite(LEFT_PWM, 0);
  analogWrite(RIGHT_PWM, 0);
}

// ===================== SENSOR READ =====================

// Read a single sensor (returns 0–1023 on Nano's 10-bit ADC)
int readSensor(int index) {
  return analogRead(sensorPins[index]);
}

// ===================== CALIBRATION ROUTINE =====================
void calibrate() {
  Serial.println(F(">> CALIBRATING — sweep robot over the line..."));

  // Seed min/max with first reading
  for (int i = 0; i < NUM_SENSORS; i++) {
    int v = readSensor(i);
    calMin[i] = v;
    calMax[i] = v;
  }

  // Collect min/max over 3 seconds
  unsigned long start = millis();
  while (millis() - start < 3000) {
    for (int i = 0; i < NUM_SENSORS; i++) {
      int v = readSensor(i);
      if (v < calMin[i]) calMin[i] = v;
      if (v > calMax[i]) calMax[i] = v;
    }
  }

  calibrated = true;
  Serial.println(F(">> Calibration complete."));
  for (int i = 0; i < NUM_SENSORS; i++) {
    Serial.print(F("  S")); Serial.print(i);
    Serial.print(F(" min=")); Serial.print(calMin[i]);
    Serial.print(F(" max=")); Serial.println(calMax[i]);
  }
}

// ===================== GET LINE POSITION =====================
//
//  Returns a weighted-average position of the line:
//    Sensor 0 = position 0
//    Sensor 1 = position 100
//    ...
//    Sensor 4 = position 400
//
//  Center of the line → ~200
//  Returns –1 if no sensor sees the line (line lost).
//
int getPosition() {
  long weightedSum = 0;
  long totalValue  = 0;

  for (int i = 0; i < NUM_SENSORS; i++) {
    int raw = readSensor(i);
    int val;

    if (calibrated) {
      int range = calMax[i] - calMin[i];
      if (range < 30) range = 30;   // Prevent divide-by-small-number
      val = (long)(raw - calMin[i]) * 1000 / range;
      val = constrain(val, 0, 1000);
    } else {
      // Fallback: simple threshold on raw 10-bit reading
      val = (raw > 500) ? 1000 : 0;
    }

    if (val > lineThreshold) {
      weightedSum += (long)i * 100 * val;   // Weighted by intensity
      totalValue  += val;
    }
  }

  if (totalValue == 0) return -1;  // No sensor sees the line
  return (int)(weightedSum / totalValue);
}

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);

  // Sensor pins are analog inputs by default on Nano — no pinMode needed

  // Left motor driver
  pinMode(LEFT_PWM,  OUTPUT);
  pinMode(LEFT_DIR1, OUTPUT);
  pinMode(LEFT_DIR2, OUTPUT);

  // Right motor driver
  pinMode(RIGHT_PWM,  OUTPUT);
  pinMode(RIGHT_DIR1, OUTPUT);
  pinMode(RIGHT_DIR2, OUTPUT);

  // Start with motors stopped
  stopMotors();

  // Calibrate sensors
  calibrate();

  Serial.println();
  Serial.println(F("============================================"));
  Serial.println(F("  LFR PID — Arduino Nano — 5 Sensor Array"));
  Serial.println(F("============================================"));
  Serial.print(F("  Kp=")); Serial.print(Kp);
  Serial.print(F("  Ki=")); Serial.print(Ki);
  Serial.print(F("  Kd=")); Serial.println(Kd);
  Serial.print(F("  Base Speed=")); Serial.print(baseSpeed);
  Serial.print(F("  Max Speed="));  Serial.println(maxSpeed);
  Serial.println(F(">> PID control active"));
  Serial.println();
}

// ===================== MAIN LOOP =====================
void loop() {

  // ---- SERIAL TUNING (optional live adjustment) ----
  // Send commands: "P8.5" "I0.01" "D5.0" "B160" to adjust on the fly
  if (Serial.available()) {
    char cmd = Serial.read();
    float val = Serial.parseFloat();
    switch (cmd) {
      case 'P': case 'p': Kp = val; Serial.print(F("Kp=")); Serial.println(Kp); break;
      case 'I': case 'i': Ki = val; Serial.print(F("Ki=")); Serial.println(Ki); break;
      case 'D': case 'd': Kd = val; Serial.print(F("Kd=")); Serial.println(Kd); break;
      case 'B': case 'b': baseSpeed = (int)val; Serial.print(F("Base=")); Serial.println(baseSpeed); break;
      case 'C': case 'c': calibrate(); break;  // Re-calibrate
    }
  }

  // ---- READ POSITION ----
  int pos = getPosition();

  // ---- LINE LOST — spin to recover using last known direction ----
  if (pos == -1) {
    if (lastError > 0) {
      // Line was last seen to the left → spin left
      motorLeft(-baseSpeed / 2);
      motorRight(baseSpeed / 2);
    } else {
      // Line was last seen to the right → spin right
      motorLeft(baseSpeed / 2);
      motorRight(-baseSpeed / 2);
    }
    return;
  }

  // ---- EXPONENTIAL SMOOTHING ----
  smoothPos = smoothPos * (1.0 - smoothAlpha) + pos * smoothAlpha;

  // ---- COMPUTE ERROR ----
  // Center position = 200 (for 5 sensors: 0, 100, 200, 300, 400)
  int error = 200 - (int)smoothPos;
  if (abs(error) < deadband) error = 0;

  // ---- PID CALCULATION ----
  float P = error;

  integral += error;
  integral = constrain(integral, -integralMax, integralMax);
  // Reset integral on zero-crossing to prevent overshoot
  if ((error > 0 && lastError < 0) || (error < 0 && lastError > 0)) {
    integral = 0;
  }

  float D = error - lastError;
  lastError = error;

  float pidOutput = (Kp * P) + (Ki * integral) + (Kd * D);

  // ---- APPLY TO MOTORS ----
  int leftSpeed  = baseSpeed + (int)pidOutput;
  int rightSpeed = baseSpeed - (int)pidOutput;

  // Hard-turn boost — push outer wheel harder on sharp curves
  if (abs(error) > 100) {
    leftSpeed  += (error > 0) ? turnBoost : -turnBoost;
    rightSpeed += (error > 0) ? -turnBoost : turnBoost;
  }

  leftSpeed  = constrain(leftSpeed,  -maxSpeed, maxSpeed);
  rightSpeed = constrain(rightSpeed, -maxSpeed, maxSpeed);

  motorLeft(leftSpeed);
  motorRight(rightSpeed);

  // ---- SERIAL DIAGNOSTICS ----
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 100) {
    lastPrint = millis();
    Serial.print(F("POS:")); Serial.print(pos);
    Serial.print(F(" SM:"));  Serial.print((int)smoothPos);
    Serial.print(F(" E:"));   Serial.print(error);
    Serial.print(F(" P:"));   Serial.print(P);
    Serial.print(F(" I:"));   Serial.print(integral);
    Serial.print(F(" D:"));   Serial.print(D, 1);
    Serial.print(F(" PID:")); Serial.print(pidOutput, 1);
    Serial.print(F(" L:"));   Serial.print(leftSpeed);
    Serial.print(F(" R:"));   Serial.println(rightSpeed);
  }

  delay(2);  // ~500 Hz loop rate — fast enough for smooth PID on Nano
}
