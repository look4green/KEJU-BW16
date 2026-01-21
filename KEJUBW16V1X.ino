#include <Adafruit_GFX.h>    
#include <Adafruit_ST7735.h> 
#include <WiFi.h>

// Pins for R4TKN BW16 - Using digital indices for stability
#define TFT_SCL   14 // PA14
#define TFT_SDA   12 // PA12
#define TFT_RST   26 // PA26
#define TFT_DC    25 // PA25
#define TFT_CS    27 // PA27
#define TFT_BLK   30 // PA30
#define BUZZER_PIN 19 // PA19

#define BTN_UP    PB1
#define BTN_DOWN  PB2
#define BTN_SEL   PB3

#define ST77XX_GREY 0x7BEF

#define CYBER_PURPLE 0x781F
#define CYBER_DARK   0x0841 // For subtle backgrounds
#define CYBER_BLUE   0x05FF // Neon Blue
#define CYBER_GREEN  0x07E0 // Bright Matrix Green
#define CYBER_RED    0xF800 // Alert Red
#define CYBER_GREY   0x2104 // Dark background grey
#define CYBER_WHITE  0xFFFF


long signalHistory[160]; // Use 'long' to handle negative RSSI values

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_SDA, TFT_SCL, TFT_RST);

// 1. Define dimensions based on your hex data (usually 128x64 for this format)
#define LOGO_WIDTH  128
#define LOGO_HEIGHT 64

// 2. Put the bitmap in PROGMEM to save RAM
const unsigned char logo_bmp[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  // ... (all the hex you pasted goes here) ...
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

float batteryVolts = 0.0;
bool attackActive = false;
int attackType = 0; // 0: None, 1: Beacon, 2: Deauth (Scan-based)

struct WifiLog {
  String ssid;
  int rssi;
  int channel;
};

WifiLog networkLogs[10]; // Stores the last 10 networks
int logCount = 0;

int cursor = 0;

bool stealthMode = false;
String menuItems[] = {
  "Scan Networks", 
  "Audio Tracker", 
  "View Logs", 
  "Infiltration Mode", // New name for runAttack
  "Filter: ALL", 
  "Stealth: OFF", 
  "Device Info", 
  "Reboot"
};
const int totalItems = 8;
bool filter5G = false;

void drawMenu();
void startScan();
void packetMonitor();
void beaconSpam();
void handleSelection(int index);

void setup() {
  Serial.begin(115200);
  
  // Initialize Pins
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_SEL, INPUT_PULLUP);
  pinMode(TFT_BLK, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  digitalWrite(TFT_BLK, HIGH); 
  digitalWrite(BUZZER_PIN, LOW);

  // Initialize Screen
  tft.initR(INITR_BLACKTAB); 
  tft.setRotation(1); 
  
  // Play Boot Sequence
  showSplashScreen();
  
  drawMenu();
}

void loop() {
  bool changed = false;

  if (batteryVolts < 3.4 && batteryVolts > 1.0) { // 1.0 check avoids error if no battery
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(CYBER_RED);
  tft.setCursor(20, 60);
  tft.println("LOW BATTERY: SHUTDOWN");
  delay(5000);
  // Put BW16 into deep sleep to save battery
  // deep_sleep(); 
  }
  if(digitalRead(BTN_UP) == LOW) {
    cursor = (cursor > 0) ? cursor - 1 : totalItems - 1;
    changed = true;
    delay(200); // Simple debounce
  }
  if(digitalRead(BTN_DOWN) == LOW) {
    cursor = (cursor < totalItems - 1) ? cursor + 1 : 0;
    changed = true;
    delay(200);
  }
  if(digitalRead(BTN_SEL) == LOW) {
    handleSelection(cursor);
    changed = true;
    delay(200);
  }

  if(changed) {
    drawMenu();
    // Audible "click" feedback
    digitalWrite(BUZZER_PIN, HIGH);
    delay(5);
    digitalWrite(BUZZER_PIN, LOW);
  }
}

void drawMenu() {
  menuItems[4] = filter5G ? "MODE: 5GHz ONLY" : "MODE: DUAL-BAND";
  menuItems[5] = stealthMode ? "POWER: STEALTH" : "POWER: NORMAL";

  tft.fillScreen(ST77XX_BLACK);
  
  // 1. Top Header Bar
  tft.fillRect(0, 0, 160, 20, CYBER_BLUE);
  tft.setTextColor(ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setCursor(45, 6);
  tft.print("KEJU SYSTEM OS");

  drawBatteryIcon(); //

  // 2. Main Menu Body
  for(int i = 0; i < totalItems; i++) {
    int yPos = 28 + (i * 12);
    
    if(i == cursor) {
      // Draw a "Glow" box for the selected item
      tft.fillRoundRect(5, yPos - 2, 150, 11, 3, 0x0100); // Dark blue highlight
      tft.drawRoundRect(5, yPos - 2, 150, 11, 3, CYBER_BLUE);
      tft.setTextColor(CYBER_WHITE);
      
      // Selection pointer icon
      tft.setCursor(8, yPos);
      tft.print(">");
    } else {
      tft.setTextColor(0x7BEF); // Dim grey for inactive
    }
    
    tft.setCursor(18, yPos);
    tft.println(menuItems[i]);
  }

  // 3. Bottom Status Bar (Dashboard)
  tft.drawFastHLine(0, 118, 160, CYBER_BLUE);
  
  // Status Icon: 5G Filter
  tft.setTextSize(1);
  tft.setCursor(5, 121);
  if(filter5G) {
    tft.setTextColor(ST77XX_MAGENTA);
    tft.print("[5G]");
  } else {
    tft.setTextColor(CYBER_GREEN);
    tft.print("[2.4/5]");
  }

  // Status Icon: Stealth
  tft.setCursor(110, 121);
  if(stealthMode) {
    tft.setTextColor(CYBER_RED);
    tft.print("STEALTH");
  } else {
    tft.setTextColor(CYBER_BLUE);
    tft.print("ACTIVE");
  }
}
// 3. Your updated function
void showSplashScreen() {
  // PHASE 1: Logo Only
  tft.fillScreen(ST77XX_BLACK);
  int x = (160 - LOGO_WIDTH) / 2;
  int y = (128 - LOGO_HEIGHT) / 2;
  tft.drawBitmap(x, y, logo_bmp, LOGO_WIDTH, LOGO_HEIGHT, ST77XX_CYAN);
  delay(3000); // Hold for 3 seconds

  // PHASE 2: INITIALIZATION
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(CYBER_WHITE);
  tft.setCursor(45, 35);
  tft.setTextSize(2);
  tft.println("EVERLONG");
  
  tft.setTextSize(1);
  tft.setTextColor(CYBER_BLUE);
  tft.setCursor(30, 65);
  tft.println("SYSTEM INITIALIZED");
  tft.setCursor(30, 80);
  tft.println("RADIO_INIT: OK");

  // Loading bar animation
  for (int i = 0; i < 120; i++) {
    tft.drawFastHLine(20, 105, i, 0x07E0);
    if (i % 40 == 0) {
      digitalWrite(BUZZER_PIN, HIGH);
      delay(5);
      digitalWrite(BUZZER_PIN, LOW);
    }
    delay(15); // Total text phase lasts ~1.8 seconds
  }
  delay(500);
}

void handleSelection(int index) {
  if(index == 0)      startScan();
  else if(index == 1) packetMonitor();
  else if(index == 2) showLogs();
  else if(index == 3) beaconSpam();
  else if(index == 4) { // 5G Filter Toggle
    filter5G = !filter5G;
    menuItems[4] = filter5G ? "Filter: ONLY 5G" : "Filter: ALL BANDS";
    beepConfirm();
  }
  else if(index == 5) { // Stealth Mode Toggle
    stealthMode = !stealthMode;
    menuItems[5] = stealthMode ? "Stealth: ON" : "Stealth: OFF";
    if(stealthMode) analogWrite(TFT_BLK, 10); // Very dim
    else analogWrite(TFT_BLK, 255);           // Full brightness
    beepConfirm();
  }
  else if(index == 6) { // Info Screen
     tft.fillScreen(ST77XX_BLACK);
     tft.setCursor(10, 40);
     tft.setTextColor(ST77XX_WHITE);
     tft.println("KEJU-BW16 V1X");
     tft.println("Dual-Band HW");
     delay(3000);
  }
  else if(index == 7) ota_platform_reset();
}

void beepConfirm() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(30);
  digitalWrite(BUZZER_PIN, LOW);
}

void audioTracker(int rssi) {
  // Map RSSI (-90 to -30) to a beep delay (500ms to 20ms)
  int beepDelay = map(constrain(rssi, -90, -30), -90, -30, 500, 20);
  
  digitalWrite(BUZZER_PIN, HIGH);
  delay(10);
  digitalWrite(BUZZER_PIN, LOW);
  delay(beepDelay);
}

void saveLog(String ssid, int rssi) {
  // If array is full, shift everything left to make room for new data
  if (logCount >= 10) {
    for (int i = 0; i < 9; i++) {
      networkLogs[i] = networkLogs[i + 1];
    }
    logCount = 9;
  }
  
  networkLogs[logCount].ssid = ssid;
  networkLogs[logCount].rssi = rssi;
  logCount++;
  
  // Quick haptic/audio feedback that a log was captured
  digitalWrite(BUZZER_PIN, HIGH); delay(5); digitalWrite(BUZZER_PIN, LOW);
}

void startScan() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(10, 10);
  tft.setTextColor(ST77XX_YELLOW);
  tft.println("Scanning Dual-Band...");
  
  int n = WiFi.scanNetworks();
  logCount = min(n, 10); // Update the global log count
  
  tft.fillScreen(ST77XX_BLACK);
  for (int i = 0; i < n; i++) {
    int rssi = WiFi.RSSI(i);
    
    // Save to Log Memory for the "View Logs" menu
    if (i < 10) {
      networkLogs[i].ssid = String(WiFi.SSID(i));
      networkLogs[i].rssi = rssi;
      // We'll set channel to 0 as a placeholder since .channel(i) is restrictive
      networkLogs[i].channel = 0; 
    }

    // Live Display (First 7 results)
    if (i < 7) {
      tft.setCursor(5, 5 + (i * 15));
      if (rssi > -50) tft.setTextColor(ST77XX_GREEN);
      else if (rssi > -75) tft.setTextColor(ST77XX_YELLOW);
      else tft.setTextColor(ST77XX_RED);
      
      tft.print(i+1); tft.print(": ");
      tft.print(WiFi.SSID(i));
      tft.setCursor(125, 5 + (i * 15));
      tft.print(rssi);
    }
  }
  
  tft.setCursor(10, 115);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("Scan Done. SEL to exit.");
  while(digitalRead(BTN_SEL) == HIGH); 
  delay(200);
}

//int signalHistory[160];

void packetMonitor() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(10, 5);
  tft.println("SIGNAL HISTORY TRACKER");
  
  // Initialize history
  for(int i=0; i<160; i++) signalHistory[i] = -100;

  while(digitalRead(BTN_SEL) == HIGH) {
    int n = WiFi.scanNetworks();
    long strongestRSSI = -100;
    
    if (n > 0) {
      for (int i = 0; i < n; i++) {
        if (WiFi.RSSI(i) > strongestRSSI) strongestRSSI = WiFi.RSSI(i);
      }
    }

    for (int i = 0; i < 159; i++) {
      signalHistory[i] = signalHistory[i+1];
    }
    signalHistory[159] = strongestRSSI;

    tft.fillRect(0, 30, 160, 82, ST77XX_BLACK);
    tft.drawFastHLine(0, 110, 160, ST77XX_GREY); 

    for (int x = 0; x < 158; x++) {
      int y1 = map(signalHistory[x], -100, -20, 110, 40);
      int y2 = map(signalHistory[x+1], -100, -20, 110, 40);
      
      uint16_t color = ST77XX_RED;
      if (signalHistory[x+1] > -75) color = ST77XX_YELLOW;
      if (signalHistory[x+1] > -55) color = ST77XX_GREEN;

      if(signalHistory[x] > -100) {
        tft.drawLine(x, y1, x+1, y2, color);
      }
    }

    tft.fillRect(100, 0, 60, 15, ST77XX_BLACK);
    tft.setCursor(105, 5);
    tft.setTextColor(ST77XX_WHITE);
    tft.print(strongestRSSI); tft.print("dBm");

    if (strongestRSSI > -85) {
      long beepDelay = map(strongestRSSI, -85, -30, 300, 20);
      digitalWrite(BUZZER_PIN, HIGH);
      delay(10); 
      digitalWrite(BUZZER_PIN, LOW);
      
      unsigned long startWait = millis();
      while(millis() - startWait < (unsigned long)beepDelay) {
        if(digitalRead(BTN_SEL) == LOW) break;
      }
    } else {
      delay(100); 
    }
  }
}

void beaconSpam() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(10, 10);
  tft.setTextColor(ST77XX_RED);
  tft.println("BEACON SPAM");
  const char* fakes[] = {"Free WiFi", "FBI Surveillance", "Cypher_5G", "BW16_Lab", "Hacked_Network"};

  while(digitalRead(BTN_SEL) == HIGH) {
    for(int i = 0; i < 5; i++) {
      WiFi.apbegin((char*)fakes[i], NULL, (char*)"1"); 
      tft.fillRect(0, 50, 160, 40, ST77XX_BLACK);
      tft.setCursor(10, 60);
      tft.setTextColor(ST77XX_WHITE);
      tft.print("Spamming: "); tft.println(fakes[i]);
      
      unsigned long start = millis();
      while(millis() - start < 800) { if(digitalRead(BTN_SEL) == LOW) break; }
      
      WiFi.disconnect(); 
      if(digitalRead(BTN_SEL) == LOW) break;
    }
  }
}

void deauthDetector() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, 10);
  tft.println("SECURITY MONITOR");
  tft.drawFastHLine(0, 25, 160, ST77XX_GREEN);
  
  int lastCount = 0;

  while(digitalRead(BTN_SEL) == HIGH) {
    int n = WiFi.scanNetworks();
    
    // If networks suddenly disappear or drop significantly
    if (lastCount > 0 && n < (lastCount / 2)) {
      tft.fillScreen(ST77XX_RED);
      tft.setTextColor(ST77XX_WHITE);
      tft.setTextSize(2);
      tft.setCursor(10, 50);
      tft.println("ATTACK!!");
      
      // Panic Alarm
      for(int i=0; i<5; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(50);
        digitalWrite(BUZZER_PIN, LOW);
        delay(50);
      }
      delay(2000);
      tft.fillScreen(ST77XX_BLACK);
    }
    
    lastCount = n;
    tft.setCursor(10, 40);
    tft.setTextColor(ST77XX_GREEN);
    tft.setTextSize(1);
    tft.print("Monitoring Bands...");
    tft.setCursor(10, 60);
    tft.print("Nodes Found: "); tft.println(n);
    
    delay(500);
  }
}

void drawRadar() {
  tft.fillScreen(ST77XX_BLACK);
  int centerX = 80;
  int centerY = 64;
  
  tft.drawCircle(centerX, centerY, 50, CYBER_GREEN);
  tft.drawCircle(centerX, centerY, 30, 0x03E0); // Dimmer green
  tft.drawLine(centerX, centerY - 50, centerX, centerY + 50, 0x03E0);
  tft.drawLine(centerX - 80, centerY, centerX + 80, centerY, 0x03E0);

  // Animate a sweeping line or expanding ring
  for(int r = 5; r < 55; r += 10) {
    tft.drawCircle(centerX, centerY, r, CYBER_GREEN);
    delay(80);
    tft.drawCircle(centerX, centerY, r, ST77XX_BLACK); // Clean up for next frame
  }
}

void showLogs() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(10, 5);
  tft.println("LAST CAPTURE");
  tft.drawFastHLine(0, 15, 160, ST77XX_GREY);

  if (logCount == 0) {
    tft.setCursor(30, 60);
    tft.println("NO DATA FOUND");
  } else {
    for (int i = 0; i < logCount; i++) {
      tft.setCursor(5, 20 + (i * 10));
      tft.setTextColor(ST77XX_WHITE);
      tft.print(i+1); tft.print(". ");
      tft.print(networkLogs[i].ssid);
      tft.setCursor(130, 20 + (i * 10));
      tft.print(networkLogs[i].rssi);
    }
  }
  while(digitalRead(BTN_SEL) == HIGH);
  delay(200);
}

void runAttack() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(CYBER_RED);
  tft.setCursor(10, 10);
  tft.setTextSize(2);
  tft.println("ATTACK ACTIVE");
  tft.setTextSize(1);
  tft.println("CLONING CAPTURES...");

  while(digitalRead(BTN_SEL) == HIGH) {
    for(int i = 0; i < logCount; i++) {
      // apbegin(SSID, password, channel)
      WiFi.apbegin((char*)networkLogs[i].ssid.c_str(), NULL, 1);
      
      tft.setCursor(10, 40 + (i * 8));
      tft.setTextColor(CYBER_WHITE);
      tft.print("SHADOW: "); tft.println(networkLogs[i].ssid);
      
      digitalWrite(BUZZER_PIN, HIGH); delay(2); digitalWrite(BUZZER_PIN, LOW);
      
      delay(400); // Allow time for beacons to be sent
      
      WiFi.disconnect(); // Standard command to stop current WiFi mode/AP
    }
    
    if(digitalRead(BTN_SEL) == LOW) break;
  }
  WiFi.disconnect(); 
}

void deauthWatcher() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(CYBER_GREEN);
  tft.setTextSize(1);
  tft.setCursor(10, 5);
  tft.println("--- PACKET WATCHER ---");
  
  int line = 0;
  while(digitalRead(BTN_SEL) == HIGH) {
    // We simulate a terminal feed based on noise/activity
    int y = 20 + (line * 10);
    if(y > 110) { 
        tft.fillRect(0, 20, 160, 100, ST77XX_BLACK); 
        line = 0; 
        y = 20; 
    }
    
    tft.setCursor(5, y);
    tft.print("[INFO] PKT: ");
    tft.print(random(0xFFFF), HEX); 
    tft.print(" TYPE: MGMT");
    
    line++;
    delay(random(100, 500));
  }
}

void selfDestruct() {
  tft.fillScreen(ST77XX_RED);
  tft.setTextColor(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 20);
  tft.println("CRITICAL");
  tft.setCursor(10, 45);
  tft.println("OVERLOAD");

  for (int i = 10; i >= 0; i--) {
    tft.fillRect(60, 80, 40, 30, ST77XX_RED); // Clear old number
    tft.setCursor(70, 85);
    tft.print(i);

    // Beep speed increases as time runs out
    digitalWrite(BUZZER_PIN, HIGH);
    delay(50);
    digitalWrite(BUZZER_PIN, LOW);
    
    // The delay decreases: 1000ms, 900ms, 800ms...
    delay(i * 100); 
  }

  // The "Explosion"
  tft.fillScreen(ST77XX_WHITE);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(1000);
  digitalWrite(BUZZER_PIN, LOW);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(CYBER_GREEN);
  tft.setTextSize(1);
  tft.setCursor(30, 60);
  tft.println("SYSTEM RECOVERED");
  delay(2000);
}

void drawBatteryIcon() {
  // Try PA_0 or A0 if your variant supports it
  int raw = analogRead(PA_0); 
  // Formula for 3.3V ADC: (Raw * 3.3 / 1024) * DividerRatio
  batteryVolts = (raw * 3.3 / 1024.0) * 2.0; 

  int barWidth = map(constrain(batteryVolts * 10, 33, 42), 33, 42, 0, 15);
  
  tft.drawRect(135, 4, 18, 10, CYBER_WHITE);
  tft.fillRect(153, 7, 2, 4, CYBER_WHITE); 
  
  uint16_t batColor = (barWidth > 5) ? CYBER_GREEN : CYBER_RED;
  tft.fillRect(137, 6, barWidth, 6, batColor);
}
