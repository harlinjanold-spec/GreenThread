#include <Arduino.h>
#include <DHT.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Wire.h>
#include "SparkFun_AS7265X.h"
#include <Stepper.h>

// ==========================================
// PIN CONFIGURATIONS
// ==========================================

// --- TFT Display (ST7735 HSPI) ---
#define TFT_CS    22 
#define TFT_RST   4
#define TFT_DC    2
#define TFT_SDA   13 // MOSI
#define TFT_SCK   14 // SCK

// --- Sensors ---
#define DHTPIN 32
#define DHTTYPE DHT11
#define MOISTURE_PIN 34
#define SPECTRO_SDA 21
#define SPECTRO_SCL 33 // Moved from 22 to avoid TFT CS conflict!

// --- Motors ---
#define DC_MOTOR_IN1 26
#define DC_MOTOR_IN2 27

#define STEPPER_IN1 19
#define STEPPER_IN2 18
#define STEPPER_IN3 5
#define STEPPER_IN4 17

// ==========================================
// OBJECTS & VARIABLES
// ==========================================

SPIClass hspi(HSPI);
Adafruit_ST7735 tft = Adafruit_ST7735(&hspi, TFT_CS, TFT_DC, TFT_RST);
#define ST77XX_DARKGREEN 0x03E0

DHT dht(DHTPIN, DHTTYPE);
AS7265X spectroscopy;
bool hasSpectroscopy = false;

const int stepsPerRevolution = 2048; 
Stepper myStepper(stepsPerRevolution, STEPPER_IN1, STEPPER_IN3, STEPPER_IN2, STEPPER_IN4);

unsigned long lastTelemetryTime = 0;
float currentTemp = 0.0;
float currentHum = 0.0;
int lastMoisture = 0;
String lastStatusMsg = "Booting...";

enum DisplayMode { MODE_DATA, MODE_RADAR, MODE_PLANT };
DisplayMode currentMode = MODE_DATA;
float radarAngle = 0;
unsigned long lastRadarUpdate = 0;
int plantFrame = 0;
unsigned long lastPlantUpdate = 0;

// ==========================================
// UI FUNCTIONS
// ==========================================

void drawDataUI() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(10, 10);
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(2);
  tft.println("GreenThread OS");
  tft.drawLine(0, 30, 160, 30, ST77XX_WHITE);
  
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, 45); tft.print("Temp:");
  tft.setCursor(10, 65); tft.print("Humidity:");
  tft.setCursor(10, 85); tft.print("Soil Moist:");
  
  tft.setCursor(10, 110);
  tft.setTextColor(ST77XX_YELLOW);
  tft.print("Status:");
}

void updateTFTValues(String statusMsg = "") {
  if (statusMsg != "") lastStatusMsg = statusMsg;
  if (currentMode != MODE_DATA) return; 
  
  // Update Temp
  tft.fillRect(80, 45, 80, 15, ST77XX_BLACK);
  tft.setCursor(80, 45);
  tft.setTextColor(ST77XX_CYAN);
  tft.print(currentTemp, 1); tft.print(" C");

  // Update Humidity
  tft.fillRect(80, 65, 80, 15, ST77XX_BLACK);
  tft.setCursor(80, 65);
  tft.print(currentHum, 1); tft.print(" %");

  // Update Moisture
  tft.fillRect(80, 85, 80, 15, ST77XX_BLACK);
  tft.setCursor(80, 85);
  tft.print(lastMoisture);
  
  // Update Status
  tft.fillRect(60, 110, 100, 15, ST77XX_BLACK);
  tft.setCursor(60, 110);
  tft.setTextColor(ST77XX_MAGENTA);
  tft.print(lastStatusMsg);
}

void setDisplayMode(DisplayMode newMode) {
  if (currentMode == newMode) return;
  currentMode = newMode;
  if (currentMode == MODE_DATA) {
    drawDataUI();
    String oldStatus = lastStatusMsg;
    lastStatusMsg = "";
    updateTFTValues(oldStatus);
  } else if (currentMode == MODE_RADAR) {
    tft.fillScreen(ST77XX_BLACK);
  } else if (currentMode == MODE_PLANT) {
    tft.fillScreen(ST77XX_BLACK);
    plantFrame = 0;
  }
}

void drawRadarFrame() {
  if (millis() - lastRadarUpdate < 50) return; // ~20 FPS
  lastRadarUpdate = millis();
  
  int cx = 160 / 2;
  int cy = 128 / 2 + 10;
  int radius = 50;
  
  int oldX = cx + radius * cos(radarAngle);
  int oldY = cy + radius * sin(radarAngle);
  tft.drawLine(cx, cy, oldX, oldY, ST77XX_BLACK);
  
  radarAngle += 0.15;
  if (radarAngle > 2 * PI) radarAngle -= 2 * PI;
  
  int newX = cx + radius * cos(radarAngle);
  int newY = cy + radius * sin(radarAngle);
  
  tft.drawCircle(cx, cy, radius, ST77XX_DARKGREEN);
  tft.drawCircle(cx, cy, radius - 15, ST77XX_DARKGREEN);
  tft.drawLine(cx - radius, cy, cx + radius, cy, ST77XX_DARKGREEN);
  tft.drawLine(cx, cy - radius, cx, cy + radius, ST77XX_DARKGREEN);
  tft.drawLine(cx, cy, newX, newY, ST77XX_GREEN);
  
  tft.setCursor(5, 5);
  tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  tft.print("DRIVING...");
}

void drawPlantFrame() {
  if (millis() - lastPlantUpdate < 40) return; // 25 FPS
  lastPlantUpdate = millis();
  
  if (plantFrame == 0) {
    tft.fillScreen(ST77XX_BLACK);
    tft.drawCircle(80, 95, 25, ST77XX_WHITE);
    tft.fillRect(50, 115, 60, 10, ST77XX_BLACK); 
    tft.drawLine(60, 115, 100, 115, ST77XX_WHITE); 
    tft.fillRect(60, 60, 40, 18, ST77XX_BLACK); 
    tft.drawLine(65, 78, 65, 55, ST77XX_WHITE);
    tft.drawLine(95, 78, 95, 55, ST77XX_WHITE);
    tft.drawRect(60, 52, 40, 4, ST77XX_WHITE);
    tft.drawLine(58, 95, 102, 95, ST77XX_CYAN);
    tft.drawCircle(70, 105, 3, ST77XX_CYAN);
    tft.drawCircle(90, 110, 2, ST77XX_CYAN);
    tft.drawCircle(82, 102, 1, ST77XX_CYAN);
  }
  
  if (plantFrame > 10 && plantFrame <= 90) {
     int y = 95 - (plantFrame - 10); 
     tft.drawPixel(79, y, ST77XX_GREEN);
     tft.drawPixel(80, y, ST77XX_GREEN);
     tft.drawPixel(81, y, ST77XX_GREEN);
  }
  
  if (plantFrame > 30 && plantFrame <= 50) {
     int i = plantFrame - 30;
     int x = 80 + i*1.5;
     int y = 70 - i/2;
     tft.drawPixel(x, y, ST77XX_GREEN);
     tft.drawPixel(x, y+1, ST77XX_GREEN);
     tft.drawPixel(x, y+2, ST77XX_GREEN);
  }
  
  if (plantFrame > 40 && plantFrame <= 80) {
     int i = plantFrame - 40;
     int x = 80 - i;
     int y = 55 - i;
     tft.drawPixel(x, y, ST77XX_GREEN);
     int thick = max(1, 4 - (i/10));
     for(int w=0; w<thick; w++) tft.drawPixel(x, y+w, ST77XX_GREEN);
  }
  
  if (plantFrame > 60 && plantFrame <= 110) {
     int i = plantFrame - 60;
     int x = 80 + i*1.2;
     int y = 40 - i*1.2;
     tft.drawPixel(x, y, ST77XX_GREEN);
     int thick = max(1, 5 - (i/10));
     for(int w=0; w<thick; w++) tft.drawPixel(x-w, y+w, ST77XX_GREEN);
  }
  
  if (plantFrame == 120) {
    tft.setCursor(30, 118);
    tft.setTextColor(ST77XX_YELLOW);
    tft.setTextSize(1);
    tft.print("Sampling Data...");
  }
  
  plantFrame++;
  if (plantFrame > 160) plantFrame = 0;
}

// ==========================================
// CORE FUNCTIONS
// ==========================================

void setup() {
  Serial.begin(115200);
  
  // 1. Initialize Motors
  pinMode(DC_MOTOR_IN1, OUTPUT);
  pinMode(DC_MOTOR_IN2, OUTPUT);
  digitalWrite(DC_MOTOR_IN1, LOW);
  digitalWrite(DC_MOTOR_IN2, LOW);
  myStepper.setSpeed(30); 
  
  // 2. Initialize Env Sensors
  dht.begin();
  pinMode(MOISTURE_PIN, INPUT);

  // 3. Initialize Display
  hspi.begin(14, 12, 13, 25); 
  tft.initR(INITR_BLACKTAB); 
  tft.setRotation(1); 
  drawDataUI();
  updateTFTValues("Booting...");

  // 4. Initialize AS7265x (I2C)
  Wire.begin(SPECTRO_SDA, SPECTRO_SCL);
  if(spectroscopy.begin() == false) {
    Serial.println("WARNING: AS7265x Sensor not detected. Check I2C wiring.");
    hasSpectroscopy = false;
  } else {
    hasSpectroscopy = true;
    spectroscopy.disableIndicator();
    Serial.println("AS7265x Initialized.");
  }
  
  updateTFTValues("Ready.");
  Serial.println("GreenThread OS Ready!");
}

void sendTelemetry() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (!isnan(h) && !isnan(t)) {
    currentHum = h;
    currentTemp = t;
  }
  lastMoisture = analogRead(MOISTURE_PIN);
  
  Serial.printf("{\"type\":\"telemetry\", \"temp\":%.1f, \"hum\":%.1f, \"moisture\":%d}\n", currentTemp, currentHum, lastMoisture);
  updateTFTValues(); 
}

void performSensorScan() {
  setDisplayMode(MODE_PLANT); 
  
  // 1. Drop the sensor head
  myStepper.step(3000); 
  
  // 2. Read Analog Soil Moisture
  lastMoisture = analogRead(MOISTURE_PIN);
  
  // 3. Read Spectroscopy Data (if available)
  if (hasSpectroscopy) {
    spectroscopy.takeMeasurements();
    Serial.print("{\"type\":\"spectrum\", \"csv\":\"");
    Serial.print(spectroscopy.getCalibratedA()); Serial.print(",");
    Serial.print(spectroscopy.getCalibratedB()); Serial.print(",");
    Serial.print(spectroscopy.getCalibratedC()); Serial.print(",");
    Serial.print(spectroscopy.getCalibratedD()); Serial.print(",");
    Serial.print(spectroscopy.getCalibratedE()); Serial.print(",");
    Serial.print(spectroscopy.getCalibratedF()); Serial.print(",");
    Serial.print(spectroscopy.getCalibratedG()); Serial.print(",");
    Serial.print(spectroscopy.getCalibratedH()); Serial.print(",");
    Serial.print(spectroscopy.getCalibratedI()); Serial.print(",");
    Serial.print(spectroscopy.getCalibratedJ()); Serial.print(",");
    Serial.print(spectroscopy.getCalibratedK()); Serial.print(",");
    Serial.print(spectroscopy.getCalibratedL()); Serial.print(",");
    Serial.print(spectroscopy.getCalibratedR()); Serial.print(",");
    Serial.print(spectroscopy.getCalibratedS()); Serial.print(",");
    Serial.print(spectroscopy.getCalibratedT()); Serial.print(",");
    Serial.print(spectroscopy.getCalibratedU()); Serial.print(",");
    Serial.print(spectroscopy.getCalibratedV()); Serial.print(",");
    Serial.print(spectroscopy.getCalibratedW()); 
    Serial.println("\"}");
  }
  
  // 4. Lift the sensor head
  myStepper.step(-3000); 
  
  setDisplayMode(MODE_DATA);
  sendTelemetry();
  updateTFTValues("Scan Complete");
}

void loop() {
  if (currentMode == MODE_RADAR) {
    drawRadarFrame();
  } else if (currentMode == MODE_PLANT) {
    drawPlantFrame();
  }
  
  // Telemetry heartbeats every 4s
  if (millis() - lastTelemetryTime > 4000) {
    sendTelemetry();
    lastTelemetryTime = millis();
  }

  // Receive Commands from Python Remote UI
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    switch (cmd) {
      case 'F': 
        analogWrite(DC_MOTOR_IN1, 180); 
        analogWrite(DC_MOTOR_IN2, 0);   
        setDisplayMode(MODE_RADAR);
        break;
      case 'B': 
        analogWrite(DC_MOTOR_IN1, 0);   
        analogWrite(DC_MOTOR_IN2, 180); 
        setDisplayMode(MODE_RADAR);
        break;
      case 'S': 
        analogWrite(DC_MOTOR_IN1, 0);
        analogWrite(DC_MOTOR_IN2, 0);
        setDisplayMode(MODE_DATA);
        updateTFTValues("Stopped");
        break;
      case 'D': 
        updateTFTValues("Scanning...");
        performSensorScan();
        break;
      case 'U': 
        // Just lifting manually if needed
        myStepper.step(-3000); 
        break;
    }
  }
}
