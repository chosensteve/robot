/*
 * Teensy 4.1 Line Following Robot using Bytsense v1.1 (16ch Mux)
 */

// Mux control pins (Adjust to match your wiring)
const int muxS0 = 2; // Digital out
const int muxS1 = 3; // Digital out
const int muxS2 = 4; // Digital out
const int muxS3 = 5; // Digital out
const int muxSig = A0; // Analog in from MUX SIG

// Motor control pins (Example, adjust to your motor driver like TB6612, L298N, etc.)
const int motorLeftPWM = 6;
const int motorLeftDir1 = 7;
const int motorLeftDir2 = 8;
const int motorRightPWM = 9;
const int motorRightDir1 = 10;
const int motorRightDir2 = 11;

// PID Constants (Tuning required)
float Kp = 1.2;
float Ki = 0.0;
float Kd = 0.5;

float P = 0, D = 0;
float I = 0;
float lastError = 0;
float PIDvalue = 0;

// Sensor arrays and thresholds
int sensorValues[16];
int baseSpeed = 100; // Base speed for the motors (0-255)

void setup() {
  Serial.begin(115200);
  
  // Set ADC resolution to 12-bit (0-4095) since sensor values go up to 4000
  analogReadResolution(12);
  
  // Set mux select pins as outputs
  pinMode(muxS0, OUTPUT);
  pinMode(muxS1, OUTPUT);
  pinMode(muxS2, OUTPUT);
  pinMode(muxS3, OUTPUT);
  pinMode(muxSig, INPUT); // Analog read pin
  
  // Set motor pins as outputs
  pinMode(motorLeftPWM, OUTPUT);
  pinMode(motorLeftDir1, OUTPUT);
  pinMode(motorLeftDir2, OUTPUT);
  pinMode(motorRightPWM, OUTPUT);
  pinMode(motorRightDir1, OUTPUT);
  pinMode(motorRightDir2, OUTPUT);

  // Set initial motor directions (forward)
  digitalWrite(motorLeftDir1, HIGH);
  digitalWrite(motorLeftDir2, LOW);
  digitalWrite(motorRightDir1, HIGH);
  digitalWrite(motorRightDir2, LOW);
}

void readSensors() {
  for (int i = 0; i < 16; i++) {
    // Select the channel using the S0-S3 bits
    digitalWrite(muxS0, bitRead(i, 0));
    digitalWrite(muxS1, bitRead(i, 1));
    digitalWrite(muxS2, bitRead(i, 2));
    digitalWrite(muxS3, bitRead(i, 3));
    
    delayMicroseconds(10);    // Very short delay allowing mux signal to settle
    delayMicroseconds(10); 
    
    // Read the 12-bit analog signal
    int rawValue = analogRead(muxSig); 
    
    // Apply user thresholds:
    // White is normally 1500 - 2500
    // Black is normally 2800 - 3900
    // "Otherwise they are false" -> we zero them out
    if (rawValue >= 2800 && rawValue <= 4000) { 
      // It's a black line! Map 2800-3900 to a continuous 0-1000 scale for smoother PID math
      sensorValues[i] = map(rawValue, 2800, 3900, 0, 1000);
      sensorValues[i] = constrain(sensorValues[i], 0, 1000);
    } else {
      // It is white (1500-2500), noise, or out of normal bounds -> treat as false (0)
      sensorValues[i] = 0;
    }
  }
}

void loop() {
  readSensors();
  
  // Group sensor values: Left (0-4), Center (5-10), Right (11-15)
  long leftData = 0;
  long centerData = 0;
  long rightData = 0;
  
  for(int i=0; i<5; i++) leftData += sensorValues[i];
  for(int i=5; i<11; i++) centerData += sensorValues[i];
  for(int i=11; i<16; i++) rightData += sensorValues[i];

  // Calculate Weighted Position for PID (Center is 0)
  // Assuming dark line on light background (high values = on line)
  long weightedSum = 0;
  long sum = 0;
  
  for(int i = 0; i < 16; i++) {
    // Weight multiplier ranges from roughly -75 (far left) to +75 (far right)
    float weight = (i - 7.5) * 10.0; 
    weightedSum += sensorValues[i] * weight;
    sum += sensorValues[i];
  }
  
  float error = 0;
  // Prevent division by zero if line is completely lost
  if (sum > 0) {
    error = (float)weightedSum / sum;
  } else {
    // If line lost completely, maintain previous turn logic (memory)
    error = (lastError > 0) ? 100 : -100;
  }
  
  // PID Calculation
  P = error;
  I = I + error;
  D = error - lastError;
  lastError = error;
  
  PIDvalue = (Kp * P) + (Ki * I) + (Kd * D);

  // Motor Control Application
  int leftSpeed = baseSpeed + PIDvalue;
  int rightSpeed = baseSpeed - PIDvalue;

  // Constrain to valid 8-bit PWM values
  leftSpeed = constrain(leftSpeed, -255, 255);
  rightSpeed = constrain(rightSpeed, -255, 255);
  
  // Apply speeds to motors
  setMotorSpeed(leftSpeed, rightSpeed);

  // Print diagnostics when the robot is turning (e.g., error threshold crossed)
  if (abs(error) > 15) { 
    Serial.print("TURNING -> L: "); 
    Serial.print(leftData);
    Serial.print(" | C: "); 
    Serial.print(centerData);
    Serial.print(" | R: "); 
    Serial.print(rightData);
    Serial.print(" || PID(P: "); 
    Serial.print(P);
    Serial.print(", I: "); 
    Serial.print(I);
    Serial.print(", D: "); 
    Serial.print(D);
    Serial.print(") Out: "); 
    Serial.println(PIDvalue);
  }
  
  // Teensy 4.1 runs incredibly fast, add a slight loop delay to stabilize I & D
  delay(5);
}

void setMotorSpeed(int left, int right) {
  // Handle Left Motor direction & PWM
  if (left >= 0) {
    digitalWrite(motorLeftDir1, HIGH);
    digitalWrite(motorLeftDir2, LOW);
    analogWrite(motorLeftPWM, left);
  } else {
    digitalWrite(motorLeftDir1, LOW);
    digitalWrite(motorLeftDir2, HIGH);
    analogWrite(motorLeftPWM, -left);
  }
  
  // Handle Right Motor direction & PWM
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
