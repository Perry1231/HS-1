#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <iostream>

// Структура кутів для керування суглобами
struct EulerAngles { float p = 0, y = 0, r = 0; };

struct RobotState {
    EulerAngles body;
    EulerAngles shoulder;
    EulerAngles forearm;
    EulerAngles hand;
} robot;

int main() {
    InitWindow(1280, 720, "HS-1 Exoskeleton - 3D Viewer Mode");
    SetTargetFPS(60);

    // 1. Камера зафіксована на зручній відстані від костюма
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 20.0f, 120.0f };
    camera.target = (Vector3){ 0.0f, 10.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // 2. Завантаження 3D-моделей за прямим шляхом
    // (Якщо назви файлів відрізняються — підкоригуй їх нижче)
    // Абсолютний шлях до кожної моделі
Model bodyModel    = LoadModel("Models/Body_2_0.stl");
Model bicepsModel  = LoadModel("Models/Biceps_2_0.stl");
Model forearmModel = LoadModel("Models/Forearm_2_0.stl");
Model handModel    = LoadModel("Models/Hand_2_0.stl");

// Перевірка: модель вважається завантаженою, якщо meshCount > 0
bool hasBody = (bodyModel.meshCount > 0 && bodyModel.meshes != NULL);

    float autoRotate = 0.0f;

    while (!WindowShouldClose()) {
        // --- A. КЕРУВАННЯ ---
        // Автоматичне повільне обертання для огляду
        autoRotate += 0.5f;

        // Вільний огляд мишкою ТІЛЬКИ при затиснутій правій кнопці миші (ПКМ)
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            UpdateCamera(&camera, CAMERA_FREE);
        }

        // Натисни R, якщо камера вилетіла
        if (IsKeyPressed(KEY_R)) {
            camera.position = (Vector3){ 0.0f, 20.0f, 120.0f };
            camera.target = (Vector3){ 0.0f, 10.0f, 0.0f };
        }

        // --- B. ВІДОБРАЖЕННЯ (RENDERING) ---
        BeginDrawing();
        ClearBackground(DARKGRAY);

        BeginMode3D(camera);
            DrawGrid(40, 5.0f); // Сітка під костюмом

            if (hasBody) {
                // Ієрархічний рендеринг екзоскелета
                rlPushMatrix();
                    // Загальне обертання костюма навколо своєї осі
                    rlRotatef(autoRotate, 0, 1, 0);

                    // 1. Корпус
                    DrawModel(bodyModel, (Vector3){0, 0, 0}, 0.05f, ORANGE);
                    DrawModelWires(bodyModel, (Vector3){0, 0, 0}, 0.05f, BLACK);

                    // 2. Плече / Біцепс (зміщення від центру корпусу)
                    rlPushMatrix();
                        rlTranslatef(10.0f, 15.0f, 0.0f);
                        DrawModel(bicepsModel, (Vector3){0, 0, 0}, 0.05f, LIGHTGRAY);
                        DrawModelWires(bicepsModel, (Vector3){0, 0, 0}, 0.05f, DARKGRAY);

                        // 3. Передпліччя (зміщення від ліктя)
                        rlPushMatrix();
                            rlTranslatef(0.0f, -12.0f, 0.0f);
                            DrawModel(forearmModel, (Vector3){0, 0, 0}, 0.05f, GRAY);
                            DrawModelWires(forearmModel, (Vector3){0, 0, 0}, 0.05f, BLACK);

                            // 4. Кисть (зміщення від зап'ястя)
                            rlPushMatrix();
                                rlTranslatef(0.0f, -10.0f, 0.0f);
                                DrawModel(handModel, (Vector3){0, 0, 0}, 0.05f, DARKGRAY);
                            rlPopMatrix();

                        rlPopMatrix();
                    rlPopMatrix();

                rlPopMatrix();
            } else {
                // Якщо STL-файли не знайдені — малюємо макет костюма з блоків
                DrawCube((Vector3){0, 15, 0}, 15.0f, 25.0f, 8.0f, ORANGE);       // Тулуб
                DrawCubeWires((Vector3){0, 15, 0}, 15.0f, 25.0f, 8.0f, WHITE);
                
                DrawCube((Vector3){12, 22, 0}, 6.0f, 15.0f, 6.0f, LIGHTGRAY);    // Праве плече
                DrawCube((Vector3){-12, 22, 0}, 6.0f, 15.0f, 6.0f, LIGHTGRAY);   // Ліве плече
            }
        EndMode3D();

        // Текстова панель
        DrawText("HS-1 Exoskeleton - Offline 3D Mode", 10, 10, 20, RAYWHITE);
        DrawText(hasBody ? "STL Models: LOADED" : "STL Models: NOT FOUND (Drawing Placeholder Box)", 10, 35, 16, hasBody ? GREEN : RED);
        DrawText("Затисни ПКМ для обертання камери | [R] - скинути камеру", 10, 60, 14, LIGHTGRAY);

        EndDrawing();
    }

    // Очищення пам'яті
    if (hasBody) {
        UnloadModel(bodyModel);
        UnloadModel(bicepsModel);
        UnloadModel(forearmModel);
        UnloadModel(handModel);
    }

    CloseWindow();
    return 0;
}