#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>

const int I2C_SDA = 3; // SDA на GPIO 3
const int I2C_SCL = 2; // SCL на GPIO 2

Adafruit_BNO055 bno28 = Adafruit_BNO055(55, 0x28);
Adafruit_BNO055 bno29 = Adafruit_BNO055(55, 0x29);

Adafruit_BNO055* activeBno = NULL;

void setup() {
  Serial.begin(115200);

  while (!Serial) {
    delay(10);
  }

  Serial.println("\n=== ТЕСТ BNO055 (ВНУТРІШНІЙ ГЕНЕРАТОР): SDA=3, SCL=2 ===");

  Wire.begin(I2C_SDA, I2C_SCL);

  Serial.print("Спроба підключення до 0x28... ");
  if (bno28.begin()) {
    Serial.println("ЗНАЙДЕНО (0x28)!");
    activeBno = &bno28;
  } else {
    Serial.println("немає відповіді.");
    Serial.print("Спроба підключення до 0x29... ");
    if (bno29.begin()) {
      Serial.println("ЗНАЙДЕНО (0x29)!");
      activeBno = &bno29;
    } else {
      Serial.println("немає відповіді.");
    }
  }

  if (activeBno == NULL) {
    Serial.println("\n[ПОМИЛКА] Датчик BNO055 не відповідає!");
    while (1) {
      delay(1000);
    }
  }

  // Явно вимикаємо зовнішній кварц — використовуємо ВНУТРІШНІЙ генератор
  activeBno->setExtCrystalUse(false);
  
  Serial.println("Датчик готовий до роботи!");
  Serial.println("--------------------------------------------------");
}

void loop() {
  sensors_event_t event;
  activeBno->getEvent(&event);

  // Отримуємо статус калібрування
  uint8_t sys, gyro, accel, mag;
  sys = gyro = accel = mag = 0;
  activeBno->getCalibration(&sys, &gyro, &accel, &mag);

  Serial.print("X (Yaw): "); Serial.print(event.orientation.x, 1);
  Serial.print("° \t| Y (Roll): "); Serial.print(event.orientation.y, 1);
  Serial.print("° \t| Z (Pitch): "); Serial.print(event.orientation.z, 1);
  Serial.print("° \t[Calib Sys:"); Serial.print(sys);
  Serial.print(" G:"); Serial.print(gyro);
  Serial.print(" A:"); Serial.print(accel);
  Serial.print(" M:"); Serial.print(mag); Serial.println("]");

  delay(100);
}