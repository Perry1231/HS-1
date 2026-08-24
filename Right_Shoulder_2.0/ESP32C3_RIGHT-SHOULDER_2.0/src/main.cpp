#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include "ArmPackets.h"

#define SDA_PIN 3
#define SCL_PIN 2
#define RX_PIN  1  // Прийом від Передпліччя

Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28, &Wire);
HardwareSerial LinkSerial(1);

FullArmPacket fullArm;

void setup() {
  Serial.begin(115200);

  uint32_t startWait = millis();
  while (!Serial && (millis() - startWait < 3000)) delay(10);

  LinkSerial.begin(115200, SERIAL_8N1, RX_PIN, -1);
  Wire.begin(SDA_PIN, SCL_PIN, 100000);

  if (bno.begin(OPERATION_MODE_NDOF)) {
    bno.setExtCrystalUse(false);
  }
}

void loop() {
  // 1. Приймаємо 28 байт від каскаду (Кисть + Передпліччя)
  if (LinkSerial.available() >= (sizeof(SensorData) * 2)) {
    LinkSerial.readBytes((uint8_t*)&fullArm.hand, sizeof(SensorData));
    LinkSerial.readBytes((uint8_t*)&fullArm.forearm, sizeof(SensorData));
  }

  // 2. Зчитуємо власний BNO055
  sensors_event_t event;
  bno.getEvent(&event);
  uint8_t sys, gyro, accel, mag;
  bno.getCalibration(&sys, &gyro, &accel, &mag);

  fullArm.shoulder.yaw = event.orientation.x;
  fullArm.shoulder.pitch = event.orientation.y;
  fullArm.shoulder.roll = event.orientation.z;

  // 3. Відправляємо JSON у ПК
  Serial.printf("{\"shoulder\":{\"p\":%.1f,\"y\":%.1f,\"r\":%.1f},\"forearm\":{\"p\":%.1f,\"y\":%.1f,\"r\":%.1f},\"hand\":{\"p\":%.1f,\"y\":%.1f,\"r\":%.1f}}\n",
                fullArm.shoulder.pitch, fullArm.shoulder.yaw, fullArm.shoulder.roll,
                fullArm.forearm.pitch, fullArm.forearm.yaw, fullArm.forearm.roll,
                fullArm.hand.pitch, fullArm.hand.yaw, fullArm.hand.roll);

  delay(20);
}