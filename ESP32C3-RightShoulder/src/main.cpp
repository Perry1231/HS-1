#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>

// --- Штатні апаратні піни Adafruit QT Py ESP32-C3 ---
#define SDA_PIN 3  // Ніжка SDA (GPIO 5)
#define SCL_PIN 2  // Ніжка SCL (GPIO 6)

Adafruit_BNO055 bno28 = Adafruit_BNO055(55, 0x28, &Wire);
Adafruit_BNO055 bno29 = Adafruit_BNO055(55, 0x29, &Wire);
Adafruit_BNO055* activeBno = nullptr;

bool sensorReady = false;
uint32_t frameCount = 0;
uint32_t lastFpsCalc = 0;
float currentFps = 0.0;

// Графічний слайдер-бар для консолі
void drawAxisBar(float val, float minV, float maxV, int width = 10) {
  float norm = (val - minV) / (maxV - minV);
  if (norm < 0.0) norm = 0.0;
  if (norm > 1.0) norm = 1.0;

  int pos = norm * width;
  int center = width / 2;

  Serial.print("[");
  for (int i = 0; i <= width; ++i) {
    if (i == pos) {
      Serial.print("◆");
    } else if (i == center) {
      Serial.print("│");
    } else {
      Serial.print("─");
    }
  }
  Serial.print("]");
}

// Бейдж статусу калібрування
void printCalibBadge(uint8_t sys, uint8_t gyro, uint8_t accel, uint8_t mag) {
  Serial.print(" CALIB: ");
  Serial.print("S:"); Serial.print(sys);   Serial.print(" ");
  Serial.print("G:"); Serial.print(gyro);  Serial.print(" ");
  Serial.print("A:"); Serial.print(accel); Serial.print(" ");
  Serial.print("M:"); Serial.print(mag);

  if (sys == 3) {
    Serial.print(" [ FULL ]");
  } else if (sys > 0) {
    Serial.print(" [ MID  ]");
  } else {
    Serial.print(" [ CAL..]");
  }
}

void setup() {
  Serial.begin(115200);

  // Чекаємо відкриття монітора USB CDC
  uint32_t startWait = millis();
  while (!Serial && (millis() - startWait < 4000)) {
    delay(10);
  }
  delay(500);

  Serial.println("\n");
  Serial.println(" ░▒▓████████████████████████████████████████████████████████▓▒░");
  Serial.println(" ░▒▓█      BNO055 VISUAL HUD — QT Py ESP32-C3 (GPIO 5/6)    █▓▒░");
  Serial.println(" ░▒▓████████████████████████████████████████████████████████▓▒░");
  Serial.println("  ├── Pins : SDA -> GPIO 5 | SCL -> GPIO 6");
  Serial.println("  └── Mode : Hardware I2C (Internal Pull-Ups Enabled)");
  Serial.println(" ──────────────────────────────────────────────────────────────");

  pinMode(SDA_PIN, INPUT_PULLUP);
  pinMode(SCL_PIN, INPUT_PULLUP);

  Wire.setTimeOut(1000);
  Wire.begin(SDA_PIN, SCL_PIN, 100000);

  Serial.print("  [1/2] Пошук BNO055 (0x28)... ");
  if (bno28.begin(OPERATION_MODE_NDOF)) {
    activeBno = &bno28;
    sensorReady = true;
    Serial.println("[ ONLINE ✅ ]");
  } else {
    Serial.println("[ немає ]");
    Serial.print("  [2/2] Пошук BNO055 (0x29)... ");
    if (bno29.begin(OPERATION_MODE_NDOF)) {
      activeBno = &bno29;
      sensorReady = true;
      Serial.println("[ ONLINE ✅ ]");
    } else {
      Serial.println("[ немає ]");
      Serial.println("\n  ❌ Помилка: Жоден BNO055 не відповідає!");
    }
  }

  if (sensorReady) {
    // Вимикаємо зовнішній кварц (використовуємо внутрішній осцилятор BNO055)
    activeBno->setExtCrystalUse(false);
    Serial.println(" ──────────────────────────────────────────────────────────────\n");
    Serial.println("  YAW (Z: 0..360°)     PITCH (Y: ±180°)     ROLL (X: ±90°)       FPS   STATUS");
    Serial.println(" ══════════════════════════════════════════════════════════════════════════════════");
  }
}

void loop() {
  if (!sensorReady) {
    Serial.println(" ⚠️ [ALERT] Датчик не виявлено. Перевірте з'єднання.");
    delay(1500);
    return;
  }

  // Обчислення FPS
  frameCount++;
  uint32_t now = millis();
  if (now - lastFpsCalc >= 1000) {
    currentFps = frameCount * 1000.0 / (now - lastFpsCalc);
    frameCount = 0;
    lastFpsCalc = now;
  }

  sensors_event_t event;
  activeBno->getEvent(&event);

  uint8_t sys, gyro, accel, mag;
  activeBno->getCalibration(&sys, &gyro, &accel, &mag);

  float yaw   = event.orientation.x; 
  float pitch = event.orientation.y; 
  float roll  = event.orientation.z; 

  char buffer[64];
  
  // Yaw
  snprintf(buffer, sizeof(buffer), "  %5.1f° ", yaw);
  Serial.print(buffer);
  drawAxisBar(yaw, 0.0, 360.0, 10);

  // Pitch
  snprintf(buffer, sizeof(buffer), "  %5.1f° ", pitch);
  Serial.print(buffer);
  drawAxisBar(pitch, -180.0, 180.0, 10);

  // Roll
  snprintf(buffer, sizeof(buffer), "  %5.1f° ", roll);
  Serial.print(buffer);
  drawAxisBar(roll, -90.0, 90.0, 10);

  // FPS
  snprintf(buffer, sizeof(buffer), "  %4.1f ", currentFps);
  Serial.print(buffer);

  // Калібрування
  printCalibBadge(sys, gyro, accel, mag);
  Serial.println();

  delay(40); // ~25 FPS
}