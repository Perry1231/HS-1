#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include "ArmPackets.h"

#define SDA_PIN 3
#define SCL_PIN 2
#define TX_PIN  4

Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28, &Wire);
HardwareSerial LinkSerial(1);
SensorData handData;
bool bnoReady = false;

void setup() {
  LinkSerial.begin(115200, SERIAL_8N1, -1, TX_PIN);

  // 1. Примусове вибивання I2C з заторів (Bus Recovery)
  pinMode(SCL_PIN, OUTPUT);
  pinMode(SDA_PIN, INPUT_PULLUP);
  for (int i = 0; i < 10; i++) {
    digitalWrite(SCL_PIN, HIGH);
    delayMicroseconds(5);
    digitalWrite(SCL_PIN, LOW);
    delayMicroseconds(5);
  }

  Wire.begin(SDA_PIN, SCL_PIN, 100000);
  Wire.setTimeOut(50);

  if (bno.begin(OPERATION_MODE_NDOF)) {
    bno.setExtCrystalUse(false);
    bnoReady = true;
  }
}

void loop() {
  // 2. Якщо при старті BNO055 не прокинувся — пробуємо підключити в циклі
  if (!bnoReady) {
    if (bno.begin(OPERATION_MODE_NDOF)) {
      bno.setExtCrystalUse(false);
      bnoReady = true;
    }
  } else {
    sensors_event_t event;
    bno.getEvent(&event);
    handData.yaw = event.orientation.x;
    handData.pitch = event.orientation.y;
    handData.roll = event.orientation.z;
  }

  // Відправляємо дані на Передпліччя
  LinkSerial.write((uint8_t*)&handData, sizeof(SensorData));
  delay(20);
}