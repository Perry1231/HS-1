#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include "ArmPackets.h"

#define SDA_PIN 3
#define SCL_PIN 2
#define TX_PIN  4  // Передача на Передпліччя (GPIO 4 -> GPIO 1)

Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28, &Wire);
HardwareSerial LinkSerial(1);

SensorData handData;

void setup() {
  // Налаштування UART (Тільки TX на GPIO 4)
  LinkSerial.begin(115200, SERIAL_8N1, -1, TX_PIN);

  // Налаштування I2C на GPIO 3 та GPIO 2
  Wire.begin(SDA_PIN, SCL_PIN, 100000);

  if (bno.begin(OPERATION_MODE_NDOF)) {
    bno.setExtCrystalUse(false);
  }
}

void loop() {
  sensors_event_t event;
  bno.getEvent(&event);

  uint8_t sys, gyro, accel, mag;
  bno.getCalibration(&sys, &gyro, &accel, &mag);

  handData.yaw = event.orientation.x;
  handData.pitch = event.orientation.y;
  handData.roll = event.orientation.z;
  handData.calibSys = sys;
  handData.calibGyro = gyro;

  // Відправляємо 14 байт далі по каскаду
  LinkSerial.write((uint8_t*)&handData, sizeof(handData));

  delay(20); // 50 Гц
}