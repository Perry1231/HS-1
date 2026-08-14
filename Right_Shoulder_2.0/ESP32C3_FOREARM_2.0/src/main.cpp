#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include "ArmPackets.h"

#define SDA_PIN 3
#define SCL_PIN 2
#define RX_PIN  1 
#define TX_PIN  4  

Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28, &Wire);
HardwareSerial LinkSerial(1);

SensorData handDataBuffer;
SensorData forearmData;

void setup() {
  LinkSerial.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

  Wire.begin(SDA_PIN, SCL_PIN, 100000);

  if (bno.begin(OPERATION_MODE_NDOF)) {
    bno.setExtCrystalUse(false);
  }
}

void loop() {
  if (LinkSerial.available() >= sizeof(SensorData)) {
    LinkSerial.readBytes((uint8_t*)&handDataBuffer, sizeof(SensorData));
  }

 
  sensors_event_t event;
  bno.getEvent(&event);

  uint8_t sys, gyro, accel, mag;
  bno.getCalibration(&sys, &gyro, &accel, &mag);

  forearmData.yaw = event.orientation.x;
  forearmData.pitch = event.orientation.y;
  forearmData.roll = event.orientation.z;
  forearmData.calibSys = sys;
  forearmData.calibGyro = gyro;

  LinkSerial.write((uint8_t*)&handDataBuffer, sizeof(SensorData));
  LinkSerial.write((uint8_t*)&forearmData, sizeof(SensorData));

  delay(20); //50 Gz
}