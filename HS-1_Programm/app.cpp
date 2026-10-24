#include "raylib.h"
#include "json.hpp" // nlohmann/json
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <fstream>

#if defined(_WIN32)
#include <windows.h>
#endif

using json = nlohmann::json;

// Global variables for the Serial reading thread and kinematic hierarchy states
std::mutex dataMutex;
json latestSensorData;
bool useSerial = false;

// Structure to store Euler angles (Pitch, Yaw, Roll)
struct EulerAngles {
    float p = 0, y = 0, r = 0;
};

// Store current angles for each body part
struct RobotState {
    EulerAngles body;
    EulerAngles shoulder_1, forearm_1, hand_1;
    EulerAngles shoulder_2, forearm_2, hand_2;
    EulerAngles leg_higher_1, leg_lower_1, leg_foot_1;
    EulerAngles leg_higher_2, leg_lower_2, leg_foot_2;
} robotState;

// Serial port reading thread (to prevent freezing the render loop)
void SerialReaderThread(std::string portName, int baudRate) {
#if defined(_WIN32)
    HANDLE hSerial = CreateFileA(portName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hSerial == INVALID_HANDLE_VALUE) {
        std::cout << "Failed to open " << portName << "\n";
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

    useSerial = true;
    std::cout << "Successfully connected to " << portName << "!\n";

    char szBuff[256];
    std::string lineBuffer = "";

    while (useSerial) {
        DWORD dwBytesRead = 0;
        if (ReadFile(hSerial, szBuff, sizeof(szBuff) - 1, &dwBytesRead, NULL) && dwBytesRead > 0) {
            szBuff[dwBytesRead] = '\0';
            lineBuffer += szBuff;

            size_t pos;
            while ((pos = lineBuffer.find('\n')) != std::string::npos) {
                std::string line = lineBuffer.substr(0, pos);
                lineBuffer.erase(0, pos + 1);

                // Parse JSON line
                if (!line.empty() && line.front() == '{' && line.back() == '}') {
                    try {
                        auto j = json::parse(line);
                        std::lock_guard<std::mutex> lock(dataMutex);
                        latestSensorData = j;
                    } catch (...) {
                        // Ignore parsing errors to keep the thread running
                    }
                }
            }
        }
    }
    CloseHandle(hSerial);
#endif
}

int main() {
    // 1. Initialize Raylib window
    const int screenWidth = 1280;
    const int screenHeight = 720;
    InitWindow(screenWidth, screenHeight, "HS-1 3D Visualization - C++ Raylib");
    SetTargetFPS(60);

    // 2. Camera setup (equivalent to EditorCamera in Ursina)
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 0.0f, -40.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // 3. Load 3D Models (STL)
    Model bodyModel = LoadModel("Models/Body_2_0.stl");
    Model bicepsModel = LoadModel("Models/Biceps_2_0.stl");
    Model forearmModel = LoadModel("Models/Forearm_2_0.stl");
    Model handModel = LoadModel("Models/Hand_2_0.stl");

    // Launch COM port reading thread ("COM3")
    std::thread serialThread(SerialReaderThread, "COM3", 115200);
    serialThread.detach();

    float rotationSpeed = 2.0f;
    float moveSpeed = 0.5f;

    // Main application loop
    while (!WindowShouldClose()) {
        // --- A. UPDATE DATA FROM SERIAL ---
        {
            std::lock_guard<std::mutex> lock(dataMutex);
            if (!latestSensorData.is_null()) {
                if (latestSensorData.contains("body")) {
                    robotState.body.p = latestSensorData["body"]["p"];
                    robotState.body.y = latestSensorData["body"]["y"];
                    robotState.body.r = latestSensorData["body"]["r"];
                }
                if (latestSensorData.contains("shoulder")) {
                    robotState.shoulder_1.p = latestSensorData["shoulder"]["p"];
                    robotState.shoulder_1.y = latestSensorData["shoulder"]["y"];
                    robotState.shoulder_1.r = latestSensorData["shoulder"]["r"];
                }
                // You can map the remaining body parts similarly...
            }
        }

        // --- B. MANUAL KEYBOARD CONTROLS (Replicating Python logic) ---
        if (IsKeyDown(KEY_T)) robotState.body.p += moveSpeed;
        if (IsKeyDown(KEY_G)) robotState.body.p -= moveSpeed;
        if (IsKeyDown(KEY_B)) robotState.body.y += rotationSpeed;
        if (IsKeyDown(KEY_V)) robotState.body.y -= rotationSpeed;

        // Reset pose on Spacebar
        if (IsKeyPressed(KEY_SPACE)) {
            robotState = RobotState{}; // Reset all angles to zero
        }

        // Free camera control with mouse (when right click is held)
        UpdateCamera(&camera, CAMERA_FREE);

        // --- C. RENDERING ---
        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);

        // Draw floor grid
        DrawGrid(20, 10.0f);

        // Hierarchical rendering using transformation matrices
        // Example: Drawing the Body model
        DrawModelEx(bodyModel, (Vector3){0, 0, 0}, (Vector3){0, 1, 0}, robotState.body.y, (Vector3){1, 1, 1}, ORANGE);

        EndMode3D();

        // Heads-Up Display (HUD)
        DrawText("HS-1 Exoskeleton Telemetry - C++ Mode", 10, 10, 20, DARKGRAY);
        DrawText(TextFormat("Serial Status: %s", useSerial ? "CONNECTED" : "DISCONNECTED"), 10, 40, 16, useSerial ? GREEN : RED);

        EndDrawing();
    }

    // Cleanup memory
    UnloadModel(bodyModel);
    UnloadModel(bicepsModel);
    UnloadModel(forearmModel);
    UnloadModel(handModel);

    CloseWindow();
    return 0;
}