#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- BITMAP LOGOS ---
// 24x24 Spotify Icon for the Tab Menu
const unsigned char PROGMEM menu_spotify_icon [] = {
  0x00, 0xff, 0x00, 0x03, 0xff, 0xc0, 0x07, 0xff, 0xf0, 0x1f, 0xff, 0xf8,
  0x1f, 0xff, 0xfc, 0x3f, 0xff, 0xfc, 0x7f, 0xff, 0xfe, 0x70, 0x00, 0x7e,
  0xf0, 0x00, 0x1f, 0xff, 0xff, 0x87, 0xff, 0xff, 0xef, 0xf8, 0x01, 0xff,
  0xf9, 0xf8, 0x3f, 0xff, 0xff, 0x1f, 0xff, 0xff, 0xff, 0xf8, 0x01, 0xff,
  0x7f, 0xfc, 0x7e, 0x7f, 0xff, 0xfe, 0x3f, 0xff, 0xfc, 0x1f, 0xff, 0xf8,
  0x1f, 0xff, 0xf8, 0x07, 0xff, 0xe0, 0x03, 0xff, 0xc0, 0x00, 0xff, 0x00
};

// 24x24 Analog Clock Icon for the Tab Menu
const unsigned char PROGMEM menu_clock_icon [] = {
  0x00, 0x7f, 0x80, 0x03, 0xff, 0xe0, 0x0f, 0x00, 0xf0, 0x1c, 0x00, 0x38, 
  0x38, 0x00, 0x1c, 0x70, 0x10, 0x0e, 0x60, 0x10, 0x06, 0xe0, 0x10, 0x07, 
  0xc0, 0x10, 0x03, 0xc0, 0x10, 0x03, 0xc0, 0x10, 0x03, 0xc0, 0x10, 0x03, 
  0xc0, 0x1f, 0x03, 0xe0, 0x00, 0x07, 0x60, 0x00, 0x06, 0x70, 0x00, 0x0e, 
  0x38, 0x00, 0x1c, 0x1c, 0x00, 0x38, 0x0f, 0x00, 0xf0, 0x03, 0xff, 0xe0, 
  0x00, 0x7f, 0x80, 0x00, 0x00, 0x00
};

// 8x8 Play Icon
const unsigned char PROGMEM play_icon [] = {
  0x00, 0x08, 0x0c, 0x0e, 0x0f, 0x0e, 0x0c, 0x08
};

// 8x8 Pause Icon
const unsigned char PROGMEM pause_icon [] = {
  0x00, 0x1b, 0x1b, 0x1b, 0x1b, 0x1b, 0x1b, 0x00
};

// Helper function to print perfectly centered text
void printCenteredText(String text, int y, int size) {
  int16_t x1, y1;
  uint16_t w, h;
  display.setTextSize(size);
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  
  // Calculate X position so width is perfectly centered on screen
  int x = (SCREEN_WIDTH - w) / 2;
  display.setCursor(x, y);
  display.print(text);
}


void setup() {
  Serial.begin(115200);

  if(!display.begin(0x3C, true)) {
    Serial.println(F("SH1106 allocation failed"));
    for(;;);
  }
  
  // High constrast startup screen
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  printCenteredText("Booting...", 28, 1);
  display.display();
}

void renderSpotifyTab(String data) {
  // Find the pipes that separate our data
  int p1 = data.indexOf('|');
  int p2 = data.indexOf('|', p1 + 1);
  int p3 = data.indexOf('|', p2 + 1);
  int p4 = data.indexOf('|', p3 + 1);
  
  if (p4 != -1) {
    String song = data.substring(0, p1);
    String artist = data.substring(p1 + 1, p2);
    long progress = data.substring(p2 + 1, p3).toInt();
    long duration = data.substring(p3 + 1, p4).toInt();
    String status = data.substring(p4 + 1);
    
    display.clearDisplay();
    
    // 1. Draw Text perfectly spaced in upper half
    printCenteredText(song, 14, 1);
    printCenteredText(artist, 28, 1);
    
    // 2. Draw Progress Bar at the lower-middle
    int barWidth = 100;
    int barHeight = 4;
    int barX = (SCREEN_WIDTH - barWidth) / 2;
    int barY = 44; 
    
    display.drawRect(barX, barY, barWidth, barHeight, SH110X_WHITE); // Outline
    
    if (duration > 0 && progress >= 0) {
      int fillWidth = (progress * barWidth) / duration;
      if (fillWidth > barWidth) fillWidth = barWidth; 
      display.fillRect(barX, barY, fillWidth, barHeight, SH110X_WHITE); // Inner fill
    }
    
    // 3. Draw Play/Pause Icon under the progress bar center
    int iconX = 60;
    int iconY = 52;
    
    if (status == "PLAYING") {
      display.drawBitmap(iconX, iconY, play_icon, 8, 8, SH110X_WHITE);
    } else {
      display.drawBitmap(iconX, iconY, pause_icon, 8, 8, SH110X_WHITE);
    }

    display.display();
  }
}

void renderClockTab(String data) {
  // Data format from python: "12:05:03|Friday, 04 April"
  int p1 = data.indexOf('|');
  
  if (p1 != -1) {
    String timeStr = data.substring(0, p1);
    String dateStr = data.substring(p1 + 1);
    
    display.clearDisplay();
    
    // Time format in large font, right in the center!
    printCenteredText(timeStr, 20, 2);
    
    // Date format in smaller font directly below it
    printCenteredText(dateStr, 45, 1);

    display.display();
  }
}

void renderMenuTab(String tabName) {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  
  // Draw giant < and > arrows on the edges
  display.setTextSize(2);
  display.setCursor(4, 24);
  display.print("<");
  
  display.setCursor(110, 24);
  display.print(">");
  
  // Draw the app icon in the center!
  if (tabName == "Spotify") {
    display.drawBitmap(52, 12, menu_spotify_icon, 24, 24, SH110X_WHITE);
  } else {
    display.drawBitmap(52, 12, menu_clock_icon, 24, 24, SH110X_WHITE);
  }
  
  // Draw the app title underneath the icon
  printCenteredText(tabName, 44, 1);
  
  display.display();
}

void loop() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.length() > 2) {
      char tab_id = command.charAt(0);
      String payload = command.substring(2); // Skip prefix
      
      if (tab_id == 'M') {
        renderMenuTab(payload);
      }
      else if (tab_id == 'S') {
        renderSpotifyTab(payload);
      } 
      else if (tab_id == 'C') {
        renderClockTab(payload);
      }
      else {
        // Fallback for errors
        display.clearDisplay();
        printCenteredText(command, 30, 1);
        display.display();
      }
    }
  }
}
