#include "serial_reader.h"
#include "json.hpp" // nlohmann/json
#include <iostream>
#include <sstream>

using json = nlohmann::json;

SerialReader::SerialReader() 
    : hSerial(INVALID_HANDLE_VALUE), running(false), isConnected(false) {}

SerialReader::~SerialReader() {
    Stop();
}

bool SerialReader::Start(const std::string& portName, int baudRate) {
    if (running) return true;

    running = true;
    workerThread = std::thread(&SerialReader::WorkerThread, this, portName, baudRate);
    return true;
}

void SerialReader::Stop() {
    running = false;

    if (workerThread.joinable()) {
        workerThread.join();
    }

    if (hSerial != INVALID_HANDLE_VALUE) {
        CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
    }

    isConnected = false;
}

bool SerialReader::IsConnected() const {
    return isConnected;
}

JointTelemetry SerialReader::GetLatestData() {
    std::lock_guard<std::mutex> lock(dataMutex);
    return latestData;
}

void SerialReader::WorkerThread(std::string portName, int baudRate) {
    // Attempt to open COM Port
    std::string fullPortName = "\\\\.\\" + portName;
    hSerial = CreateFileA(fullPortName.c_str(), 
                          GENERIC_READ | GENERIC_WRITE, 
                          0, NULL, 
                          OPEN_EXISTING, 
                          FILE_ATTRIBUTE_NORMAL, 
                          NULL);

    if (hSerial == INVALID_HANDLE_VALUE) {
        std::cout << "[SerialReader] Port " << portName << " not found. Running in Offline Mode.\n";
        isConnected = false;
        
        // Wait silently while thread is running without blocking main thread
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        return;
    }

    // Configure DCB parameters
    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    if (!GetCommState(hSerial, &dcbSerialParams)) {
        std::cout << "[SerialReader] Error getting COM state.\n";
        CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
        isConnected = false;
        return;
    }

    dcbSerialParams.BaudRate = baudRate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity   = NOPARITY;

    if (!SetCommState(hSerial, &dcbSerialParams)) {
        std::cout << "[SerialReader] Error setting COM parameters.\n";
        CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
        isConnected = false;
        return;
    }

    // Set non-blocking timeouts
    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout         = 20;
    timeouts.ReadTotalTimeoutConstant    = 50;
    timeouts.ReadTotalTimeoutMultiplier  = 10;
    SetCommTimeouts(hSerial, &timeouts);

    isConnected = true;
    std::cout << "[SerialReader] Successfully connected to " << portName << " at " << baudRate << " baud.\n";

    std::string lineBuffer = "";
    char readBuffer[128];
    DWORD bytesRead = 0;

    while (running) {
        if (ReadFile(hSerial, readBuffer, sizeof(readBuffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
            readBuffer[bytesRead] = '\0';
            lineBuffer += readBuffer;

            size_t pos = 0;
            while ((pos = lineBuffer.find('\n')) != std::string::npos) {
                std::string line = lineBuffer.substr(0, pos);
                lineBuffer.erase(0, pos + 1);

                // Trim carriage return \r
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }

                // Parse incoming JSON telemetry string
                try {
                    auto j = json::parse(line);
                    JointTelemetry data;

                    if (j.contains("body")) {
                        data.bodyPitch = j["body"].value("pitch", 0.0f);
                        data.bodyRoll  = j["body"].value("roll", 0.0f);
                        data.bodyYaw   = j["body"].value("yaw", 0.0f);
                    }
                    if (j.contains("shoulder")) {
                        data.shoulderPitch = j["shoulder"].value("pitch", 0.0f);
                        data.shoulderRoll  = j["shoulder"].value("roll", 0.0f);
                    }
                    if (j.contains("forearm")) {
                        data.forearmPitch = j["forearm"].value("pitch", 0.0f);
                    }

                    // Thread-safe update
                    {
                        std::lock_guard<std::mutex> lock(dataMutex);
                        latestData = data;
                    }
                }
                catch (const json::parse_error&) {
                    // Ignore partial or corrupted serial lines
                }
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    CloseHandle(hSerial);
    hSerial = INVALID_HANDLE_VALUE;
    isConnected = false;
}