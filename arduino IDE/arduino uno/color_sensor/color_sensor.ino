// TCS3200 COLOR SENSOR CODE
// Reads RGB intensity values (0–255) using pulseIn()

// Define the pins connected to the sensor
#define S0_PIN 4
#define S1_PIN 5
#define S2_PIN 6
#define S3_PIN 7
#define OUT_PIN 8

// Variables to store the RGB values
int redValue = 0;
int greenValue = 0;
int blueValue = 0;

void setup() {
  Serial.begin(9600);

  pinMode(S0_PIN, OUTPUT);
  pinMode(S1_PIN, OUTPUT);
  pinMode(S2_PIN, OUTPUT);
  pinMode(S3_PIN, OUTPUT);
  pinMode(OUT_PIN, INPUT);

  // Set output frequency scaling to 20%
  digitalWrite(S0_PIN, HIGH);
  digitalWrite(S1_PIN, LOW);

  Serial.println("TCS3200 sensor initialized. Place an object in front of the sensor.");
}

void loop() {
  redValue   = readRed();
  greenValue = readGreen();
  blueValue  = readBlue();

  Serial.print("R: "); Serial.print(redValue);
  Serial.print("\tG: "); Serial.print(greenValue);
  Serial.print("\tB: "); Serial.print(blueValue);
  if (greenValue < 200 && redValue < 200 && blueValue < 200) { 
    Serial.println(" Black or Gray");
  } else if (blueValue > greenValue && blueValue > redValue ) { 
    Serial.println(" Blue"); 
  } else if (greenValue > redValue && greenValue > blueValue ) { 
    Serial.println(" Green");
  } else if (redValue >> greenValue && redValue >> blueValue) { 
    Serial.println(" Red or Orange");
  } else {
    Serial.println(" Error, Sensor Not Connected");
  }
  
  delay(500); // half-second between readings
}

// --- Helper Functions ---
int mapPulseToIntensity(unsigned long pulse) {
  // Adjust these based on your sensor and environment
  const unsigned long minPW = 25;   // pulse width for bright
  const unsigned long maxPW = 300;  // pulse width for dark

  if (pulse == 0) return 0; // timeout -> no light detected
  long v = map((long)pulse, (long)minPW, (long)maxPW, 255, 0);
  return constrain((int)v, 0, 255);
}

int readColor(int s2State, int s3State) {
  digitalWrite(S2_PIN, s2State);
  digitalWrite(S3_PIN, s3State);
  delayMicroseconds(200); // allow filter to settle

  unsigned long lowPW = pulseIn(OUT_PIN, LOW, 100000); // µs
  return mapPulseToIntensity(lowPW);
}

int readRed()   { return readColor(LOW, LOW);  }   // Red filter
int readGreen() { return readColor(HIGH, HIGH);}   // Green filter
int readBlue()  { return readColor(LOW, HIGH); }   // Blue filter
