const int led = 3;
const int trig = 10;
const int echo = 9;
long totaltime;
int distance;

void setup() {
  pinMode(trig, OUTPUT); 
  pinMode(echo, INPUT);
  pinMode(led, OUTPUT);
  Serial.begin(9600); 
}

void loop() {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  totaltime = pulseIn(echo, HIGH, 30000);

  if (totaltime > 0) {
    distance = totaltime * 0.034 / 2;
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
  } else {
    Serial.println("No echo received");
  }

  if (totaltime > 0 && distance < 100) {
    digitalWrite(led, HIGH);
  } else {
    digitalWrite(led, LOW);
  }

  delay(200);
}
