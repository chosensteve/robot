// ---------------- PIN SETUP ----------------
#define S0 14
#define S1 27
#define S2 26
#define S3 25
#define Z_OP 34    // ADC input from MUX

#define AIN1 16
#define AIN2 17
#define PWMA 18

#define BIN1 19
#define BIN2 21
#define PWMB 22

#define STBY 5

// ---------------- PID PARAMETERS ----------------
float Kp = 7.0;
float Ki = 0.0;
float Kd = 3.0;

float integral = 0;
float lastError = 0;

// ---------------- MOTOR CONTROL ----------------
void motorL(int spd) {
  if (spd >= 0) {
    digitalWrite(AIN1, 0);
    digitalWrite(AIN2, 1);
    analogWrite(PWMA, spd);
  } else {
    digitalWrite(AIN1, 1);
    digitalWrite(AIN2, 0);
    analogWrite(PWMA, -spd);
  }
}

void motorR(int spd) {
  if (spd >= 0) {
    digitalWrite(BIN1, 0);
    digitalWrite(BIN2, 1);
    analogWrite(PWMB, spd);
  } else {
    digitalWrite(BIN1, 1);
    digitalWrite(BIN2, 0);
    analogWrite(PWMB, -spd);
  }
}

// ---------------- READ MUX ----------------
int readMux(int ch) {
  digitalWrite(S0, ch & 1);
  digitalWrite(S1, (ch >> 1) & 1);
  digitalWrite(S2, (ch >> 2) & 1);
  digitalWrite(S3, (ch >> 3) & 1);
  delayMicroseconds(50);
  return analogRead(Z_OP);
}

// ---------------- GET WEIGHTED POSITION ----------------
int getPosition() {
  long sumW = 0;
  long sum = 0;
  const int THRESH = 1500;  // adjust based on your line sensor calibration

  for (int i = 0; i < 16; i++) {
    int val = readMux(i);
    if (val < THRESH) { // black line detected
      sumW += i * 100;   // weighted position
      sum++;
    }
  }

  if (sum == 0) return -1; // no line detected
  return sumW / sum;
}

// ---------------- SETUP ----------------
void setup() {
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(Z_OP, INPUT);

  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(PWMA, OUTPUT);

  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(PWMB, OUTPUT);

  pinMode(STBY, OUTPUT);
  digitalWrite(STBY, 1);  // enable motor driver
}

// ---------------- MAIN LOOP ----------------
void loop() {
  int pos = getPosition();

  // STOP if no line detected
  if (pos == -1) {
    motorL(0);
    motorR(0);
    integral = 0;
    lastError = 0;
    return;
  }

  // Smooth sensor readings
  static int smoothPos = 750;
  smoothPos = (smoothPos * 7 + pos * 3) / 10;

  // Compute PID error with deadband
  int error = 750 - smoothPos;
  if (abs(error) < 5) error = 0;  // deadband to prevent fidgeting

  integral += error;
  float derivative = error - lastError;
  lastError = error;

  float pid = Kp * error + Ki * integral + Kd * derivative;

  int baseSpeed = 150;  // adjust speed for your N20 motors
  int leftSpeed = baseSpeed + pid;
  int rightSpeed = baseSpeed - pid;

  leftSpeed = constrain(leftSpeed, -255, 255);
  rightSpeed = constrain(rightSpeed, -255, 255);

  motorL(leftSpeed);
  motorR(rightSpeed);

  delay(10);  // small loop delay
}
