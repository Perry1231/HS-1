#include "serial_reader.h"
#include <windows.h>
#include <iostream>

SerialReader::SerialReader() = default;

SerialReader::~SerialReader() {
    Stop();
}

bool SerialReader::Start(const std::string& portName, int baudRate) {
    if (running) return false;
    running = true;
    worker = std::thread(&SerialReader::WorkerThread, this, portName, baudRate);
    return true;
}

void SerialReader::Stop() {
    if (running) {
        running = false;
        if (worker.joinable()) {
            worker.join();
        }
        isConnected = false;
    }
}

nlohmann::json SerialReader::GetLatestData() {
    std::lock_guard<std::mutex> lock(dataMutex);
    return latestData;
}

void SerialReader::WorkerThread(std::string portName, int baudRate) {
    HANDLE hSerial = CreateFileA(portName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    
    if (hSerial == INVALID_HANDLE_VALUE) {
        std::cout << "[Serial] Cannot open " << portName << ". Running in visual-only mode.\n";
        isConnected = false;
        running = false; 
        return;
    }


    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    GetCommState(hSerial, &dcbSerialParams);
    dcbSerialParams.BaudRate = baudRate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    SetCommState(hSerial, &dcbSerialParams);

    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    SetCommTimeouts(hSerial, &timeouts);

    isConnected = true;
    std::cout << "[Serial] Connected to " << portName << "\n";

    char szBuff[256];
    std::string lineBuffer = "";

    while (running) {
        DWORD dwBytesRead = 0;
        if (ReadFile(hSerial, szBuff, sizeof(szBuff) - 1, &dwBytesRead, NULL) && dwBytesRead > 0) {
            szBuff[dwBytesRead] = '\0';
            lineBuffer += szBuff;

            size_t pos;
            while ((pos = lineBuffer.find('\n')) != std::string::npos) {
                std::string line = lineBuffer.substr(0, pos);
                lineBuffer.erase(0, pos + 1);

                if (!line.empty() && line.front() == '{' && line.back() == '}') {
                    try {
                        auto j = nlohmann::json::parse(line);
                        std::lock_guard<std::mutex> lock(dataMutex);
                        latestData = j;
                    } catch (...) {}
                }
            }
        }
    }

    CloseHandle(hSerial);
    isConnected = false;
}