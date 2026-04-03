void setup() {
  // Initialize serial communication at 115200 bits per second:
  // (115200 is the standard baud rate for ESP32 boards)
  Serial.begin(115200);
  
  // Wait for 1 second to give the Serial Monitor time to initialize
  delay(1000);
  
  // Print an initial ready message
  Serial.println("ESP32 Test Sketch Started!");
}

void loop() {
  // Print a hello message every second to verify it's running
  Serial.println("Hello World! The ESP32 is working.");
  
  // Wait for 1000 milliseconds (1 second)
  delay(1000);
}
