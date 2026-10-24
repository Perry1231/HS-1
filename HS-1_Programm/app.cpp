#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "serial_reader.h"
#include <iostream>

int main() {
    InitWindow(1280, 720, "HS-1 Exoskeleton - Debug View");
    SetTargetFPS(60);

    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 50.0f, 200.0f }; // Віддалена камера
    camera.target = (Vector3){ 0.0f, 20.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Спробуємо завантажити деталь Корпусу
    Model bodyModel = LoadModel("Models/Body_2_0.stl");
    bool isModelLoaded = (bodyModel.meshCount > 0);

    if (isModelLoaded) {
        std::cout << "[SUCCESS] STL-модель Body_2_0.stl завантажена успішно!\n";
    } else {
        std::cout << "[ERROR] Не вдалося знайти Models/Body_2_0.stl!\n";
    }

    // Запускаємо Serial у фоні (якщо порту немає, програма НЕ зупиниться)
    SerialReader serial;
    serial.Start("COM3", 115200);

    float debugAngle = 0.0f;

    while (!WindowShouldClose()) {
        debugAngle += 0.5f; // Тестове автообертання костюма

        UpdateCamera(&camera, CAMERA_FREE);

        BeginDrawing();
        ClearBackground(DARKGRAY);

        BeginMode3D(camera);
            DrawGrid(50, 10.0f);

            if (isModelLoaded) {
                // Якщо деталь завантажилась — малюємо її із масштабом 0.1f (якщо з Fusion у мм)
                rlPushMatrix();
                    rlRotatef(debugAngle, 0, 1, 0); // Обертаємо для перевірки
                    DrawModel(bodyModel, (Vector3){0, 0, 0}, 0.1f, ORANGE);
                    DrawModelWires(bodyModel, (Vector3){0, 0, 0}, 0.1f, BLACK);
                rlPopMatrix();
            } else {
                // Якщо STL не знайдена — малюємо червоний тестовий куб
                DrawCube((Vector3){0, 15, 0}, 20.0f, 30.0f, 10.0f, RED);
                DrawCubeWires((Vector3){0, 15, 0}, 20.0f, 30.0f, 10.0f, WHITE);
            }
        EndMode3D();

        // Статус у кутку екрану
        DrawText(TextFormat("Serial: %s", serial.IsConnected() ? "CONNECTED" : "DISCONNECTED (OFFLINE)"), 10, 10, 18, serial.IsConnected() ? GREEN : RED);
        DrawText(isModelLoaded ? "STL Status: LOADED" : "STL Status: NOT FOUND (Drawing Test Red Cube)", 10, 35, 18, isModelLoaded ? GREEN : RED);
        DrawText("Затисни ПКМ + мишка для обертання камери", 10, 60, 15, LIGHTGRAY);

        EndDrawing();
    }

    serial.Stop();
    if (isModelLoaded) UnloadModel(bodyModel);
    CloseWindow();
    return 0;
}