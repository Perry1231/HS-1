#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include "ArmPackets.h"

#define SDA_PIN 3
#define SCL_PIN 2
#define RX_PIN  1  // Прийом від Передпліччя (GPIO 1)

Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28, &Wire);
HardwareSerial LinkSerial(1);

FullArmPacket fullArm;

void setup() {
  Serial.begin(115200); // USB CDC для монітора порту
  
  // Очікуємо ініціалізацію Serial
  uint32_t startWait = millis();
  while (!Serial && (millis() - startWait < 3000)) delay(10);

  // Налаштування UART (Тільки RX на GPIO 1)
  LinkSerial.begin(115200, SERIAL_8N1, RX_PIN, -1);

  Wire.begin(SDA_PIN, SCL_PIN, 100000);

  if (bno.begin(OPERATION_MODE_NDOF)) {
    bno.setExtCrystalUse(false);
  }

  Serial.println("\n=======================================================");
  Serial.println("   ПЛЕЧОВИЙ ХАБ СТАРТУВАВ — КАСКАД ВУЗЛІВ АКТИВНИЙ    ");
  Serial.println("=======================================================\n");
}

void loop() {
  // 1. Приймаємо 2 пакети від каскаду (Кисть + Передпліччя)
  if (LinkSerial.available() >= (sizeof(SensorData) * 2)) {
    LinkSerial.readBytes((uint8_t*)&fullArm.hand, sizeof(SensorData));
    LinkSerial.readBytes((uint8_t*)&fullArm.forearm, sizeof(SensorData));
  }

  // 2. Зчитуємо свій BNO055 (Плече)
  sensors_event_t event;
  bno.getEvent(&event);

  uint8_t sys, gyro, accel, mag;
  bno.getCalibration(&sys, &gyro, &accel, &mag);

  fullArm.shoulder.yaw = event.orientation.x;
  fullArm.shoulder.pitch = event.orientation.y;
  fullArm.shoulder.roll = event.orientation.z;
  fullArm.shoulder.calibSys = sys;
  fullArm.shoulder.calibGyro = gyro;

  // 3. Обчислюємо контрольную суму
  fullArm.checksum = calculateChecksum((uint8_t*)&fullArm, sizeof(FullArmPacket) - 1);

  // 4. Форматований вивід усіх 3-х суглобів
  Serial.print("КИСТЬ [Y:"); Serial.print(fullArm.hand.yaw, 1);
  Serial.print("° P:"); Serial.print(fullArm.hand.pitch, 1);
  Serial.print("° R:"); Serial.print(fullArm.hand.roll, 1);
  
  Serial.print("°] | ПЕРЕДПЛІЧЧЯ [Y:"); Serial.print(fullArm.forearm.yaw, 1);
  Serial.print("° P:"); Serial.print(fullArm.forearm.pitch, 1);
  Serial.print("° R:"); Serial.print(fullArm.forearm.roll, 1);

  Serial.print("°] | ПЛЕЧЕ [Y:"); Serial.print(fullArm.shoulder.yaw, 1);
  Serial.print("° P:"); Serial.print(fullArm.shoulder.pitch, 1);
  Serial.print("° R:"); Serial.print(fullArm.shoulder.roll, 1);
  Serial.println("°]");

  delay(40); // ~25 Гц
}