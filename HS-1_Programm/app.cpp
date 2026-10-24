#include "raylib.h"
#include "rlgl.h"
#include "stl_loader.h"
#include <iostream>

int main() {
    InitWindow(1280, 720, "HS-1 Exoskeleton - 3D Viewer");
    SetTargetFPS(60);

    ChangeDirectory("C:/Users/Dell/Desktop/projetcs/GitHub Projects/HS-1/HS-1/HS-1_Programm");

    Model bodyModel = LoadBinarySTL("Models/Body_2_0.stl");
    bool isLoaded = (bodyModel.meshCount > 0);

    // Налаштування камери
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 30.0f, 150.0f }; // Віддалена позиція
    camera.target = (Vector3){ 0.0f, 20.0f, 0.0f };     // Точка фокусу
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    float scale = 0.05f; // Масштаб деталі (з мм у м)

    while (!WindowShouldClose()) {
        // --- 1. КЕРУВАННЯ КАМЕРОЮ ---
        // Обертання камери затиснутою ПРАВОЮ кнопкою миші (ПКМ)
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            UpdateCamera(&camera, CAMERA_FREE);
        }

        // Скидання камери на клавішу R
        if (IsKeyPressed(KEY_R)) {
            camera.position = (Vector3){ 0.0f, 30.0f, 150.0f };
            camera.target = (Vector3){ 0.0f, 20.0f, 0.0f };
        }

        // --- 2. РЕНДЕР ---
        BeginDrawing();
        ClearBackground(DARKGRAY);

        BeginMode3D(camera);
            DrawGrid(50, 10.0f); // Сітка координат

            if (isLoaded) {
                rlPushMatrix();
                    // Піднімаємо деталь трохи вище сітки
                    rlTranslatef(0.0f, 10.0f, 0.0f);

                    // ГОЛОВНЕ: Повертаємо деталь на -90 градусів навколо осі X,
                    // щоб підняти її з підлоги (Z-up -> Y-up)
                    rlRotatef(-90.0f, 1.0f, 0.0f, 0.0f);

                    // Відрисовуємо модель та її каркас (wires)
                    DrawModel(bodyModel, (Vector3){0, 0, 0}, scale, ORANGE);
                    DrawModelWires(bodyModel, (Vector3){0, 0, 0}, scale, BLACK);
                rlPopMatrix();
            }
        EndMode3D();

        // Панель керування
        DrawText("HS-1 Exoskeleton - Body View", 10, 10, 20, RAYWHITE);
        DrawText("Hold RIGHT MOUSE BUTTON + Move Mouse to rotate camera", 10, 40, 14, LIGHTGRAY);
        DrawText("Press [R] to reset camera view", 10, 60, 14, LIGHTGRAY);

        EndDrawing();
    }

    if (isLoaded) UnloadModel(bodyModel);
    CloseWindow();
    return 0;
}