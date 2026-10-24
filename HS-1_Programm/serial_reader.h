#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <windows.h>

// Structure to store raw joint data read from Serial JSON
struct JointTelemetry {
    float bodyPitch = 0.0f;
    float bodyRoll  = 0.0f;
    float bodyYaw   = 0.0f;

    float shoulderPitch = 0.0f;
    float shoulderRoll  = 0.0f;

    float forearmPitch  = 0.0f;
};

class SerialReader {
public:
    SerialReader();
    ~SerialReader();

    // Start background thread for listening to the COM port
    bool Start(const std::string& portName, int baudRate = 115200);

    // Stop background thread and release COM handle
    void Stop();

    // Check if COM port is currently opened and reading
    bool IsConnected() const;

    // Thread-safe method to retrieve latest telemetry data
    JointTelemetry GetLatestData();

private:
    void WorkerThread(std::string portName, int baudRate);

    HANDLE hSerial;
    std::atomic<bool> running;
    std::atomic<bool> isConnected;

    std::mutex dataMutex;
    JointTelemetry latestData;
    std::thread workerThread;
};