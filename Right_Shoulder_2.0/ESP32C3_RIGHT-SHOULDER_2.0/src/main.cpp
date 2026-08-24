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

// Малювання візуальної шкали кута [-180° ... +180°]
void drawBar(float angle, int width = 10) {
  float norm = (angle + 180.0) / 360.0;
  if (norm < 0.0) norm = 0.0;
  if (norm > 1.0) norm = 1.0;
  int pos = norm * width;

  Serial.print("[");
  for (int i = 0; i <= width; i++) {
    if (i == pos) Serial.print("◆");
    else if (i == width / 2) Serial.print("│");
    else Serial.print("─");
  }
  Serial.print("]");
}

// Статус калібрування BNO055
void printCalibBadge(uint8_t sys, uint8_t gyro) {
  Serial.print(" [S:"); Serial.print(sys);
  Serial.print(" G:"); Serial.print(gyro); Serial.print("]");
  if (sys == 3 && gyro == 3) Serial.print(" ✅ (OK)");
  else if (sys > 0)          Serial.print(" ⚠️ (CAL)");
  else                       Serial.print(" ❌ (INIT)");
}

void setup() {
  Serial.begin(115200);

  uint32_t startWait = millis();
  while (!Serial && (millis() - startWait < 3000)) delay(10);

  LinkSerial.begin(115200, SERIAL_8N1, RX_PIN, -1);
  Wire.begin(SDA_PIN, SCL_PIN, 100000);

  if (bno.begin(OPERATION_MODE_NDOF)) {
    bno.setExtCrystalUse(false);
  }

  // Очищення екрана при старті
  Serial.print("\033[2J\033[H");
}

void loop() {
  // 1. Прийом від каскаду (Кисть + Передпліччя)
  if (LinkSerial.available() >= (sizeof(SensorData) * 2)) {
    LinkSerial.readBytes((uint8_t*)&fullArm.hand, sizeof(SensorData));
    LinkSerial.readBytes((uint8_t*)&fullArm.forearm, sizeof(SensorData));
  }

  // 2. Власний датчик (Плече)
  sensors_event_t event;
  bno.getEvent(&event);
  uint8_t sys, gyro, accel, mag;
  bno.getCalibration(&sys, &gyro, &accel, &mag);

  fullArm.shoulder.yaw = event.orientation.x;
  fullArm.shoulder.pitch = event.orientation.y;
  fullArm.shoulder.roll = event.orientation.z;
  fullArm.shoulder.calibSys = sys;
  fullArm.shoulder.calibGyro = gyro;

  // 3. ANSI код: повернення курсора в лівий верхній кут без мерехтіння
  Serial.print("\033[H");

  char buf[80];

  Serial.println("╔══════════════════════════════════════════════════════════════════════════╗");
  Serial.println("║                     KINEMATIC SUIT — ARM HUD                             ║");
  Serial.println("╠══════════════╦═══════════════════════╦═══════════════════════╦══════════════╣");
  Serial.println("║ ВУЗОЛ        ║ YAW (Курс)            ║ PITCH (Тангаж)        ║ КАЛІБРУВАННЯ ║");
  Serial.println("╠══════════════╬═══════════════════════╬═══════════════════════╬══════════════╣");

  // --- КИСТЬ ---
  snprintf(buf, sizeof(buf), "║ 🖐️  КИСТЬ    ║ %6.1f° ", fullArm.hand.yaw); Serial.print(buf);
  drawBar(fullArm.hand.yaw - 180.0);
  snprintf(buf, sizeof(buf), " ║ %6.1f° ", fullArm.hand.pitch); Serial.print(buf);
  drawBar(fullArm.hand.pitch);
  Serial.print(" ║");
  printCalibBadge(fullArm.hand.calibSys, fullArm.hand.calibGyro);
  Serial.println("    ║");

  // --- ПЕРЕДПЛІЧЧЯ ---
  snprintf(buf, sizeof(buf), "║ 🦾 ПЕРЕДПЛ   ║ %6.1f° ", fullArm.forearm.yaw); Serial.print(buf);
  drawBar(fullArm.forearm.yaw - 180.0);
  snprintf(buf, sizeof(buf), " ║ %6.1f° ", fullArm.forearm.pitch); Serial.print(buf);
  drawBar(fullArm.forearm.pitch);
  Serial.print(" ║");
  printCalibBadge(fullArm.forearm.calibSys, fullArm.forearm.calibGyro);
  Serial.println("    ║");

  // --- ПЛЕЧЕ ---
  snprintf(buf, sizeof(buf), "║ 🦾 ПЛЕЧЕ     ║ %6.1f° ", fullArm.shoulder.yaw); Serial.print(buf);
  drawBar(fullArm.shoulder.yaw - 180.0);
  snprintf(buf, sizeof(buf), " ║ %6.1f° ", fullArm.shoulder.pitch); Serial.print(buf);
  drawBar(fullArm.shoulder.pitch);
  Serial.print(" ║");
  printCalibBadge(fullArm.shoulder.calibSys, fullArm.shoulder.calibGyro);
  Serial.println("    ║");

  Serial.println("╚══════════════╩═══════════════════════╩═══════════════════════╩══════════════╝");

  // -----------------------------------------------------------------------------------
  // 4. ВІДПРАВКА ОДНОРЯДКОВОГО JSON ДЛЯ PYTHON УРСІНИ
  // -----------------------------------------------------------------------------------
  Serial.printf("{\"shoulder\":{\"p\":%.1f,\"y\":%.1f,\"r\":%.1f},\"forearm\":{\"p\":%.1f,\"y\":%.1f,\"r\":%.1f},\"hand\":{\"p\":%.1f,\"y\":%.1f,\"r\":%.1f}}\n",
                fullArm.shoulder.pitch, fullArm.shoulder.yaw, fullArm.shoulder.roll,
                fullArm.forearm.pitch, fullArm.forearm.yaw, fullArm.forearm.roll,
                fullArm.hand.pitch, fullArm.hand.yaw, fullArm.hand.roll);

  delay(60); // ~15-20 FPS
}