#include "raylib.h"
#include "rlgl.h"
#include "stl_loader.h"
#include <iostream>

int main() {
    InitWindow(1280, 720, "HS-1 Exoskeleton - Manual Adjustment Mode");
    SetTargetFPS(60);

    // Set working directory to project root
    ChangeDirectory("C:/Users/Dell/Desktop/projetcs/GitHub Projects/HS-1/HS-1/HS-1_Programm");

    // Load body STL model via custom binary loader
    Model bodyModel = LoadBinarySTL("Models/Body_2_0.stl");
    bool isLoaded = (bodyModel.meshCount > 0);

    // Camera setup
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 30.0f, 150.0f };
    camera.target = (Vector3){ 0.0f, 20.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // =========================================================================
    // MANUAL ADJUSTMENT VARIABLES (Edit default values here or use keyboard)
    // =========================================================================
    Vector3 bodyPos = { 0.0f, 10.0f, 0.0f };   // Position: [X (left/right), Y (up/down), Z (front/back)]
    float rotX = -90.0f;                       // Pitch rotation around X axis (Flips model up from floor)
    float rotY = 0.0f;                         // Yaw rotation around Y axis
    float rotZ = 0.0f;                         // Roll rotation around Z axis
    float modelScale = 0.05f;                  // Scale factor (0.05f scales mm to meter ratio for Raylib)

    while (!WindowShouldClose()) {
        // --- 1. CAMERA CONTROLS ---
        // Hold Right Mouse Button (RMB) and drag mouse to orbit around model
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            UpdateCamera(&camera, CAMERA_FREE);
        }

        // Reset camera position on key [R]
        if (IsKeyPressed(KEY_R)) {
            camera.position = (Vector3){ 0.0f, 30.0f, 150.0f };
            camera.target = (Vector3){ 0.0f, 20.0f, 0.0f };
        }

        // --- 2. LIVE MANUAL ADJUSTMENT CONTROLS ---
        // Rotation adjustments
        if (IsKeyDown(KEY_W)) rotX += 1.0f; // Tilt forward/backward
        if (IsKeyDown(KEY_S)) rotX -= 1.0f;
        if (IsKeyDown(KEY_A)) rotY += 1.0f; // Turn left/right
        if (IsKeyDown(KEY_D)) rotY -= 1.0f;
        if (IsKeyDown(KEY_Q)) rotZ += 1.0f; // Roll left/right
        if (IsKeyDown(KEY_E)) rotZ -= 1.0f;

        // Position adjustments
        if (IsKeyDown(KEY_UP))    bodyPos.y += 0.5f; // Move UP
        if (IsKeyDown(KEY_DOWN))  bodyPos.y -= 0.5f; // Move DOWN
        if (IsKeyDown(KEY_LEFT))  bodyPos.x -= 0.5f; // Move LEFT
        if (IsKeyDown(KEY_RIGHT)) bodyPos.x += 0.5f; // Move RIGHT

        // Scale adjustments
        if (IsKeyDown(KEY_KP_ADD) || IsKeyDown(KEY_EQUAL))        modelScale += 0.001f; // Scale UP (+)
        if (IsKeyDown(KEY_KP_SUBTRACT) || IsKeyDown(KEY_MINUS))  modelScale -= 0.001f; // Scale DOWN (-)

        // --- 3. RENDERING ---
        BeginDrawing();
        ClearBackground(DARKGRAY);

        BeginMode3D(camera);
            DrawGrid(50, 10.0f); // Ground alignment grid

            if (isLoaded) {
                rlPushMatrix();
                    // Step A: Translate model to target world position
                    rlTranslatef(bodyPos.x, bodyPos.y, bodyPos.z);

                    // Step B: Apply rotations in order (X, Y, Z)
                    rlRotatef(rotX, 1.0f, 0.0f, 0.0f);
                    rlRotatef(rotY, 0.0f, 1.0f, 0.0f);
                    rlRotatef(rotZ, 0.0f, 0.0f, 1.0f);

                    // Step C: Draw mesh and wireframe overlay
                    DrawModel(bodyModel, (Vector3){ 0, 0, 0 }, modelScale, ORANGE);
                    DrawModelWires(bodyModel, (Vector3){ 0, 0, 0 }, modelScale, BLACK);
                rlPopMatrix();
            }
        EndMode3D();

        // --- 4. ON-SCREEN HUD INTERFACE ---
        DrawText("HS-1 Exoskeleton - Alignment Configurator", 10, 10, 20, RAYWHITE);
        DrawText("Camera: Hold RMB + Move Mouse | Press [R] to Reset Camera", 10, 35, 14, LIGHTGRAY);
        
        // Render current transformation state
        DrawText(TextFormat("Pos: X=%.1f Y=%.1f Z=%.1f  (Arrow Keys / Up-Down)", bodyPos.x, bodyPos.y, bodyPos.z), 10, 65, 15, GREEN);
        DrawText(TextFormat("Rot: X=%.1f Y=%.1f Z=%.1f  (W/S: Pitch, A/D: Yaw, Q/E: Roll)", rotX, rotY, rotZ), 10, 85, 15, GREEN);
        DrawText(TextFormat("Scale: %.4f  (Use +/- Keys)", modelScale), 10, 105, 15, GREEN);

        EndDrawing();
    }

    // Cleanup resources
    if (isLoaded) UnloadModel(bodyModel);
    CloseWindow();
    return 0;
}