// ===== PIN DEFINITIONS =====
#define S0 25
#define S1 26
#define S2 27
#define S3 14
#define SIG 34   // Z_OP connected here

#define NUM_SENSORS 16
#define SAMPLES 5          // Small averaging for stability
#define SETTLE_TIME 10      // microseconds

void setup() {
  Serial.begin(115200);

  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);

  analogSetWidth(12);                  // 0–4095 range
  analogSetAttenuation(ADC_11db);      // Full 3.3V range

  Serial.println("ByteSense 16CH Raw Readings");
  Serial.println("--------------------------------------------");
}


// Fast raw read with small averaging
int readChannel(int ch) {

  digitalWrite(S0, ch & 1);
  digitalWrite(S1, (ch >> 1) & 1);
  digitalWrite(S2, (ch >> 2) & 1);
  digitalWrite(S3, (ch >> 3) & 1);

  delayMicroseconds(SETTLE_TIME);

  int sum = 0;
  for (int i = 0; i < SAMPLES; i++) {
    sum += analogRead(SIG);
  }

  return sum / SAMPLES;
}


void loop() {

  for (int i = 0; i < NUM_SENSORS; i++) {
    int value = readChannel(i);

    // Clean aligned formatting
  // Keep alignment
    Serial.print(value);
    Serial.print("  ");
  }

  Serial.println();
  delay(100);
}
