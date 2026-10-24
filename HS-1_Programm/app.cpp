#include "raylib.h"
#include <iostream>

int main() {
    InitWindow(1280, 720, "HS-1 - Relative Path Test");
    SetTargetFPS(60);

    // Змінюємо робочу директорію Raylib прямо на папку з проєктом
    ChangeDirectory("C:/Users/Dell/Desktop/projetcs/GitHub Projects/HS-1/HS-1/HS-1_Programm");

    // Тепер завантажуємо через ВІДНОСНИЙ шлях (без пробілів у рядку)
    Model bodyModel = LoadModel("Models/Body_2_0.stl");
    bool isLoaded = (bodyModel.meshCount > 0);

    std::cout << "\n=================================" << std::endl;
    std::cout << "Working Dir: " << GetWorkingDirectory() << std::endl;
    std::cout << "Model Loaded Status: " << (isLoaded ? "SUCCESS" : "FAILED") << std::endl;
    std::cout << "Mesh Count: " << bodyModel.meshCount << std::endl;
    std::cout << "=================================\n" << std::endl;

    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 20.0f, 100.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(DARKGRAY);
        BeginMode3D(camera);
            DrawGrid(20, 10.0f);
            if (isLoaded) {
                DrawModel(bodyModel, (Vector3){0, 0, 0}, 0.05f, ORANGE);
            } else {
                DrawCube((Vector3){0, 10, 0}, 10.0f, 20.0f, 10.0f, RED);
            }
        EndMode3D();

        DrawText(isLoaded ? "SUCCESS!" : "FAILED TO LOAD STL", 10, 10, 20, isLoaded ? GREEN : RED);
        EndDrawing();
    }

    if (isLoaded) UnloadModel(bodyModel);
    CloseWindow();
    return 0;
}