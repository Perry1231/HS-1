#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>

// Піни I2C для ESP32-C3
const int I2C_SDA = 3;
const int I2C_SCL = 2;

// Створюємо 2 об'єкти з різними адресами
Adafruit_BNO055 bnoShoulder = Adafruit_BNO055(1, 0x28); // Плече (ADD на GND)
Adafruit_BNO055 bnoBiceps   = Adafruit_BNO055(2, 0x29); // Біцепс (ADD на 3.3V)

bool shoulderReady = false;
bool bicepsReady   = false;

void setup() {
  Serial.begin(115200);

  while (!Serial) {
    delay(10);
  }

  Serial.println("\n=== ВУЗОЛ ПЛЕЧЕ + БІЦЕПС (2x BNO055) ===");

  // Запуск шини I2C
  Wire.begin(I2C_SDA, I2C_SCL);

  // 1. Ініціалізація датчика Плеча (0x28)
  Serial.print("Ініціалізація BNO055 Плече (0x28)... ");
  if (bnoShoulder.begin()) {
    Serial.println("УСПІШНО!");
    bnoShoulder.setExtCrystalUse(false); // Внутрішній генератор
    shoulderReady = true;
  } else {
    Serial.println("ПОМИЛКА! Не знайдено.");
  }

  // 2. Ініціалізація датчика Біцепса (0x29)
  Serial.print("Ініціалізація BNO055 Біцепс (0x29)... ");
  if (bnoBiceps.begin()) {
    Serial.println("УСПІШНО!");
    bnoBiceps.setExtCrystalUse(false); // Внутрішній генератор
    bicepsReady = true;
  } else {
    Serial.println("ПОМИЛКА! Не знайдено (перевірь пін ADD на 3.3V).");
  }

  Serial.println("--------------------------------------------------");
}

void loop() {
  // Зчитуємо датчик Плеча
  if (shoulderReady) {
    sensors_event_t event1;
    bnoShoulder.getEvent(&event1);
    Serial.print("Плече [X]: "); 
    Serial.print(event1.orientation.x, 1);
  } else {
    Serial.print("Плече: N/A");
  }

  // Зчитуємо датчик Біцепса
  if (bicepsReady) {
    sensors_event_t event2;
    bnoBiceps.getEvent(&event2);
    Serial.print("\t| Біцепс [X]: "); 
    Serial.println(event2.orientation.x, 1);
  } else {
    Serial.println("\t| Біцепс: N/A");
  }

  delay(50); // ~20 оновлень на секунду
}