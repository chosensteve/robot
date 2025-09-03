const int irPin = 7;   // Sensor OUT connected to D7
const int ledPin = 13; // Built-in LED for indication

void setup() {
  pinMode(irPin, INPUT);    // Sensor as input
  pinMode(ledPin, OUTPUT);  // LED output
  Serial.begin(9600);       // Start serial monitor
}

void loop() {
  int sensorValue = digitalRead(irPin);

  if (sensorValue == LOW) { 
    // LOW usually means OBSTACLE detected
    Serial.println("Obstacle Detected!");
    digitalWrite(ledPin, HIGH); 
  } else {
    // HIGH means no obstacle
    Serial.println("Clear");
    digitalWrite(ledPin, LOW);
  }

  delay(200); // small delay to avoid spamming
}
