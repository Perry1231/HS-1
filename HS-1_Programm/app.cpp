#include "raylib.h"
#include "stl_loader.h" // Наш кастомний парсер
#include <iostream>

int main() {
    InitWindow(1280, 720, "HS-1 Exoskeleton - Fixed STL Loader");
    SetTargetFPS(60);

    // Змінюємо робочу директорію на проєкт
    ChangeDirectory("C:/Users/Dell/Desktop/projetcs/GitHub Projects/HS-1/HS-1/HS-1_Programm");

    // Завантажуємо через наш надійний Binary STL парсер
    Model bodyModel = LoadBinarySTL("Models/Body_2_0.stl");
    bool isLoaded = (bodyModel.meshCount > 0);

    std::cout << "\n=================================" << std::endl;
    std::cout << "Model Status: " << (isLoaded ? "SUCCESS" : "FAILED") << std::endl;
    std::cout << "Mesh Count: " << bodyModel.meshCount << std::endl;
    std::cout << "=================================\n" << std::endl;

    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 50.0f, 200.0f };
    camera.target = (Vector3){ 0.0f, 20.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(DARKGRAY);

        BeginMode3D(camera);
            DrawGrid(20, 10.0f);
            if (isLoaded) {
                // Масштаб 0.05f (бо деталі з CAD зазвичай в міліметрах)
                DrawModel(bodyModel, (Vector3){0, 0, 0}, 0.05f, ORANGE);
                DrawModelWires(bodyModel, (Vector3){0, 0, 0}, 0.05f, BLACK);
            } else {
                DrawCube((Vector3){0, 15, 0}, 10.0f, 20.0f, 10.0f, RED);
            }
        EndMode3D();

        DrawText(isLoaded ? "STL LOADED SUCCESSFULLY!" : "FAILED TO LOAD STL", 10, 10, 20, isLoaded ? GREEN : RED);
        EndDrawing();
    }

    if (isLoaded) UnloadModel(bodyModel);
    CloseWindow();
    return 0;
}