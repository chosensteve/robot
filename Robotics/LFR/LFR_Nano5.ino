#define SENSOR_1  A0
#define SENSOR_2  A1
#define SENSOR_3  A2
#define SENSOR_4  A3
#define SENSOR_5  A4

const uint8_t sensorPins[5] = { SENSOR_1, SENSOR_2, SENSOR_3, SENSOR_4, SENSOR_5 };
#define NUM_SENSORS  5

#define LEFT_PWM   3
#define LEFT_DIR1  4
#define LEFT_DIR2  2

#define RIGHT_PWM  5
#define RIGHT_DIR1 7
#define RIGHT_DIR2 8

float Kp = 8.0;
float Ki = 0.0;
float Kd = 4.0;

int   baseSpeed    = 150;
int   maxSpeed     = 255;
int   turnBoost    = 30;

int   deadband     = 3;
float integralMax  = 5000.0;
float smoothAlpha  = 0.3;

int   lineThreshold = 300;

float integral   = 0;
float lastError  = 0;
float smoothPos  = 200.0;

int   calMin[NUM_SENSORS];
int   calMax[NUM_SENSORS];
bool  calibrated = false;

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

int readSensor(int index) {
  return analogRead(sensorPins[index]);
}

void calibrate() {
  for (int i = 0; i < NUM_SENSORS; i++) {
    int v = readSensor(i);
    calMin[i] = v;
    calMax[i] = v;
  }

  unsigned long start = millis();
  while (millis() - start < 3000) {
    for (int i = 0; i < NUM_SENSORS; i++) {
      int v = readSensor(i);
      if (v < calMin[i]) calMin[i] = v;
      if (v > calMax[i]) calMax[i] = v;
    }
  }

  calibrated = true;
  for (int i = 0; i < NUM_SENSORS; i++) {
    Serial.print(F("  S")); Serial.print(i);
    Serial.print(F(" min=")); Serial.print(calMin[i]);
    Serial.print(F(" max=")); Serial.println(calMax[i]);
  }
}

int getPosition() {
  long weightedSum = 0;
  long totalValue  = 0;

  for (int i = 0; i < NUM_SENSORS; i++) {
    int raw = readSensor(i);
    int val;

    if (calibrated) {
      int range = calMax[i] - calMin[i];
      if (range < 30) range = 30;
      val = (long)(raw - calMin[i]) * 1000 / range;
      val = constrain(val, 0, 1000);
    } else {
      val = (raw > 500) ? 1000 : 0;
    }

    if (val > lineThreshold) {
      weightedSum += (long)i * 100 * val;
      totalValue  += val;
    }
  }

  if (totalValue == 0) return -1;
  return (int)(weightedSum / totalValue);
}

void setup() {
  Serial.begin(115200);

  pinMode(LEFT_PWM,  OUTPUT);
  pinMode(LEFT_DIR1, OUTPUT);
  pinMode(LEFT_DIR2, OUTPUT);

  pinMode(RIGHT_PWM,  OUTPUT);
  pinMode(RIGHT_DIR1, OUTPUT);
  pinMode(RIGHT_DIR2, OUTPUT);

  stopMotors();
  calibrate();
}

void loop() {
  int pos = getPosition();

  if (pos == -1) {
    if (lastError > 0) {
      motorLeft(-baseSpeed / 2);
      motorRight(baseSpeed / 2);
    } else {
      motorLeft(baseSpeed / 2);
      motorRight(-baseSpeed / 2);
    }
    return;
  }

  smoothPos = smoothPos * (1.0 - smoothAlpha) + pos * smoothAlpha;

  int error = 200 - (int)smoothPos;
  if (abs(error) < deadband) error = 0;

  float P = error;

  integral += error;
  integral = constrain(integral, -integralMax, integralMax);
  if ((error > 0 && lastError < 0) || (error < 0 && lastError > 0)) {
    integral = 0;
  }

  float D = error - lastError;
  lastError = error;

  float pidOutput = (Kp * P) + (Ki * integral) + (Kd * D);

  int leftSpeed  = baseSpeed + (int)pidOutput;
  int rightSpeed = baseSpeed - (int)pidOutput;

  if (abs(error) > 100) {
    leftSpeed  += (error > 0) ? turnBoost : -turnBoost;
    rightSpeed += (error > 0) ? -turnBoost : turnBoost;
  }

  leftSpeed  = constrain(leftSpeed,  -maxSpeed, maxSpeed);
  rightSpeed = constrain(rightSpeed, -maxSpeed, maxSpeed);

  motorLeft(leftSpeed);
  motorRight(rightSpeed);

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

  delay(2);
}
