#include <Arduino.h>

void setup() {
  Serial.begin(115200);

  // Очікування відкриття монітора порту (максимум 5 секунд)
  uint32_t startWait = millis();
  while (!Serial && (millis() - startWait < 5000)) {
    delay(10);
  }
  delay(500);

  Serial.println("\n\n==========================================");
  Serial.println("   ESP32-C3 USB CDC LINK ESTABLISHED!     ");
  Serial.println("==========================================\n");
}

void loop() {
  static uint32_t count = 0;
  count++;

  Serial.print("► Status: ONLINE | Packet #");
  Serial.print(count);
  Serial.print(" | Uptime: ");
  Serial.print(millis() / 1000);
  Serial.println("s");

  delay(1000);
}