#ifndef ARM_PACKETS_H
#define ARM_PACKETS_H

#include <Arduino.h>

#pragma pack(push, 1)
struct SensorData {
    float yaw;
    float pitch;
    float roll;
};

struct FullArmPacket {
    SensorData hand;
    SensorData forearm;
    SensorData shoulder;
};
#pragma pack(pop)

#endif