#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// Servo setup
int servoMin = 150;
int servoMax = 600;

// Motor driver pins
int IN1 = 9;
int IN2 = 10;
int IN3 = 11;
int IN4 = 12;

void setup() {
  Serial.begin(9600);

  // Servo setup
  pwm.begin();
  pwm.setPWMFreq(50);
  delay(10);

  // Motor setup
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Start with motors OFF
  stopMotors();

  Serial.println("System ready.");
  Serial.println("Enter: '0-3 angle' for claw, 'WASD duration' for movement, or 'default' for claw reset.");
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    // 🔹 Handle keyword commands first
    if (input.equalsIgnoreCase("default")) {
      clawDefault();
      Serial.println("Claw reset to default position.");
      return;
    }

    // Future expansion: add more keyword commands
    // if (input.equalsIgnoreCase("wave")) {
    //   clawWave();
    //   return;
    // }

    int spaceIndex = input.indexOf(' ');
    if (spaceIndex == -1) {
      Serial.println("Invalid format. Use: '0 angle' or 'w 1000'.");
      return;
    }

    String first = input.substring(0, spaceIndex);
    String second = input.substring(spaceIndex + 1);

    // If the first part is a digit → Servo control
    if (isDigit(first.charAt(0))) {
      int channel = first.toInt();
      int angle = second.toInt();

      if (channel >= 0 && channel <= 3 && angle >= 0 && angle <= 180) {
        clawMove(channel, angle);
        Serial.print("Servo "); Serial.print(channel);
        Serial.print(" -> "); Serial.println(angle);
      } else {
        Serial.println("Servo input invalid. Use channel 0–3, angle 0–180.");
      }
    } 
    else {
      // Otherwise → Movement
      char dir = first.charAt(0);
      int duration = second.toInt();

      if (duration <= 0) {
        Serial.println("Duration must be > 0 ms.");
        return;
      }

      if (dir == 'w' || dir == 'W') forward();
      else if (dir == 's' || dir == 'S') backward();
      else if (dir == 'a' || dir == 'A') left();
      else if (dir == 'd' || dir == 'D') right();
      else if (dir == 'x' || dir == 'X') stopMotors();
      else {
        Serial.println("Use W/A/S/D/X for movement.");
        return;
      }

      delay(duration);
      stopMotors();
      Serial.print("Moved "); Serial.print(dir);
      Serial.print(" for "); Serial.print(duration);
      Serial.println(" ms");
    }
  }
}

// Motor functions
void backward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void forward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void right() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void left() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

// Claw functions
void clawMove(int channel, int angle) {
  int pulselen = map(angle, 0, 180, servoMin, servoMax);
  pwm.setPWM(channel, 0, pulselen);  
}

void clawDefault() {
  clawMove(2, 165);
  clawMove(1, 90);
  clawMove(0, 90);
  clawMove(3, 50);
}
