#include <Arduino.h>
#include <Wire.h>
#include <SoftwareWire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>

// Апаратна шина (Плече)
#define HARD_SDA 3
#define HARD_SCL 2

// Програмна шина (Біцепс)
#define SOFT_SDA 4
#define SOFT_SCL 5

// 1. Апаратний I2C
Adafruit_BNO055 bnoShoulder = Adafruit_BNO055(1, 0x28, &Wire);

// 2. Програмний I2C на GPIO 4 / GPIO 5
SoftwareWire mySoftwareWire(SOFT_SDA, SOFT_SCL);
Adafruit_BNO055 bnoBiceps = Adafruit_BNO055(2, 0x28, (TwoWire*)&mySoftwareWire);

bool shoulderOK = false;
bool bicepsOK   = false;

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n=== ЗАПУСК ВУЗЛА ПЛЕЧЕ + БІЦЕПС ===");

  // Ініціалізація апаратного I2C (Плече)
  Wire.begin(HARD_SDA, HARD_SCL, 100000);
  Serial.print("1. Датчик Плеча (GPIO 3/2)... ");
  if (bnoShoulder.begin()) {
    Serial.println("УСПІШНО!");
    bnoShoulder.setExtCrystalUse(false);
    shoulderOK = true;
  } else {
    Serial.println("ПОМИЛКА (Перевірте пайку SDA/SCL)");
  }

  // Ініціалізація програмного I2C (Біцепс)
  mySoftwareWire.begin();
  Serial.print("2. Датчик Біцепса (GPIO 4/5)... ");
  if (bnoBiceps.begin()) {
    Serial.println("УСПІШНО!");
    bnoBiceps.setExtCrystalUse(false);
    bicepsOK = true;
  } else {
    Serial.println("ПОМИЛКА (Перевірте пайку SDA/SCL на GPIO 4/5)");
  }

  Serial.println("----------------------------------------");
}

void loop() {
  if (shoulderOK) {
    sensors_event_t e1;
    bnoShoulder.getEvent(&e1);
    Serial.print("Плече X: ");
    Serial.print(e1.orientation.x, 1);
  }

  if (bicepsOK) {
    sensors_event_t e2;
    bnoBiceps.getEvent(&e2);
    Serial.print("° \t| Біцепс X: ");
    Serial.print(e2.orientation.x, 1);
    Serial.println("°");
  } else {
    Serial.println();
  }

  delay(40);
}