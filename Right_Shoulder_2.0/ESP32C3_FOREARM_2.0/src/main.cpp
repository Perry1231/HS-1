#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include "ArmPackets.h"

#define SDA_PIN 3
#define SCL_PIN 2
#define RX_PIN  1
#define TX_PIN  4

// Створюємо об'єкти під обидві можливі адреси
Adafruit_BNO055 bno28 = Adafruit_BNO055(55, 0x28, &Wire);
Adafruit_BNO055 bno29 = Adafruit_BNO055(55, 0x29, &Wire);

// Вказуємо вказівник на активний сенсор
Adafruit_BNO055* activeBno = nullptr;

HardwareSerial LinkSerial(1);
SensorData handData;
SensorData forearmData;

void setup() {
  LinkSerial.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
  pinMode(SDA_PIN, INPUT_PULLUP);
pinMode(SCL_PIN, OUTPUT);

for (int i = 0; i < 10; i++) {
  digitalWrite(SCL_PIN, HIGH);
  delayMicroseconds(5);
  digitalWrite(SCL_PIN, LOW);
  delayMicroseconds(5);
}


  Wire.begin(SDA_PIN, SCL_PIN, 100000);

  // 1. Пробуємо завантажити за адресою 0x28
  if (bno28.begin(OPERATION_MODE_NDOF)) {
    activeBno = &bno28;
    activeBno->setExtCrystalUse(false);
  } 
  // 2. Якщо 0x28 не відповів, пробуємо 0x29
  else if (bno29.begin(OPERATION_MODE_NDOF)) {
    activeBno = &bno29;
    activeBno->setExtCrystalUse(false);
  }
}

void loop() {
  // 1. Зчитуємо пакет від Кисті
  while (LinkSerial.available() >= sizeof(SensorData)) {
    LinkSerial.readBytes((uint8_t*)&handData, sizeof(SensorData));
  }

  // 2. Якщо якийсь із BNO055 підключився — зчитуємо його
  if (activeBno != nullptr) {
    sensors_event_t event;
    activeBno->getEvent(&event);
    forearmData.yaw = event.orientation.x;
    forearmData.pitch = event.orientation.y;
    forearmData.roll = event.orientation.z;
  }

  // 3. Відправляємо каскад (Hand + Forearm) на Плече
  LinkSerial.write((uint8_t*)&handData, sizeof(SensorData));
  LinkSerial.write((uint8_t*)&forearmData, sizeof(SensorData));

  delay(20);
}