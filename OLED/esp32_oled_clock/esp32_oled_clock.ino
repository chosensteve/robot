#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <WiFi.h>
#include <time.h>

// OLED display size in pixels
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an ESP32: SDA = GPIO 21, SCL = GPIO 22
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ===== WIFI SETTINGS =====
const char* ssid       = "Airtel_DevMadan";     // Enter your Wi-Fi SSID
const char* password   = "9811785935"; // Enter your Wi-Fi Password

// ===== TIME SETTINGS =====
const char* ntpServer = "pool.ntp.org";
// I noticed your local time zone is +05:30 (India Standard Time). 
// 5 hours and 30 minutes = 19800 seconds
const long  gmtOffset_sec = 19800;  
const int   daylightOffset_sec = 0; // No daylight saving time in standard IST

void setup() {
  Serial.begin(115200);

  // Initialize the OLED object
  // Address 0x3C is standard for 128x64 or 128x32 OLEDs
  if(!display.begin(0x3C, true)) {
    Serial.println(F("SH1106 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  
  // Clear the buffer
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Booting up...");
  display.display();

  // Connect to Wi-Fi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  display.println("Connecting to Wi-Fi:");
  display.println(ssid);
  display.display();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected.");
  display.println("Connected!");
  display.display();
  delay(1000);

  // Initialize time from NTP server
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void loop() {
  struct tm timeinfo;
  
  // Clear the display for the new time update
  display.clearDisplay();
  
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println("Waiting for Time...");
    display.display();
    delay(1000);
    return;
  }

  // Format time as HH:MM:SS
  char timeStr[10];
  strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
  
  // Format Date as DD/MM/YYYY
  char dateStr[20];
  strftime(dateStr, sizeof(dateStr), "%d %b %Y", &timeinfo);

  // Display the time on the OLED (Large font)
  display.setTextSize(2); // Large font
  
  // Center the time somewhat mathematically
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(timeStr, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 20); 
  display.println(timeStr);

  // Display the date on the OLED (Smaller font)
  display.setTextSize(1); // Normal font
  display.getTextBounds(dateStr, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 45);
  display.println(dateStr);

  // Push everything to the screen
  display.display();
  
  // Wait a bit before checking time again
  delay(1000);
}
