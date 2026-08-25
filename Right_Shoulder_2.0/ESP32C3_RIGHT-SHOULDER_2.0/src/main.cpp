#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include "ArmPackets.h"

#define SDA_PIN 3
#define SCL_PIN 2
#define RX_PIN  1

Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28, &Wire);
HardwareSerial LinkSerial(1);
FullArmPacket fullArm;

void setup() {
  Serial.begin(115200);
  LinkSerial.begin(115200, SERIAL_8N1, RX_PIN, -1);
  // Додати у setup() Плеча та Передпліччя ПЕРЕД Wire.begin()
pinMode(SDA_PIN, INPUT_PULLUP);
pinMode(SCL_PIN, OUTPUT);

for (int i = 0; i < 10; i++) {
  digitalWrite(SCL_PIN, HIGH);
  delayMicroseconds(5);
  digitalWrite(SCL_PIN, LOW);
  delayMicroseconds(5);
}


  Wire.begin(SDA_PIN, SCL_PIN, 100000);
  if (bno.begin(OPERATION_MODE_NDOF)) {
    bno.setExtCrystalUse(false);
  }
}

void loop() {
  // Зчитуємо ровно 24 байти (Hand + Forearm)
  while (LinkSerial.available() >= (sizeof(SensorData) * 2)) {
    LinkSerial.readBytes((uint8_t*)&fullArm.hand, sizeof(SensorData));
    LinkSerial.readBytes((uint8_t*)&fullArm.forearm, sizeof(SensorData));
  }

  // Власний BNO055 Плеча
  sensors_event_t event;
  bno.getEvent(&event);
  fullArm.shoulder.yaw = event.orientation.x;
  fullArm.shoulder.pitch = event.orientation.y;
  fullArm.shoulder.roll = event.orientation.z;

  // Фільтр від сміття
  if (abs(fullArm.hand.roll) < 361.0 && abs(fullArm.forearm.roll) < 361.0) {
    Serial.printf("{\"shoulder\":{\"p\":%.1f,\"y\":%.1f,\"r\":%.1f},\"forearm\":{\"p\":%.1f,\"y\":%.1f,\"r\":%.1f},\"hand\":{\"p\":%.1f,\"y\":%.1f,\"r\":%.1f}}\n",
                  fullArm.shoulder.pitch, fullArm.shoulder.yaw, fullArm.shoulder.roll,
                  fullArm.forearm.pitch, fullArm.forearm.yaw, fullArm.forearm.roll,
                  fullArm.hand.pitch, fullArm.hand.yaw, fullArm.hand.roll);
  }

  delay(20);
}