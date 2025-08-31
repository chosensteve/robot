#include <Servo.h>

Servo servo1;
int servopin = 9;

void setup() {
  servo1.attach(servopin);
  Serial.begin(9600);
  while (!Serial); // wait for Serial to initialize
  Serial.println("Enter a value (0-180):");
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n'); // read until Enter
    input.trim();                                 // remove whitespace/newlines
    int angle = input.toInt();

    if (angle >= 0 && angle <= 180) {
      servo1.write(angle);
      Serial.print("Moving servo to: ");
      Serial.println(angle);
    } else {
      Serial.println("Please enter a number between 0-180.");
    }

    Serial.println("Enter a value (0-180):");
  }
}