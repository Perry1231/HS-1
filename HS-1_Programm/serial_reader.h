#ifndef SERIAL_READER_H
#define SERIAL_READER_H

#include <string>
#include <mutex>
#include <atomic>
#include <thread>
#include <nlohmann/json.hpp>

class SerialReader {
public:
    SerialReader();
    ~SerialReader();

    bool Start(const std::string& portName, int baudRate);
    void Stop();
    bool IsConnected() const { return isConnected; }
    
    nlohmann::json GetLatestData();

private:
    void WorkerThread(std::string portName, int baudRate);

    std::thread worker;
    std::atomic<bool> running{false};
    std::atomic<bool> isConnected{false};
    
    std::mutex dataMutex;
    nlohmann::json latestData;
};

#endif // SERIAL_READER_H