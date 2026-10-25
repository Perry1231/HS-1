#include "raylib.h"
#include "rlgl.h"
#include "stl_loader.h"
#include <iostream>
#include <fstream>
#include <cmath>

struct PartTransform {
    Vector3 defaultPos;
    float defaultRotX;
    float defaultRotY;
    float defaultRotZ;
    float defaultScale;

    Vector3 pos;
    float rotX;
    float rotY;
    float rotZ;
    float scale;
};

int main() {
    InitWindow(1280, 720, "HS-1 Exoskeleton - Full Body Configurator");
    SetTargetFPS(60);

    ChangeDirectory("C:/Users/Dell/Desktop/projetcs/GitHub Projects/HS-1/HS-1/HS-1_Programm");

    // --- 1. ЗАВАНТАЖЕННЯ ВСІХ МОДЕЛЕЙ (Права/Центр + Ліві) ---
    // Права сторона / Центр
    Model modelBicepsR   = LoadBinarySTL("Models/Biceps_2_0.stl");
    Model modelBody      = LoadBinarySTL("Models/Body_2_0.stl");
    Model modelForearmR  = LoadBinarySTL("Models/Forearm_2_0.stl");
    Model modelHandR     = LoadBinarySTL("Models/Hand_2_0.stl");
    Model modelFootR     = LoadBinarySTL("Models/Leg_Foot_2_0.stl");
    Model modelLegHighR  = LoadBinarySTL("Models/Leg_Higher_2_0.stl");
    Model modelLegLowR   = LoadBinarySTL("Models/Leg_Lower_2_0.stl");

    // Ліві частини (Змініть назви файлів нижче, якщо вони у вас називаються інакше)
    Model modelBicepsL   = LoadBinarySTL("Models/Biceps_Left_2_0.stl");
    Model modelForearmL  = LoadBinarySTL("Models/Forearm_Left_2_0.stl");
    Model modelHandL     = LoadBinarySTL("Models/Hand_Left_2_0.stl");
    Model modelFootL     = LoadBinarySTL("Models/Leg_Foot_Left_2_0.stl");
    Model modelLegHighL  = LoadBinarySTL("Models/Leg_Higher_Left_2_0.stl");
    Model modelLegLowL   = LoadBinarySTL("Models/Leg_Lower_Left_2_0.stl");

    const int totalParts = 13; // Всього 13 частин тіла

    bool isLoaded[13] = {
        modelBicepsR.meshCount > 0,
        modelBody.meshCount > 0,
        modelForearmR.meshCount > 0,
        modelHandR.meshCount > 0,
        modelFootR.meshCount > 0,
        modelLegHighR.meshCount > 0,
        modelLegLowR.meshCount > 0,
        modelBicepsL.meshCount > 0,
        modelForearmL.meshCount > 0,
        modelHandL.meshCount > 0,
        modelFootL.meshCount > 0,
        modelLegHighL.meshCount > 0,
        modelLegLowL.meshCount > 0
    };
    
    // --- 2. МАСИВ НАЛАШТУВАНЬ ДЛЯ ВСІХ 13 ЧАСТИН ---
    PartTransform parts[13] = {
        // 0: Right Biceps
        { {0.0f, 20.0f, 0.0f}, 0.0f, 0.0f, 0.0f, 0.05f,   {0.0f, 20.0f, 0.0f}, 0.0f, 0.0f, 0.0f, 0.05f },
        // 1: Body
        { {0.0f, 15.0f, 0.0f}, 0.0f, 0.0f, 0.0f, 0.05f,   {0.0f, 15.0f, 0.0f}, 0.0f, 0.0f, 0.0f, 0.05f },
        // 2: Right Forearm
        { {0.0f, 15.0f, 0.0f}, 0.0f, 0.0f, 0.0f, 0.05f,   {0.0f, 15.0f, 0.0f}, 0.0f, 0.0f, 0.0f, 0.05f },
        // 3: Right Hand
        { {0.0f, 10.0f, 0.0f}, 0.0f, 0.0f, 0.0f, 0.05f,   {0.0f, 10.0f, 0.0f}, 0.0f, 0.0f, 0.0f, 0.05f },
        // 4: Right Foot
        { {0.0f, 0.0f, 0.0f},  0.0f, 0.0f, 0.0f, 0.05f,   {0.0f, 0.0f, 0.0f},  0.0f, 0.0f, 0.0f, 0.05f },
        // 5: Right Leg Higher
        { {0.0f, 8.0f, 0.0f},  0.0f, 0.0f, 0.0f, 0.05f,   {0.0f, 8.0f, 0.0f},  0.0f, 0.0f, 0.0f, 0.05f },
        // 6: Right Leg Lower
        { {0.0f, 4.0f, 0.0f},  0.0f, 0.0f, 0.0f, 0.05f,   {0.0f, 4.0f, 0.0f},  0.0f, 0.0f, 0.0f, 0.05f },
        
        // 7: Left Biceps
        { {0.0f, 20.0f, 0.0f}, 0.0f, 0.0f, 0.0f, 0.05f,   {0.0f, 20.0f, 0.0f}, 0.0f, 0.0f, 0.0f, 0.05f },
        // 8: Left Forearm
        { {0.0f, 15.0f, 0.0f}, 0.0f, 0.0f, 0.0f, 0.05f,   {0.0f, 15.0f, 0.0f}, 0.0f, 0.0f, 0.0f, 0.05f },
        // 9: Left Hand
        { {0.0f, 10.0f, 0.0f}, 0.0f, 0.0f, 0.0f, 0.05f,   {0.0f, 10.0f, 0.0f}, 0.0f, 0.0f, 0.0f, 0.05f },
        // 10: Left Foot
        { {0.0f, 0.0f, 0.0f},  0.0f, 0.0f, 0.0f, 0.05f,   {0.0f, 0.0f, 0.0f},  0.0f, 0.0f, 0.0f, 0.05f },
        // 11: Left Leg Higher
        { {0.0f, 8.0f, 0.0f},  0.0f, 0.0f, 0.0f, 0.05f,   {0.0f, 8.0f, 0.0f},  0.0f, 0.0f, 0.0f, 0.05f },
        // 12: Left Leg Lower
        { {0.0f, 4.0f, 0.0f},  0.0f, 0.0f, 0.0f, 0.05f,   {0.0f, 4.0f, 0.0f},  0.0f, 0.0f, 0.0f, 0.05f }
    };

    const char* partNames[13] = { 
        "1. Right Biceps", 
        "2. Body", 
        "3. Right Forearm", 
        "4. Right Hand", 
        "5. Right Foot", 
        "6. Right Leg High", 
        "7. Right Leg Low",
        "8. Left Biceps", 
        "9. Left Forearm", 
        "10. Left Hand", 
        "11. Left Foot", 
        "12. Left Leg High", 
        "13. Left Leg Low"
    };

    int selectedPart = 0; 
    bool showDropdownList = false;

    std::ifstream inFile("exoskeleton_config.txt");
    if (inFile.is_open()) {
        for (int i = 0; i < totalParts; i++) {
            inFile >> parts[i].pos.x >> parts[i].pos.y >> parts[i].pos.z 
                   >> parts[i].rotX >> parts[i].rotY >> parts[i].rotZ >> parts[i].scale;
        }
        inFile.close();
    }

    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 30.0f, 150.0f };
    camera.target = (Vector3){ 0.0f, 20.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    bool showControlWindow = false;
    bool cursorUnlocked = false;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            cursorUnlocked = !cursorUnlocked;
            if (cursorUnlocked) EnableCursor();
            else DisableCursor();
        }

        Vector2 mousePos = GetMousePosition();

        if (!showControlWindow) {
            float camSpeed = 1.5f;
            Vector3 forward = { camera.target.x - camera.position.x, 0.0f, camera.target.z - camera.position.z };
            float forwardLen = sqrtf(forward.x * forward.x + forward.z * forward.z);
            if (forwardLen > 0.001f) { forward.x /= forwardLen; forward.z /= forwardLen; }
            Vector3 right = { -forward.z, 0.0f, forward.x };

            if (IsKeyDown(KEY_UP))    { camera.position.x += forward.x * camSpeed; camera.position.z += forward.z * camSpeed; camera.target.x += forward.x * camSpeed; camera.target.z += forward.z * camSpeed; }
            if (IsKeyDown(KEY_DOWN))  { camera.position.x -= forward.x * camSpeed; camera.position.z -= forward.z * camSpeed; camera.target.x -= forward.x * camSpeed; camera.target.z -= forward.z * camSpeed; }
            if (IsKeyDown(KEY_LEFT))  { camera.position.x -= right.x * camSpeed; camera.position.z -= right.z * camSpeed; camera.target.x -= right.x * camSpeed; camera.target.z -= right.z * camSpeed; }
            if (IsKeyDown(KEY_RIGHT)) { camera.position.x += right.x * camSpeed; camera.position.z += right.z * camSpeed; camera.target.x += right.x * camSpeed; camera.target.z += right.z * camSpeed; }

            if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) { camera.position.y -= camSpeed; camera.target.y -= camSpeed; }
            if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))     { camera.position.y += camSpeed; camera.target.y += camSpeed; }
        }

        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            UpdateCamera(&camera, CAMERA_FREE);
        }

        if (IsKeyPressed(KEY_R)) {
            camera.position = (Vector3){ 0.0f, 30.0f, 150.0f };
            camera.target = (Vector3){ 0.0f, 20.0f, 0.0f };
        }

        if (IsKeyDown(KEY_KP_ADD) || IsKeyDown(KEY_EQUAL))      parts[selectedPart].scale += 0.001f;
        if (IsKeyDown(KEY_KP_SUBTRACT) || IsKeyDown(KEY_MINUS)) parts[selectedPart].scale -= 0.001f;

        BeginDrawing();
        ClearBackground(DARKGRAY);

        BeginMode3D(camera);
            DrawGrid(50, 10.0f);

            Model* models[13] = { 
                &modelBicepsR, &modelBody, &modelForearmR, &modelHandR, &modelFootR, &modelLegHighR, &modelLegLowR,
                &modelBicepsL, &modelForearmL, &modelHandL, &modelFootL, &modelLegHighL, &modelLegLowL 
            };
            Color colors[13] = { 
                RED, ORANGE, BLUE, SKYBLUE, GREEN, YELLOW, PURPLE, 
                MAROON, DARKBLUE, DARKGREEN, BROWN, DARKPURPLE, GOLD 
            };

            for (int i = 0; i < 13; i++) {
                if (isLoaded[i]) {
                    rlPushMatrix();
                        rlTranslatef(parts[i].pos.x, parts[i].pos.y, parts[i].pos.z);
                        rlRotatef(parts[i].rotX, 1.0f, 0.0f, 0.0f);
                        rlRotatef(parts[i].rotY, 0.0f, 1.0f, 0.0f);
                        rlRotatef(parts[i].rotZ, 0.0f, 0.0f, 1.0f);
                        DrawModel(*models[i], (Vector3){0, 0, 0}, parts[i].scale, colors[i]);
                        DrawModelWires(*models[i], (Vector3){0, 0, 0}, parts[i].scale, BLACK);
                    rlPopMatrix();
                }
            }
        EndMode3D();

        DrawText("HS-1 Exoskeleton - Full Body Configurator", 10, 10, 20, RAYWHITE);
        DrawText("Camera: Arrows (Forward/Back/Strafe) | Ctrl/Shift (Down/Up) | RMB + Drag", 10, 35, 14, LIGHTGRAY);
        
        Rectangle btnOpenRect = { (float)GetScreenWidth() - 180, 10, 170, 30 };
        bool btnOpenHover = CheckCollisionPointRec(mousePos, btnOpenRect);
        DrawRectangleRec(btnOpenRect, btnOpenHover ? LIGHTGRAY : GRAY);
        DrawRectangleLines((int)btnOpenRect.x, (int)btnOpenRect.y, (int)btnOpenRect.width, (int)btnOpenRect.height, BLACK);
        DrawText("Part Controls", (int)btnOpenRect.x + 25, (int)btnOpenRect.y + 8, 14, BLACK);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && btnOpenHover) {
            showControlWindow = !showControlWindow;
            showDropdownList = false;
        }

        if (showControlWindow) {
            // Вікно керування (збільшене по висоті під список із 13 елементів)
            Rectangle winRect = { (float)GetScreenWidth() - 320, 50, 300, 420 };
            DrawRectangleRec(winRect, LIGHTGRAY);
            DrawRectangleLines((int)winRect.x, (int)winRect.y, (int)winRect.width, (int)winRect.height, BLACK);
            
            DrawRectangle((int)winRect.x, (int)winRect.y, (int)winRect.width, 30, DARKGRAY);
            DrawText("Select & Control Part", (int)winRect.x + 10, (int)winRect.y + 8, 14, RAYWHITE);

            Rectangle btnCloseRect = { winRect.x + winRect.width - 25, winRect.y + 5, 20, 20 };
            bool btnCloseHover = CheckCollisionPointRec(mousePos, btnCloseRect);
            DrawRectangleRec(btnCloseRect, btnCloseHover ? RED : DARKGRAY);
            DrawText("X", (int)btnCloseRect.x + 6, (int)btnCloseRect.y + 3, 14, RAYWHITE);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && btnCloseHover) {
                showControlWindow = false;
                showDropdownList = false;
            }

            Rectangle dropDownRect = { winRect.x + 15, winRect.y + 45, 270, 30 };
            bool dropHover = CheckCollisionPointRec(mousePos, dropDownRect);
            DrawRectangleRec(dropDownRect, dropHover ? GRAY : WHITE);
            DrawRectangleLines((int)dropDownRect.x, (int)dropDownRect.y, (int)dropDownRect.width, (int)dropDownRect.height, BLACK);
            DrawText(TextFormat("Part: %s [v]", partNames[selectedPart]), (int)dropDownRect.x + 10, (int)dropDownRect.y + 8, 12, BLACK);

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && dropHover) {
                showDropdownList = !showDropdownList;
            }

            auto DrawHoldButton = [&](Rectangle rect, const char* text) {
                bool hover = CheckCollisionPointRec(mousePos, rect);
                bool down = hover && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
                DrawRectangleRec(rect, down ? DARKGRAY : (hover ? GRAY : LIGHTGRAY));
                DrawRectangleLines((int)rect.x, (int)rect.y, (int)rect.width, (int)rect.height, BLACK);
                DrawText(text, (int)rect.x + 6, (int)rect.y + 8, 10, BLACK);
                return down;
            };

            auto DrawPressButton = [&](Rectangle rect, const char* text) {
                bool hover = CheckCollisionPointRec(mousePos, rect);
                DrawRectangleRec(rect, hover ? GRAY : DARKGRAY);
                DrawRectangleLines((int)rect.x, (int)rect.y, (int)rect.width, (int)rect.height, BLACK);
                DrawText(text, (int)rect.x + 6, (int)rect.y + 8, 10, RAYWHITE);
                return (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hover);
            };

            float moveStep = 0.2f;
            Rectangle btnUp = { winRect.x + 125, winRect.y + 90, 50, 30 };
            Rectangle btnDown = { winRect.x + 125, winRect.y + 160, 50, 30 };
            Rectangle btnLeft = { winRect.x + 20, winRect.y + 125, 65, 30 };
            Rectangle btnRight = { winRect.x + 215, winRect.y + 125, 65, 30 };
            Rectangle btnBack = { winRect.x + 90, winRect.y + 125, 30, 30 };
            Rectangle btnFwd  = { winRect.x + 180, winRect.y + 125, 30, 30 };

            if (DrawHoldButton(btnUp, "UP"))    parts[selectedPart].pos.y += moveStep;
            if (DrawHoldButton(btnDown, "DOWN"))  parts[selectedPart].pos.y -= moveStep;
            if (DrawHoldButton(btnLeft, "LEFT"))  parts[selectedPart].pos.x -= moveStep;
            if (DrawHoldButton(btnRight, "RIGHT")) parts[selectedPart].pos.x += moveStep;
            if (DrawHoldButton(btnBack, "BACK"))   parts[selectedPart].pos.z += moveStep;
            if (DrawHoldButton(btnFwd, "FWD"))    parts[selectedPart].pos.z -= moveStep;

            float rotStep = 1.0f;
            Rectangle btnRotLeft = { winRect.x + 65, winRect.y + 205, 75, 30 };
            Rectangle btnRotRight = { winRect.x + 160, winRect.y + 205, 75, 30 };
            if (DrawHoldButton(btnRotLeft, "ROT -"))  parts[selectedPart].rotY -= rotStep;
            if (DrawHoldButton(btnRotRight, "ROT +")) parts[selectedPart].rotY += rotStep;

            Rectangle btnTiltDown = { winRect.x + 65, winRect.y + 245, 75, 30 };
            Rectangle btnTiltUp   = { winRect.x + 160, winRect.y + 245, 75, 30 };
            if (DrawHoldButton(btnTiltDown, "TILT -")) parts[selectedPart].rotX -= rotStep;
            if (DrawHoldButton(btnTiltUp, "TILT +"))   parts[selectedPart].rotX += rotStep;

            Rectangle btnReset = { winRect.x + 20, winRect.y + 295, 120, 35 };
            if (DrawPressButton(btnReset, "Reset Default")) {
                parts[selectedPart].pos = parts[selectedPart].defaultPos;
                parts[selectedPart].rotX = parts[selectedPart].defaultRotX;
                parts[selectedPart].rotY = parts[selectedPart].defaultRotY;
                parts[selectedPart].rotZ = parts[selectedPart].defaultRotZ;
                parts[selectedPart].scale = parts[selectedPart].defaultScale;
            }

            Rectangle btnSave = { winRect.x + 160, winRect.y + 295, 120, 35 };
            if (DrawPressButton(btnSave, "Save Settings")) {
                std::ofstream outFile("exoskeleton_config.txt");
                if (outFile.is_open()) {
                    for (int i = 0; i < totalParts; i++) {
                        outFile << parts[i].pos.x << " " << parts[i].pos.y << " " << parts[i].pos.z << " " 
                                << parts[i].rotX << " " << parts[i].rotY << " " << parts[i].rotZ << " " << parts[i].scale << "\n";
                    }
                    outFile.close();
                }
            }

            DrawText("Config saved to exoskeleton_config.txt", (int)winRect.x + 35, (int)winRect.y + 345, 10, DARKGRAY);

            // Випадаючий список (з прокруткою або вибором потрібної деталі з усіх 13)
            if (showDropdownList) {
                for (int i = 0; i < totalParts; i++) {
                    Rectangle itemRect = { winRect.x + 15, winRect.y + 75 + (float)(i * 20), 270, 20 };
                    bool itemHover = CheckCollisionPointRec(mousePos, itemRect);
                    DrawRectangleRec(itemRect, itemHover ? LIGHTGRAY : WHITE);
                    DrawRectangleLines((int)itemRect.x, (int)itemRect.y, (int)itemRect.width, (int)itemRect.height, BLACK);
                    DrawText(partNames[i], (int)itemRect.x + 5, (int)itemRect.y + 4, 10, BLACK);

                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && itemHover) {
                        selectedPart = i;
                        showDropdownList = false;
                    }
                }
            }
        }

        DrawText(TextFormat("Selected: %s", partNames[selectedPart]), 10, 90, 15, YELLOW);
        DrawText(TextFormat("Pos: X=%.1f Y=%.1f Z=%.1f", parts[selectedPart].pos.x, parts[selectedPart].pos.y, parts[selectedPart].pos.z), 10, 110, 15, GREEN);
        DrawText(TextFormat("Rot: X=%.1f Y=%.1f Z=%.1f", parts[selectedPart].rotX, parts[selectedPart].rotY, parts[selectedPart].rotZ), 10, 130, 15, GREEN);
        DrawText(TextFormat("Scale: %.4f", parts[selectedPart].scale), 10, 150, 15, GREEN);

        EndDrawing();
    }

    // Вивантаження всіх моделей при закритті
    if (isLoaded[0]) UnloadModel(modelBicepsR);
    if (isLoaded[1]) UnloadModel(modelBody);
    if (isLoaded[2]) UnloadModel(modelForearmR);
    if (isLoaded[3]) UnloadModel(modelHandR);
    if (isLoaded[4]) UnloadModel(modelFootR);
    if (isLoaded[5]) UnloadModel(modelLegHighR);
    if (isLoaded[6]) UnloadModel(modelLegLowR);
    if (isLoaded[7]) UnloadModel(modelBicepsL);
    if (isLoaded[8]) UnloadModel(modelForearmL);
    if (isLoaded[9]) UnloadModel(modelHandL);
    if (isLoaded[10]) UnloadModel(modelFootL);
    if (isLoaded[11]) UnloadModel(modelLegHighL);
    if (isLoaded[12]) UnloadModel(modelLegLowL);

    CloseWindow();
    return 0;
}