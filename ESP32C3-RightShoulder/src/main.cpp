#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>

// --- Конфігурація пінів для Adafruit QT Py ESP32-C3 ---
#define SDA_PIN SDA
#define SCL_PIN SCL
#define BNO055_ADDR 0x28

Adafruit_BNO055 bno = Adafruit_BNO055(55, BNO055_ADDR, &Wire);

bool sensorReady = false;
uint32_t frameCount = 0;
uint32_t lastFpsCalc = 0;
float currentFps = 0.0;

// Малювання горизонтальної шкали-слайдера
void drawAxisBar(float val, float minV, float maxV, int width = 14) {
  float norm = (val - minV) / (maxV - minV);
  if (norm < 0.0) norm = 0.0;
  if (norm > 1.0) norm = 1.0;

  int pos = norm * width;
  int center = width / 2;

  Serial.print("[");
  for (int i = 0; i <= width; ++i) {
    if (i == pos) {
      Serial.print("◆"); // Поточний маркер кута
    } else if (i == center) {
      Serial.print("│"); // Нульовий центр
    } else {
      Serial.print("─");
    }
  }
  Serial.print("]");
}

// Оформлення статусу калібрування (0..3)
void printCalibBadge(uint8_t sys, uint8_t gyro, uint8_t accel, uint8_t mag) {
  Serial.print("CALIB: ");
  Serial.print("SYS:"); Serial.print(sys);   Serial.print(" ");
  Serial.print("G:");   Serial.print(gyro);  Serial.print(" ");
  Serial.print("A:");   Serial.print(accel); Serial.print(" ");
  Serial.print("M:");   Serial.print(mag);
  
  if (sys == 3) {
    Serial.print(" [  FULL  ]");
  } else if (sys > 0) {
    Serial.print(" [ MID-CAL ]");
  } else {
    Serial.print(" [ NO-CAL  ]");
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000); 

  // Ефектний заголовок та логотип системи
  Serial.println("\n");
  Serial.println(" ░▒▓████████████████████████████████████████████████████████▓▒░");
  Serial.println(" ░▒▓█      BNO055 ORIENTATION HUD - ESP32-C3 QT Py     █▓▒░");
  Serial.println(" ░▒▓████████████████████████████████████████████████████████▓▒░");
  Serial.println("  ├── Hardware Subsystem : Adafruit QT Py ESP32-C3");
  Serial.println("  ├── Protocol Interface : I2C Bus (Fast Mode 100kHz)");
  Serial.println("  └── Target Address     : 0x28 (Default Mode)");
  Serial.println(" ──────────────────────────────────────────────────────────────");

  Wire.begin(SDA_PIN, SCL_PIN, 100000);

  Serial.print("  [SYSTEM INIT] Connecting to BNO055...");

  if (bno.begin(OPERATION_MODE_NDOF)) {
    bno.setExtCrystalUse(false);
    sensorReady = true;
    Serial.println(" [  OK - ONLINE  ]");
  } else {
    Serial.println(" [  FAILED - N/A  ]");
    Serial.println("\n  ❌ CRITICAL ERROR: Sensor non-responsive.");
    Serial.println("     Please check GND connection on COM3 pin and I2C lines.\n");
  }

  Serial.println(" ──────────────────────────────────────────────────────────────\n");
  delay(1200);

  if (sensorReady) {
    Serial.println("  YAW (Z: 0..360°)     PITCH (Y: ±180°)     ROLL (X: ±90°)       FPS   STATUS");
    Serial.println(" ══════════════════════════════════════════════════════════════════════════════════");
  }
}

void loop() {
  if (!sensorReady) {
    Serial.println(" ⚠️ [ALERT] Searching for device on I2C bus...");
    delay(1000);
    return;
  }

  // Розрахунок реального FPS оновлення даних
  frameCount++;
  uint32_t now = millis();
  if (now - lastFpsCalc >= 1000) {
    currentFps = frameCount * 1000.0 / (now - lastFpsCalc);
    frameCount = 0;
    lastFpsCalc = now;
  }

  // Отримання даних орієнтації
  sensors_event_t event;
  bno.getEvent(&event);

  // Отримання стану калібрування
  uint8_t sys, gyro, accel, mag;
  bno.getCalibration(&sys, &gyro, &accel, &mag);

  float yaw   = event.orientation.x; 
  float pitch = event.orientation.y; 
  float roll  = event.orientation.z; 

  // Форматоване виведення кутів
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "  %5.1f° ", yaw);
  Serial.print(buffer);
  drawAxisBar(yaw, 0.0, 360.0, 10);

  snprintf(buffer, sizeof(buffer), "  %5.1f° ", pitch);
  Serial.print(buffer);
  drawAxisBar(pitch, -180.0, 180.0, 10);

  snprintf(buffer, sizeof(buffer), "  %5.1f° ", roll);
  Serial.print(buffer);
  drawAxisBar(roll, -90.0, 90.0, 10);

  // FPS + статус калібрування
  snprintf(buffer, sizeof(buffer), "  %4.1f ", currentFps);
  Serial.print(buffer);

  printCalibBadge(sys, gyro, accel, mag);
  Serial.println();

  delay(40); // ~25 FPS
}