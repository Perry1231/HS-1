#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>

// Піни першого датчика (Плече)
#define SDA_SHOULDER 3
#define SCL_SHOULDER 2

// Піни другого датчика (Біцепс)
#define SDA_BICEPS 4
#define SCL_BICEPS 5

// Створюємо ОБ'ЄДНАНИЙ об'єкт на єдиній апаратній шині Wire
Adafruit_BNO055 bnoShoulder = Adafruit_BNO055(1, 0x28, &Wire);
Adafruit_BNO055 bnoBiceps   = Adafruit_BNO055(2, 0x28, &Wire);

bool shoulderOK = false;
bool bicepsOK   = false;

// Функція перемикання I2C на потрібні піни
void selectI2CPins(int sda, int scl) {
  Wire.end();                   // Зупиняємо поточну шину
  Wire.begin(sda, scl, 100000); // Перепідключаємо її до нових пінів
  delay(5);                     // Стабілізація шини
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n=== ВУЗОЛ ПЛЕЧЕ + БІЦЕПС (МУЛЬТИПЛЕКСУВАННЯ I2C) ===");

  // 1. Ініціалізація датчика Плеча (GPIO 3 / 2)
  Serial.print("1. Ініціалізація Плеча (GPIO 3/2)... ");
  selectI2CPins(SDA_SHOULDER, SCL_SHOULDER);
  if (bnoShoulder.begin(OPERATION_MODE_NDOF)) {
    Serial.println("УСПІШНО!");
    bnoShoulder.setExtCrystalUse(false);
    shoulderOK = true;
  } else {
    Serial.println("ПОМИЛКА!");
  }

  // 2. Ініціалізація датчика Біцепса (GPIO 4 / 5)
  Serial.print("2. Ініціалізація Біцепса (GPIO 4/5)... ");
  selectI2CPins(SDA_BICEPS, SCL_BICEPS);
  if (bnoBiceps.begin(OPERATION_MODE_NDOF)) {
    Serial.println("УСПІШНО!");
    bnoBiceps.setExtCrystalUse(false);
    bicepsOK = true;
  } else {
    Serial.println("ПОМИЛКА!");
  }

  Serial.println("-------------------------------------------------------");
}

void loop() {
  // --- Опитуємо Плече ---
  if (shoulderOK) {
    selectI2CPins(SDA_SHOULDER, SCL_SHOULDER);
    sensors_event_t e1;
    bnoShoulder.getEvent(&e1);
    Serial.print("Плече X: ");
    Serial.print(e1.orientation.x, 1);
  } else {
    Serial.print("Плече: N/A");
  }

  // --- Опитуємо Біцепс ---
  if (bicepsOK) {
    selectI2CPins(SDA_BICEPS, SCL_BICEPS);
    sensors_event_t e2;
    bnoBiceps.getEvent(&e2);
    Serial.print("° \t| Біцепс X: ");
    Serial.print(e2.orientation.x, 1);
    Serial.println("°");
  } else {
    Serial.println("° \t| Біцепс: N/A");
  }

  delay(40); // ~20 оновлень на секунду
}