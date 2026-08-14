#pragma once
#include <Arduino.h>


struct __attribute__((packed)) SensorData {
  float yaw;
  float pitch;
  float roll;
  uint8_t calibSys;
  uint8_t calibGyro;
};


struct __attribute__((packed)) FullArmPacket {
  uint8_t header = 0xAA;
  SensorData hand;      
  SensorData forearm;    
  SensorData shoulder;   
  uint8_t checksum;
};

inline uint8_t calculateChecksum(const uint8_t* data, size_t len) {
  uint8_t sum = 0;
  for (size_t i = 0; i < len; i++) sum ^= data[i];
  return sum;
}