#include "raylib.h"
#include "serial_reader.h"
#include <iostream>

using json = nlohmann::json;

struct EulerAngles {
    float p = 0, y = 0, r = 0;
};

struct RobotState {
    EulerAngles body;
    EulerAngles shoulder_1, forearm_1, hand_1;
    EulerAngles shoulder_2, forearm_2, hand_2;
    EulerAngles leg_higher_1, leg_lower_1, leg_foot_1;
    EulerAngles leg_higher_2, leg_lower_2, leg_foot_2;
} robotState;

int main() {
    const int screenWidth = 1280;
    const int screenHeight = 720;
    
    InitWindow(screenWidth, screenHeight, "HS-1 3D Visualization - C++ Raylib");
    SetTargetFPS(60);

    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 0.0f, -40.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Model bodyModel = LoadModel("Models/Body_2_0.stl");
    Model bicepsModel = LoadModel("Models/Biceps_2_0.stl");
    Model forearmModel = LoadModel("Models/Forearm_2_0.stl");
    Model handModel = LoadModel("Models/Hand_2_0.stl");

    // Запуск потоку Serial Reader
    SerialReader serial;
    serial.Start("COM3", 115200);

    float rotationSpeed = 2.0f;
    float moveSpeed = 0.5f;

    while (!WindowShouldClose()) {
        // --- A. UPDATE DATA FROM SERIAL ---
        json latestSensorData = serial.GetLatestData();
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
        }

        // --- B. CONTROLS ---
        if (IsKeyDown(KEY_T)) robotState.body.p += moveSpeed;
        if (IsKeyDown(KEY_G)) robotState.body.p -= moveSpeed;
        if (IsKeyDown(KEY_B)) robotState.body.y += rotationSpeed;
        if (IsKeyDown(KEY_V)) robotState.body.y -= rotationSpeed;

        if (IsKeyPressed(KEY_SPACE)) {
            robotState = RobotState{};
        }

        UpdateCamera(&camera, CAMERA_FREE);

        // --- C. RENDERING ---
        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);
        DrawGrid(20, 10.0f);
        DrawModelEx(bodyModel, (Vector3){0, 0, 0}, (Vector3){0, 1, 0}, robotState.body.y, (Vector3){1, 1, 1}, ORANGE);
        EndMode3D();

        // HUD
        DrawText("HS-1 Exoskeleton Telemetry - C++ Mode", 10, 10, 20, DARKGRAY);
        DrawText(TextFormat("Serial Status: %s", serial.IsConnected() ? "CONNECTED" : "DISCONNECTED"), 10, 40, 16, serial.IsConnected() ? GREEN : RED);

        EndDrawing();
    }

    // Зупиняємо потік перед закриттям
    serial.Stop();

    UnloadModel(bodyModel);
    UnloadModel(bicepsModel);
    UnloadModel(forearmModel);
    UnloadModel(handModel);

    CloseWindow();
    return 0;
}