#include <Adafruit_GFX.h>    
#include <Adafruit_ST7735.h> 
#include <WiFi.h>
#include "image.h"

// --- YOUR WORKING R4TKN PINOUT ---
#define TFT_SCL    PA14
#define TFT_SDA    PA12
#define TFT_RST    PA26
#define TFT_DC     PA25
#define TFT_CS     PA27
#define TFT_BLK    PA30

#define BTN_UP     PB1
#define BTN_DOWN   PB2
#define BTN_SEL    PB3

// USE THE 5-PIN CONSTRUCTOR AGAIN (This fixed the white screen)
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_SDA, TFT_SCL, TFT_RST);

enum State { MENU, ABOUT, SCANNER };
State currentState = MENU;
int selectedItem = 0;
bool refreshNeeded = true; // The trick for speed

void setup() {
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_SEL, INPUT_PULLUP);
  
  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, HIGH); 

  tft.initR(INITR_BLACKTAB); 
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);

  // Logo Splash
  tft.drawRGBBitmap(0, 0, keju_logo, 128, 160);
  delay(2000);
}

void loop() {
  if (currentState == MENU) {
    if (digitalRead(BTN_UP) == LOW) {
      selectedItem = (selectedItem <= 0) ? 2 : selectedItem - 1;
      refreshNeeded = true;
      delay(150); 
    }
    if (digitalRead(BTN_DOWN) == LOW) {
      selectedItem = (selectedItem >= 2) ? 0 : selectedItem + 1;
      refreshNeeded = true;
      delay(150);
    }
    
    // ONLY redraw if a button was actually pressed
    if (refreshNeeded) {
      drawMenu();
      refreshNeeded = false; 
    }

    if (digitalRead(BTN_SEL) == LOW) {
      if (selectedItem == 0) { currentState = ABOUT; drawAbout(); }
      else if (selectedItem == 1) { currentState = SCANNER; startScanner(); }
      else { NVIC_SystemReset(); }
      delay(200);
      refreshNeeded = true; 
    }
  }
}  

void drawMenu() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(20, 10);
  tft.print("BW-BOX");
  tft.drawFastHLine(0, 32, 128, 0x07E0);

  drawItem("ABOUT", 55, selectedItem == 0);
  drawItem("SCANNER", 85, selectedItem == 1);
  drawItem("REBOOT", 115, selectedItem == 2);
}

void drawItem(const char* text, int y, bool sel) {
  if (sel) {
    tft.fillRect(5, y - 5, 118, 25, 0x781F); 
    tft.setTextColor(ST77XX_WHITE);
  } else {
    tft.setTextColor(0x7BEF); 
  }
  tft.setCursor(25, y);
  tft.setTextSize(2);
  tft.print(text);
}

void drawAbout() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(0x05FF);
  tft.setCursor(10, 20);
  tft.setTextSize(2);
  tft.print("KEJU OS");
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, 60);
  tft.println("Status: Online");
  tft.println("Driver: Soft-SPI");
  tft.setCursor(10, 140);
  tft.print("Press SEL to back");
}

void startScanner() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(0x07E0); // Matrix Green
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.print("SCANNING...");

  // Scan WiFi
  int n = WiFi.scanNetworks();
  
  tft.fillScreen(ST77XX_BLACK); 
  tft.setCursor(0, 0);
  tft.setTextSize(1);
  tft.setTextColor(0x05FF); // Neon Blue
  tft.println("  SSID          RSSI");
  tft.drawFastHLine(0, 10, 128, 0x7BEF);
  tft.println("");

  if (n == 0) {
    tft.setTextColor(ST77XX_RED);
    tft.println(" No networks found.");
  } else {
    // Show up to 10 networks
    for (int i = 0; i < min(n, 10); i++) {
      tft.setCursor(5, 20 + (i * 12));
      
      // FIX: Convert char* to String so we can use substring
      String ssidName = String(WiFi.SSID(i));
      if(ssidName.length() > 12) ssidName = ssidName.substring(0, 12);
      
      tft.setTextColor(ST77XX_WHITE);
      tft.print(ssidName);

      // Draw simple signal bars based on RSSI
      long rssi = WiFi.RSSI(i);
      uint16_t barColor = (rssi > -60) ? 0x07E0 : (rssi > -80) ? 0xFFE0 : 0xF800;
      int barWidth = map(rssi, -100, -30, 1, 20);
      
      tft.fillRect(95, 22 + (i * 12), barWidth, 4, barColor);
    }
  }
  
  tft.setTextColor(0x781F); // Purple
  tft.setCursor(10, 150);
  tft.print("Press SEL to Exit");
}
