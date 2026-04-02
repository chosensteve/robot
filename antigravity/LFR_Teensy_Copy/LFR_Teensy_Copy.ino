const int muxS0 = 2;
const int muxS1 = 3;
const int muxS2 = 4;
const int muxS3 = 5;
const int muxSig = A0;

const int motorLeftPWM = 6;
const int motorLeftDir1 = 7;
const int motorLeftDir2 = 8;
const int motorRightPWM = 9;
const int motorRightDir1 = 10;
const int motorRightDir2 = 11;

float Kp = 1.2;
float Ki = 0.0;
float Kd = 0.5;

float P = 0, D = 0;
float I = 0;
float lastError = 0;
float PIDvalue = 0;

int sensorValues[16];
int baseSpeed = 100;

void setup() {
  Serial.begin(115200);
  
  analogReadResolution(12);
  
  pinMode(muxS0, OUTPUT);
  pinMode(muxS1, OUTPUT);
  pinMode(muxS2, OUTPUT);
  pinMode(muxS3, OUTPUT);
  pinMode(muxSig, INPUT);
  
  pinMode(motorLeftPWM, OUTPUT);
  pinMode(motorLeftDir1, OUTPUT);
  pinMode(motorLeftDir2, OUTPUT);
  pinMode(motorRightPWM, OUTPUT);
  pinMode(motorRightDir1, OUTPUT);
  pinMode(motorRightDir2, OUTPUT);

  digitalWrite(motorLeftDir1, HIGH);
  digitalWrite(motorLeftDir2, LOW);
  digitalWrite(motorRightDir1, HIGH);
  digitalWrite(motorRightDir2, LOW);
}

void readSensors() {
  for (int i = 0; i < 16; i++) {
    digitalWrite(muxS0, bitRead(i, 0));
    digitalWrite(muxS1, bitRead(i, 1));
    digitalWrite(muxS2, bitRead(i, 2));
    digitalWrite(muxS3, bitRead(i, 3));
    
    delayMicroseconds(20);
    
    int rawValue = analogRead(muxSig); 
    
    if (rawValue >= 2800 && rawValue <= 4000) { 
      sensorValues[i] = map(rawValue, 2800, 3900, 0, 1000);
      sensorValues[i] = constrain(sensorValues[i], 0, 1000);
    } else {
      sensorValues[i] = 0;
    }
  }
}

void loop() {
  readSensors();
  
  long leftData = 0;
  long centerData = 0;
  long rightData = 0;
  
  for(int i=0; i<5; i++) leftData += sensorValues[i];
  for(int i=5; i<11; i++) centerData += sensorValues[i];
  for(int i=11; i<16; i++) rightData += sensorValues[i];

  long weightedSum = 0;
  long sum = 0;
  
  for(int i = 0; i < 16; i++) {
    float weight = (i - 7.5) * 10.0; 
    weightedSum += sensorValues[i] * weight;
    sum += sensorValues[i];
  }
  
  float error = 0;
  if (sum > 0) {
    error = (float)weightedSum / sum;
  } else {
    error = (lastError > 0) ? 100 : -100;
  }
  
  P = error;
  I = I + error;
  D = error - lastError;
  lastError = error;
  
  PIDvalue = (Kp * P) + (Ki * I) + (Kd * D);

  int leftSpeed = baseSpeed + PIDvalue;
  int rightSpeed = baseSpeed - PIDvalue;

  leftSpeed = constrain(leftSpeed, -255, 255);
  rightSpeed = constrain(rightSpeed, -255, 255);
  
  setMotorSpeed(leftSpeed, rightSpeed);

  if (abs(error) > 15) { 
    if (error < 0) {
      Serial.print("TURN L | ");
    } else {
      Serial.print("TURN R | ");
    }
    Serial.print("P:"); Serial.print(P);
    Serial.print(" I:"); Serial.print(I);
    Serial.print(" D:"); Serial.println(D);
  }
  
  delay(5);
}

void setMotorSpeed(int left, int right) {
  if (left >= 0) {
    digitalWrite(motorLeftDir1, HIGH);
    digitalWrite(motorLeftDir2, LOW);
    analogWrite(motorLeftPWM, left);
  } else {
    digitalWrite(motorLeftDir1, LOW);
    digitalWrite(motorLeftDir2, HIGH);
    analogWrite(motorLeftPWM, -left);
  }
  
  if (right >= 0) {
    digitalWrite(motorRightDir1, HIGH);
    digitalWrite(motorRightDir2, LOW);
    analogWrite(motorRightPWM, right);
  } else {
    digitalWrite(motorRightDir1, LOW);
    digitalWrite(motorRightDir2, HIGH);
    analogWrite(motorRightPWM, -right);
  }
}
