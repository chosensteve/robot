// LINE FOLLOWING ROBOT - BANG BANG CONTROL
// 8-Sensor Array (RLS08) - Arduino Uno + L298N

// -------------------- SENSOR PINS --------------------
const int IR1 = 9;
const int IR2 = 7;
const int IR3 = 8;
const int IR4 = 6;
const int IR5 = 5;
const int IR6 = 4;
const int IR7 = 3;
const int IR8 = 2;

int sensor[8];

// -------------------- MOTOR DRIVER PINS --------------------
const int IN1 = A4;
const int IN2 = A3;
const int IN3 = A2;
const int IN4 = A1;
const int ENA = 11;
const int ENB = 10;

// -------------------- PARAMETERS --------------------
int BASE_SPEED   = 150;
int TURN_SPEED   = 150;
int SHARP_SPEED  = 180;
bool LINE_IS_BLACK = true;

bool INVERT_LEFT_MOTOR  = false;
bool INVERT_RIGHT_MOTOR = false;

// -------------------- SETUP --------------------
void setup() {
  pinMode(IR1, INPUT);
  pinMode(IR2, INPUT);
  pinMode(IR3, INPUT);
  pinMode(IR4, INPUT);
  pinMode(IR5, INPUT);
  pinMode(IR6, INPUT);
  pinMode(IR7, INPUT);
  pinMode(IR8, INPUT);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  Serial.begin(9600);
}

// -------------------- MAIN LOOP --------------------
void loop() {
  readSensors();
  bangBangControl();
}

// -------------------- READ SENSORS --------------------
void readSensors() {
  sensor[0] = digitalRead(IR1);
  sensor[1] = digitalRead(IR2);
  sensor[2] = digitalRead(IR3);
  sensor[3] = digitalRead(IR4);
  sensor[4] = digitalRead(IR5);
  sensor[5] = digitalRead(IR6);
  sensor[6] = digitalRead(IR7);
  sensor[7] = digitalRead(IR8);

  if (!LINE_IS_BLACK) {
    for (int i = 0; i < 8; i++) {
      sensor[i] = !sensor[i];
    }
  }
}

// -------------------- BANG BANG LOGIC --------------------
// Sensor layout: index 0 = Rightmost (IR1) ... index 7 = Leftmost (IR8)

int lastDirection = 0; // -1 = left, 1 = right, 0 = straight

void bangBangControl() {

  bool anyActive = false;
  for (int i = 0; i < 8; i++) {
    if (sensor[i] == HIGH) anyActive = true;
  }

  if (!anyActive) {
    if (lastDirection == -1)     turnLeft(SHARP_SPEED);
    else if (lastDirection == 1) turnRight(SHARP_SPEED);
    else                         stopMotors();
    return;
  }

  if (sensor[3] == HIGH || sensor[4] == HIGH) {
    moveForward(BASE_SPEED);
    lastDirection = 0;
    return;
  }

  if (sensor[0] == HIGH || sensor[1] == HIGH || sensor[2] == HIGH) {
    turnRight(TURN_SPEED);
    lastDirection = 1;
    return;
  }

  if (sensor[5] == HIGH || sensor[6] == HIGH || sensor[7] == HIGH) {
    turnLeft(TURN_SPEED);
    lastDirection = -1;
    return;
  }
}

// -------------------- MOTOR CONTROL --------------------

void moveForward(int speed) {
  setLeftMotor(speed, true);
  setRightMotor(speed, true);
}

void turnRight(int speed) {
  setLeftMotor(speed, true);
  setRightMotor((int)(speed * 0.3f), false);  // FIX: explicit cast prevents silent float truncation
}

void turnLeft(int speed) {
  setLeftMotor((int)(speed * 0.3f), false);   // FIX: explicit cast prevents silent float truncation
  setRightMotor(speed, true);
}

void stopMotors() {
  setLeftMotor(0, true);
  setRightMotor(0, true);
}

// -------------------- LOW LEVEL MOTOR DRIVE --------------------

void setLeftMotor(int speed, bool forward) {
  if (INVERT_LEFT_MOTOR) forward = !forward;
  speed = constrain(speed, 0, 255);

  if (forward) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
  }
  analogWrite(ENA, speed);
}

void setRightMotor(int speed, bool forward) {
  if (INVERT_RIGHT_MOTOR) forward = !forward;
  speed = constrain(speed, 0, 255);

  if (forward) {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  } else {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
  }
  analogWrite(ENB, speed);
}