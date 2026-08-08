#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>

// Штатні піни Adafruit QT Py ESP32-C3
#define SDA_PIN 3
#define SCL_PIN 2

Adafruit_BNO055 bno28 = Adafruit_BNO055(55, 0x28, &Wire);
Adafruit_BNO055 bno29 = Adafruit_BNO055(55, 0x29, &Wire);
Adafruit_BNO055* activeBno = nullptr;

bool sensorReady = false;

void setup() {
  Serial.begin(115200);

  // Очікування відкриття USB CDC монітора
  uint32_t startWait = millis();
  while (!Serial && (millis() - startWait < 4000)) {
    delay(10);
  }
  delay(500);

  Serial.println("\n\n==========================================");
  Serial.println("   ІНІЦІАЛІЗАЦІЯ WCMCU-055 (BNO055)...     ");
  Serial.println("==========================================\n");

  Wire.setTimeOut(1000);
  Wire.begin(SDA_PIN, SCL_PIN, 100000);

  Serial.print("• Перевірка адреси 0x28... ");
  if (bno28.begin(OPERATION_MODE_NDOF)) {
    activeBno = &bno28;
    sensorReady = true;
    Serial.println("✅ ЗНАЙДЕНО!");
  } else {
    Serial.println("немає.");
    Serial.print("• Перевірка адреси 0x29... ");
    if (bno29.begin(OPERATION_MODE_NDOF)) {
      activeBno = &bno29;
      sensorReady = true;
      Serial.println("✅ ЗНАЙДЕНО!");
    } else {
      Serial.println("немає.");
      Serial.println("\n❌ Датчик не відповідає! Перевірте дріт COM3 -> GND.");
    }
  }

  if (sensorReady) {
    activeBno->setExtCrystalUse(false);
    Serial.println("\n🚀 Датчик підключено! Потік даних:\n");
  }
}

void loop() {
  if (!sensorReady) {
    Serial.println("⚠️ [ALERT] Очікування датчика WCMCU-055...");
    delay(2000);
    return;
  }

  sensors_event_t event;
  activeBno->getEvent(&event);

  uint8_t sys, gyro, accel, mag;
  activeBno->getCalibration(&sys, &gyro, &accel, &mag);

  // Вивід Yaw, Pitch, Roll та статусу калібрування
  Serial.print("Yaw: ");   Serial.print(event.orientation.x, 1);
  Serial.print("° | Pitch: "); Serial.print(event.orientation.y, 1);
  Serial.print("° | Roll: ");  Serial.print(event.orientation.z, 1);
  Serial.print("° | CALIB [Sys:"); Serial.print(sys);
  Serial.print(" G:"); Serial.print(gyro);
  Serial.print(" A:"); Serial.print(accel);
  Serial.print(" M:"); Serial.print(mag);
  Serial.println("]");

  delay(50); // ~20 Гц
}