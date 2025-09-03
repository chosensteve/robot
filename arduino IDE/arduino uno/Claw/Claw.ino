#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

int servoMin = 150; // adjust for your servo
int servoMax = 600;
int servoClaw = 3; // slot labeled "3"
int servoBase = 0;
int servoArm = 2;
int servoShd = 1;

void setup() {
  Serial.begin(9600);
  pwm.begin();
  pwm.setPWMFreq(50);  // 50 Hz for servos
  delay(10);
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n'); // read until Enter
    int spaceIndex = input.indexOf(' ');
    String motorStr = input.substring(0, spaceIndex);
    String angleStr = input.substring(spaceIndex + 1);
    input.trim(); // remove whitespace/newlines
    int motor = motorStr.toInt();
    int angle = angleStr.toInt();

    if (angle >= 0 && angle <= 180) {
      int pulselen = map(angle, 0, 180, servoMin, servoMax);
      Serial.println(pulselen);
      pwm.setPWM(motor, 0, pulselen);
    }
    else {
      Serial.println("\nPlease enter a number between 0-180.");
    }

    Serial.println("Enter a value (0-180):");
  }
}
