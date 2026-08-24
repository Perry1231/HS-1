#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include "ArmPackets.h"

#define SDA_PIN 3
#define SCL_PIN 2
#define TX_PIN  4  // Передача на Передпліччя (GPIO 4 -> GPIO 1 Передпліччя)

Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28, &Wire);
HardwareSerial LinkSerial(1);

SensorData handData;

void setup() {
  LinkSerial.begin(115200, SERIAL_8N1, -1, TX_PIN);
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

  // Шлемо 14 байт
  LinkSerial.write((uint8_t*)&handData, sizeof(SensorData));

  delay(20);
}