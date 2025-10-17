#include "BluetoothSerial.h"

// Check if a BluetoothSerial object can be created
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error "Bluetooth is not enabled! Please run `make menuconfig` to and enable it"
#endif

BluetoothSerial SerialBT;

// --- Motor Pin Definitions (CHANGE THESE TO MATCH YOUR WIRING) ---
// Define pins for one side (e.g., Left Motor)
const int IN1 = 25; // Motor Driver Pin IN1
const int IN2 = 26; // Motor Driver Pin IN2

// Define pins for the other side (e.g., Right Motor)
const int IN3 = 32; // Motor Driver Pin IN3
const int IN4 = 33; // Motor Driver Pin IN4

// Variable to hold the incoming Bluetooth data
char command;

void setup() {
    // Initialize serial for debugging (optional)
    Serial.begin(115200);

    // Start Bluetooth Serial 
    SerialBT.begin("ESP32_Robot_Car"); 
    Serial.println("Bluetooth Started! Ready to pair.");

    // --- Motor Pin Setup ---
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(ENA, OUTPUT);
    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);
    pinMode(ENB, OUTPUT);
    
    // Initial state: STOP
    stopCar(); 
}

void loop() {
    // Check if data is available from the Bluetooth device
    if (SerialBT.available()) {
        command = SerialBT.read();
        Serial.print("Received Command: ");
        Serial.println(command);

        // Process the command
        switch (command) {
            case 'F': // Forward
              Serial.print("F");
                moveForward();
                break;
            case 'B': // Backward
              Serial.print("B");
                moveBackward();
                break;
            case 'L': // Turn Left
                Serial.print("L");
                turnLeft();
                break;
            case 'R': // Turn Right
                Serial.print("R");
                turnRight();
                break;
            case 'S': // Stop
              Serial.print("S");
                stopCar();
                break;
            // Add more cases for specific buttons/commands if needed
            default:
                // Handle unknown command or do nothing
                break;
        }
    }
}

// --- Car Movement Functions ---

void moveForward() {
    // Left Motor: Forward
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    // Right Motor: Forward
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
}

void moveBackward() {
    // Left Motor: Backward
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    // Right Motor: Backward
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
}

void turnLeft() {
    // Left Motor: Stop or Backward (for sharp turn)
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH); // Stopping one side
    // Right Motor: Forward
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
}

void turnRight() {
    // Left Motor: Forward
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    // Right Motor: Stop or Backward (for sharp turn)
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH); // Stopping one side
}

void stopCar() {
    // Apply brakes (both IN pins LOW or HIGH on L298N for 'fast stop' if using 'analogWrite' for ENA/ENB)
    // For simplicity, setting both sides to LOW
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
}