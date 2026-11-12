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

struct ResolutionOption {
    const char* label;
    int width;
    int height;
};

// Застосування роздільної здатності без блокування Alt+Tab
void ApplyWindowResolution(int width, int height) {
    int monitor = GetCurrentMonitor();
    int monW = GetMonitorWidth(monitor);
    int monH = GetMonitorHeight(monitor);

    if (width >= 1920 || (width >= monW && height >= monH)) {
        // Безрамковий повноекранний режим: перекриває панель завдань, але дозволяє Alt+Tab
        ClearWindowState(FLAG_WINDOW_TOPMOST);
        SetWindowState(FLAG_WINDOW_UNDECORATED);
        SetWindowSize(width, height);
        SetWindowPosition(0, 0);
    } else {
        // Звичайне вікно з хрестиком та рамкою
        ClearWindowState(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TOPMOST);
        SetWindowSize(width, height);
        SetWindowPosition(
            (monW - width) / 2,
            (monH - height) / 2
        );
    }
}

int main() {
    ResolutionOption resolutions[] = {
        { "1280 x 720 (HD)",      1280, 720  },
        { "1366 x 768",           1366, 768  },
        { "1600 x 900 (HD+)",     1600, 900  },
        { "1920 x 1080 (FullHD)", 1920, 1080 }
    };
    const int totalResolutions = 4;
    int selectedResolution = 0;
    bool showResolutionDropdown = false;

    int keyForward = KEY_UP;
    int keyBackward = KEY_DOWN;
    int keyLeft = KEY_LEFT;
    int keyRight = KEY_RIGHT;
    int keyUpCam = KEY_LEFT_SHIFT;
    int keyDownCam = KEY_LEFT_CONTROL;

    ChangeDirectory("C:/Users/Dell/Desktop/projetcs/GitHub Projects/HS-1/HS-1/HS-1_Programm");

    std::ifstream progIn("program_settings.txt");
    if (progIn.is_open()) {
        int savedRes = 0;
        if (progIn >> savedRes >> keyForward >> keyBackward >> keyLeft >> keyRight >> keyUpCam >> keyDownCam) {
            if (savedRes >= 0 && savedRes < totalResolutions) {
                selectedResolution = savedRes;
            }
        }
        progIn.close();
    }

    InitWindow(resolutions[selectedResolution].width, resolutions[selectedResolution].height, "HS-1 Exoskeleton - Full Body Configurator");
    ApplyWindowResolution(resolutions[selectedResolution].width, resolutions[selectedResolution].height);
    SetTargetFPS(60);

    // Вимикаємо закриття на ESC, щоб ESC тільки розблоковував курсор
    SetExitKey(KEY_NULL);

    // --- 1. LOAD ALL MODELS ---
    Model modelBicepsR   = LoadBinarySTL("Models/Biceps_2_0.stl");
    Model modelBody      = LoadBinarySTL("Models/Body_2_0.stl");
    Model modelForearmR  = LoadBinarySTL("Models/Forearm_2_0.stl");
    Model modelHandR     = LoadBinarySTL("Models/Hand_2_0.stl");
    Model modelFootR     = LoadBinarySTL("Models/Leg_Foot_2_0.stl");
    Model modelLegHighR  = LoadBinarySTL("Models/Leg_Higher_2_0.stl");
    Model modelLegLowR   = LoadBinarySTL("Models/Leg_Lower_2_0.stl");

    Model modelBicepsL   = LoadBinarySTL("Models/Biceps_Left_2_0.stl");
    Model modelForearmL  = LoadBinarySTL("Models/Forearm_Left_2_0.stl");
    Model modelHandL     = LoadBinarySTL("Models/Hand_Left_2_0.stl");
    Model modelFootL     = LoadBinarySTL("Models/Leg_Foot_Left_2_0.stl");
    Model modelLegHighL  = LoadBinarySTL("Models/Leg_Higher_Left_2_0.stl");
    Model modelLegLowL   = LoadBinarySTL("Models/Leg_Lower_Left_2_0.stl");

    const int totalParts = 13;

    bool isLoaded[13] = {
        modelBicepsR.meshCount > 0, modelBody.meshCount > 0, modelForearmR.meshCount > 0,
        modelHandR.meshCount > 0, modelFootR.meshCount > 0, modelLegHighR.meshCount > 0,
        modelLegLowR.meshCount > 0, modelBicepsL.meshCount > 0, modelForearmL.meshCount > 0,
        modelHandL.meshCount > 0, modelFootL.meshCount > 0, modelLegHighL.meshCount > 0,
        modelLegLowL.meshCount > 0
    };
    
    PartTransform parts[13] = {
        { {0.0f, 20.0f, 0.0f}, 0.0f, 0.0f, 0.0f, 0.05f,   {0.0f, 20.0f, 0.0f}, 0.0f, 0.0f, 0.0f, 0.05f },
        { {0.0f, 15.0f, 0.0f}, 0.0f, 0.0f, 0.0f, 0.05f,   {0.0f, 15.0f, 0.0f}, 0.0f, 0.0f, 0.0f, 0.05f },
        { {0.0f, 15.0f, 0.0f}, 0.0f, 0.0f, 0.0f, 0.05f,   {0.0f, 15.0f, 0.0f}, 0.0f, 0.0f, 0.0f, 0.05f },
        { {0.0f, 10.0f, 0.0f}, 0.0f, 0.0f, 0.0f, 0.05f,   {0.0f, 10.0f, 0.0f}, 0.0f, 0.0f, 0.0f, 0.05f },
        { {0.0f, 0.0f, 0.0f},  0.0f, 0.0f, 0.0f, 0.05f,   {0.0f, 0.0f, 0.0f},  0.0f, 0.0f, 0.0f, 0.05f },
        { {0.0f, 8.0f, 0.0f},  0.0f, 0.0f, 0.0f, 0.05f,   {0.0f, 8.0f, 0.0f},  0.0f, 0.0f, 0.0f, 0.05f },
        { {0.0f, 4.0f, 0.0f},  0.0f, 0.0f, 0.0f, 0.05f,   {0.0f, 4.0f, 0.0f},  0.0f, 0.0f, 0.0f, 0.05f },
        { {24.8f, 67.4f, 0.0f}, 0.0f, 90.0f, 0.0f, 0.05f, {0.0f, 20.0f, 0.0f}, 0.0f, 0.0f, 0.0f, 0.05f },
        { {24.8f, 55.5f, 0.0f}, 0.0f, 0.0f, 0.0f, 0.05f,   {0.0f, 15.0f, 0.0f}, 0.0f, 0.0f, 0.0f, 0.05f },
        { {23.0f, 48.2f, 0.0f}, 0.0f, 90.0f, 0.0f, 0.05f, {0.0f, 10.0f, 0.0f}, 0.0f, 0.0f, 0.0f, 0.05f },
        { {0.0f, 0.0f, 0.0f},  0.0f, 0.0f, 0.0f, 0.05f,   {0.0f, 0.0f, 0.0f},  0.0f, 0.0f, 0.0f, 0.05f },
        { {0.0f, 8.0f, 0.0f},  0.0f, 0.0f, 0.0f, 0.05f,   {0.0f, 8.0f, 0.0f},  0.0f, 0.0f, 0.0f, 0.05f },
        { {0.0f, 4.0f, 0.0f},  0.0f, 0.0f, 0.0f, 0.05f,   {0.0f, 4.0f, 0.0f},  0.0f, 0.0f, 0.0f, 0.05f }
    };

    const char* partNames[13] = { 
        "1. Right Biceps", "2. Body", "3. Right Forearm", "4. Right Hand", 
        "5. Right Foot", "6. Right Leg High", "7. Right Leg Low",
        "8. Left Biceps", "9. Left Forearm", "10. Left Hand", 
        "11. Left Foot", "12. Left Leg High", "13. Left Leg Low"
    };

    bool is2DMode = false;
    bool isProgramSettingsMode = false;
    bool shouldExitApp = false;
    float settingsSavedTimer = 0.0f;
    
    bool bodyConnected[13] = { false }; 

    int selectedPart = 0; 
    bool showDropdownList = false;
    int waitingForKeyIndex = -1;

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
    bool showKeybindsWindow = false;
    bool cursorUnlocked = false;

    while (!WindowShouldClose() && !shouldExitApp) {
        if (settingsSavedTimer > 0.0f) {
            settingsSavedTimer -= GetFrameTime();
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            cursorUnlocked = !cursorUnlocked;
            if (cursorUnlocked) EnableCursor();
            else DisableCursor();
        }

        Vector2 mousePos = GetMousePosition();

        // Keybinding capture logic
        if (waitingForKeyIndex != -1) {
            int keyPressed = GetKeyPressed();
            if (keyPressed != 0) {
                if (waitingForKeyIndex == 0) keyForward = keyPressed;
                else if (waitingForKeyIndex == 1) keyBackward = keyPressed;
                else if (waitingForKeyIndex == 2) keyLeft = keyPressed;
                else if (waitingForKeyIndex == 3) keyRight = keyPressed;
                else if (waitingForKeyIndex == 4) keyUpCam = keyPressed;
                else if (waitingForKeyIndex == 5) keyDownCam = keyPressed;
                
                waitingForKeyIndex = -1;
            }
        }

        if (!is2DMode && !isProgramSettingsMode && !showControlWindow && !showKeybindsWindow && waitingForKeyIndex == -1) {
            float camSpeed = 1.5f;
            Vector3 forward = { camera.target.x - camera.position.x, 0.0f, camera.target.z - camera.position.z };
            float forwardLen = sqrtf(forward.x * forward.x + forward.z * forward.z);
            if (forwardLen > 0.001f) { forward.x /= forwardLen; forward.z /= forwardLen; }
            Vector3 right = { -forward.z, 0.0f, forward.x };

            if (IsKeyDown(keyForward))  { camera.position.x += forward.x * camSpeed; camera.position.z += forward.z * camSpeed; camera.target.x += forward.x * camSpeed; camera.target.z += forward.z * camSpeed; }
            if (IsKeyDown(keyBackward)) { camera.position.x -= forward.x * camSpeed; camera.position.z -= forward.z * camSpeed; camera.target.x -= forward.x * camSpeed; camera.target.z -= forward.z * camSpeed; }
            if (IsKeyDown(keyLeft))     { camera.position.x -= right.x * camSpeed; camera.position.z -= right.z * camSpeed; camera.target.x -= right.x * camSpeed; camera.target.z -= right.z * camSpeed; }
            if (IsKeyDown(keyRight))    { camera.position.x += right.x * camSpeed; camera.position.z += right.z * camSpeed; camera.target.x += right.x * camSpeed; camera.target.z += right.z * camSpeed; }

            if (IsKeyDown(keyDownCam)) { camera.position.y -= camSpeed; camera.target.y -= camSpeed; }
            if (IsKeyDown(keyUpCam))   { camera.position.y += camSpeed; camera.target.y += camSpeed; }
        }

        if (!is2DMode && !isProgramSettingsMode && IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            UpdateCamera(&camera, CAMERA_FREE);
        }

        if (!isProgramSettingsMode && IsKeyPressed(KEY_R)) {
            camera.position = (Vector3){ 0.0f, 30.0f, 150.0f };
            camera.target = (Vector3){ 0.0f, 20.0f, 0.0f };
        }

        if (!is2DMode && !isProgramSettingsMode) {
            if (IsKeyDown(KEY_KP_ADD) || IsKeyDown(KEY_EQUAL))      parts[selectedPart].scale += 0.001f;
            if (IsKeyDown(KEY_KP_SUBTRACT) || IsKeyDown(KEY_MINUS)) parts[selectedPart].scale -= 0.001f;
        }

        BeginDrawing();
        ClearBackground(DARKGRAY);

        if (isProgramSettingsMode) {
            // ================= PROGRAMM SETTINGS PAGE =================
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), DARKGRAY);
            DrawText("PROGRAMM SETTINGS", 40, 30, 26, RAYWHITE);
            DrawLine(40, 65, GetScreenWidth() - 40, 65, GRAY);

            // --- BACK BUTTON (TOP RIGHT) ---
            Rectangle btnBackTopRight = { (float)GetScreenWidth() - 140, 20, 120, 35 };
            bool btnBackHover = CheckCollisionPointRec(mousePos, btnBackTopRight);
            DrawRectangleRec(btnBackTopRight, btnBackHover ? ORANGE : GRAY);
            DrawRectangleLines((int)btnBackTopRight.x, (int)btnBackTopRight.y, (int)btnBackTopRight.width, (int)btnBackTopRight.height, RAYWHITE);
            
            int backTxtW = MeasureText("Back", 16);
            DrawText("Back", (int)(btnBackTopRight.x + (btnBackTopRight.width - backTxtW) / 2), (int)btnBackTopRight.y + 9, 16, BLACK);

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && btnBackHover) {
                isProgramSettingsMode = false;
                showResolutionDropdown = false;
                settingsSavedTimer = 0.0f;
            }

            // --- 1. WINDOW RESOLUTION SETTING ---
            DrawText("Window Resolution:", 40, 95, 18, RAYWHITE);

            Rectangle resBtnRect = { 240, 88, 220, 35 };
            bool resBtnHover = CheckCollisionPointRec(mousePos, resBtnRect);
            DrawRectangleRec(resBtnRect, resBtnHover ? LIGHTGRAY : WHITE);
            DrawRectangleLines((int)resBtnRect.x, (int)resBtnRect.y, (int)resBtnRect.width, (int)resBtnRect.height, BLACK);
            
            DrawText(TextFormat("%s  [v]", resolutions[selectedResolution].label), (int)resBtnRect.x + 12, (int)resBtnRect.y + 10, 14, BLACK);

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && resBtnHover) {
                showResolutionDropdown = !showResolutionDropdown;
            }

            // --- 2. SAVE PROGRAM SETTINGS BUTTON ---
            Rectangle btnSaveProgRect = { 40, (float)GetScreenHeight() - 75, 220, 42 };
            bool btnSaveProgHover = CheckCollisionPointRec(mousePos, btnSaveProgRect);
            DrawRectangleRec(btnSaveProgRect, btnSaveProgHover ? LIME : GREEN);
            DrawRectangleLines((int)btnSaveProgRect.x, (int)btnSaveProgRect.y, (int)btnSaveProgRect.width, (int)btnSaveProgRect.height, BLACK);
            
            int saveTxtW = MeasureText("Save Program Settings", 15);
            DrawText("Save Program Settings", (int)(btnSaveProgRect.x + (btnSaveProgRect.width - saveTxtW) / 2), (int)btnSaveProgRect.y + 13, 15, BLACK);

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && btnSaveProgHover) {
                std::ofstream progOut("program_settings.txt");
                if (progOut.is_open()) {
                    progOut << selectedResolution << " "
                            << keyForward << " " << keyBackward << " "
                            << keyLeft << " " << keyRight << " "
                            << keyUpCam << " " << keyDownCam << "\n";
                    progOut.close();
                    settingsSavedTimer = 3.0f;
                }
            }

            if (settingsSavedTimer > 0.0f) {
                DrawText("Saved to program_settings.txt!", (int)btnSaveProgRect.x + 240, (int)btnSaveProgRect.y + 13, 16, GREEN);
            }

            if (showResolutionDropdown) {
                for (int i = 0; i < totalResolutions; i++) {
                    Rectangle itemRect = { resBtnRect.x, resBtnRect.y + resBtnRect.height + (float)(i * 32), resBtnRect.width, 32 };
                    bool itemHover = CheckCollisionPointRec(mousePos, itemRect);
                    
                    Color itemColor = (i == selectedResolution) ? SKYBLUE : (itemHover ? LIGHTGRAY : RAYWHITE);
                    DrawRectangleRec(itemRect, itemColor);
                    DrawRectangleLines((int)itemRect.x, (int)itemRect.y, (int)itemRect.width, (int)itemRect.height, BLACK);
                    DrawText(resolutions[i].label, (int)itemRect.x + 12, (int)itemRect.y + 9, 14, BLACK);

                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && itemHover) {
                        selectedResolution = i;
                        showResolutionDropdown = false;
                        ApplyWindowResolution(resolutions[i].width, resolutions[i].height);
                    }
                }
            }

        } else if (!is2DMode) {
            // ================= 3D MODE =================
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
            DrawText("Camera: Custom Keys | RMB + Drag | Alt+Tab to switch", 10, 35, 14, LIGHTGRAY);
            
            // --- 2D MAP BUTTON (BOTTOM CENTER) ---
            Rectangle btnTo2D = { (float)GetScreenWidth() / 2.0f - 110, (float)GetScreenHeight() - 60, 220, 40 };
            bool btn2DHover = CheckCollisionPointRec(mousePos, btnTo2D);
            DrawRectangleRec(btnTo2D, btn2DHover ? SKYBLUE : BLUE);
            DrawRectangleLines((int)btnTo2D.x, (int)btnTo2D.y, (int)btnTo2D.width, (int)btnTo2D.height, BLACK);
            DrawText("Open 2D Body Map", (int)btnTo2D.x + 28, (int)btnTo2D.y + 13, 14, RAYWHITE);

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && btn2DHover) {
                is2DMode = true;
                showControlWindow = false;
                showKeybindsWindow = false;
            }

            // --- EXIT APP BUTTON (BOTTOM RIGHT) ---
            Rectangle btnExitApp = { (float)GetScreenWidth() - 130, (float)GetScreenHeight() - 55, 110, 35 };
            bool btnExitHover = CheckCollisionPointRec(mousePos, btnExitApp);
            DrawRectangleRec(btnExitApp, btnExitHover ? RED : MAROON);
            DrawRectangleLines((int)btnExitApp.x, (int)btnExitApp.y, (int)btnExitApp.width, (int)btnExitApp.height, RAYWHITE);
            int exitTxtW = MeasureText("Exit [X]", 14);
            DrawText("Exit [X]", (int)(btnExitApp.x + (btnExitApp.width - exitTxtW) / 2), (int)btnExitApp.y + 10, 14, RAYWHITE);

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && btnExitHover) {
                shouldExitApp = true;
            }

            // --- 1. PART CONTROLS BUTTON ---
            Rectangle btnOpenRect = { (float)GetScreenWidth() - 180, 10, 170, 30 };
            bool btnOpenHover = CheckCollisionPointRec(mousePos, btnOpenRect);
            DrawRectangleRec(btnOpenRect, btnOpenHover ? LIGHTGRAY : GRAY);
            DrawRectangleLines((int)btnOpenRect.x, (int)btnOpenRect.y, (int)btnOpenRect.width, (int)btnOpenRect.height, BLACK);
            DrawText("Part Controls", (int)btnOpenRect.x + 25, (int)btnOpenRect.y + 8, 14, BLACK);

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && btnOpenHover) {
                showControlWindow = !showControlWindow;
                showKeybindsWindow = false;
                showDropdownList = false;
            }

            // --- 2. KEYBINDS SETUP BUTTON ---
            Rectangle btnKeybindsRect = { (float)GetScreenWidth() - 180, 50, 170, 30 };
            bool btnKeybindsHover = CheckCollisionPointRec(mousePos, btnKeybindsRect);
            DrawRectangleRec(btnKeybindsRect, btnKeybindsHover ? LIGHTGRAY : GRAY);
            DrawRectangleLines((int)btnKeybindsRect.x, (int)btnKeybindsRect.y, (int)btnKeybindsRect.width, (int)btnKeybindsRect.height, BLACK);
            DrawText("Keybinds Setup", (int)btnKeybindsRect.x + 28, (int)btnKeybindsRect.y + 8, 14, BLACK);

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && btnKeybindsHover) {
                showKeybindsWindow = !showKeybindsWindow;
                showControlWindow = false;
            }

            // --- 3. PROGRAMM SETTINGS BUTTON ---
            Rectangle btnProgSettingsRect = { (float)GetScreenWidth() - 180, 90, 170, 30 };
            bool btnProgSettingsHover = CheckCollisionPointRec(mousePos, btnProgSettingsRect);
            DrawRectangleRec(btnProgSettingsRect, btnProgSettingsHover ? LIGHTGRAY : GRAY);
            DrawRectangleLines((int)btnProgSettingsRect.x, (int)btnProgSettingsRect.y, (int)btnProgSettingsRect.width, (int)btnProgSettingsRect.height, BLACK);
            DrawText("Programm Settings", (int)btnProgSettingsRect.x + 18, (int)btnProgSettingsRect.y + 8, 14, BLACK);

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && btnProgSettingsHover) {
                isProgramSettingsMode = true;
                showControlWindow = false;
                showKeybindsWindow = false;
                showDropdownList = false;
            }

            // Part Controls Window
            if (showControlWindow) {
                Rectangle winRect = { (float)GetScreenWidth() - 320, 130, 300, 420 };
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

            // Keybinds Setup Window
            if (showKeybindsWindow) {
                Rectangle winKeyRect = { (float)GetScreenWidth() - 320, 130, 300, 310 };
                DrawRectangleRec(winKeyRect, LIGHTGRAY);
                DrawRectangleLines((int)winKeyRect.x, (int)winKeyRect.y, (int)winKeyRect.width, (int)winKeyRect.height, BLACK);
                
                DrawRectangle((int)winKeyRect.x, (int)winKeyRect.y, (int)winKeyRect.width, 30, DARKGRAY);
                DrawText("Keybinds Configuration", (int)winKeyRect.x + 10, (int)winKeyRect.y + 8, 14, RAYWHITE);

                Rectangle btnCloseKey = { winKeyRect.x + winKeyRect.width - 25, winKeyRect.y + 5, 20, 20 };
                bool btnCloseKeyHover = CheckCollisionPointRec(mousePos, btnCloseKey);
                DrawRectangleRec(btnCloseKey, btnCloseKeyHover ? RED : DARKGRAY);
                DrawText("X", (int)btnCloseKey.x + 6, (int)btnCloseKey.y + 3, 14, RAYWHITE);
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && btnCloseKeyHover) {
                    showKeybindsWindow = false;
                    waitingForKeyIndex = -1;
                }

                const char* bindLabels[6] = { "Forward", "Backward", "Move Left", "Move Right", "Move Up", "Move Down" };
                int* bindKeys[6] = { &keyForward, &keyBackward, &keyLeft, &keyRight, &keyUpCam, &keyDownCam };

                for (int i = 0; i < 6; i++) {
                    float yPos = winKeyRect.y + 45 + (float)(i * 38);
                    DrawText(bindLabels[i], (int)winKeyRect.x + 15, (int)yPos + 8, 12, BLACK);

                    Rectangle btnBindRect = { winKeyRect.x + 160, yPos, 125, 30 };
                    bool btnBindHover = CheckCollisionPointRec(mousePos, btnBindRect);
                    
                    bool isWaiting = (waitingForKeyIndex == i);
                    Color btnColor = isWaiting ? YELLOW : (btnBindHover ? GRAY : WHITE);
                    
                    DrawRectangleRec(btnBindRect, btnColor);
                    DrawRectangleLines((int)btnBindRect.x, (int)btnBindRect.y, (int)btnBindRect.width, (int)btnBindRect.height, BLACK);
                    
                    const char* keyName = isWaiting ? "Press Key..." : TextFormat("Key: %i", *bindKeys[i]);
                    DrawText(keyName, (int)btnBindRect.x + 10, (int)btnBindRect.y + 9, 10, BLACK);

                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && btnBindHover) {
                        waitingForKeyIndex = i;
                    }
                }
            }

            DrawText(TextFormat("Selected: %s", partNames[selectedPart]), 10, 90, 15, YELLOW);
            DrawText(TextFormat("Pos: X=%.1f Y=%.1f Z=%.1f", parts[selectedPart].pos.x, parts[selectedPart].pos.y, parts[selectedPart].pos.z), 10, 110, 15, GREEN);
            DrawText(TextFormat("Rot: X=%.1f Y=%.1f Z=%.1f", parts[selectedPart].rotX, parts[selectedPart].rotY, parts[selectedPart].rotZ), 10, 130, 15, GREEN);
            DrawText(TextFormat("Scale: %.4f", parts[selectedPart].scale), 10, 150, 15, GREEN);

        } else {
            // ================= 2D FULLSCREEN BODY MAP MODE =================
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), BLACK);

            DrawText("HS-1 EXOSKELETON - 2D BODY CONNECTION STATUS", 50, 30, 24, RAYWHITE);
            DrawText("Green = Connected to Real Block | White = Disconnected", 50, 60, 14, LIGHTGRAY);

            struct BodyBox2D {
                const char* label;
                Rectangle rect;
                int partIndex;
            };

            float cx = GetScreenWidth() / 2.0f;
            
            BodyBox2D layout[13] = {
                { "Right Biceps",    { cx - 220, 180, 130, 45 }, 0  },
                { "Body / Torso",    { cx - 75,  180, 150, 50 }, 1  },
                { "Right Forearm",   { cx - 220, 240, 130, 45 }, 2  },
                { "Right Hand",      { cx - 220, 300, 130, 40 }, 3  },
                { "Right Foot",      { cx - 110, 500, 100, 40 }, 4  },
                { "Right Leg High",  { cx - 110, 380, 100, 50 }, 5  },
                { "Right Leg Low",   { cx - 110, 440, 100, 50 }, 6  },
                
                { "Left Biceps",     { cx + 90,  180, 130, 45 }, 7  },
                { "Left Forearm",    { cx + 90,  240, 130, 45 }, 8  },
                { "Left Hand",       { cx + 90,  300, 130, 40 }, 9  },
                { "Left Foot",       { cx + 10,  500, 100, 40 }, 10 },
                { "Left Leg High",   { cx + 10,  380, 100, 50 }, 11 },
                { "Left Leg Low",    { cx + 10,  440, 100, 50 }, 12 }
            };

            for (int i = 0; i < 13; i++) {
                bool isConnected = bodyConnected[layout[i].partIndex];
                Color boxColor = isConnected ? GREEN : WHITE;
                Color textColor = BLACK;

                DrawRectangleRec(layout[i].rect, boxColor);
                DrawRectangleLines((int)layout[i].rect.x, (int)layout[i].rect.y, (int)layout[i].rect.width, (int)layout[i].rect.height, BLACK);
                
                int textWidth = MeasureText(layout[i].label, 10);
                DrawText(layout[i].label, (int)(layout[i].rect.x + (layout[i].rect.width - textWidth) / 2), (int)(layout[i].rect.y + 18), 10, textColor);
            }

            // --- BACK TO 3D BUTTON (MATCHED HEIGHT & SIZE WITH 2D BUTTON) ---
            Rectangle btnBackTo3D = { cx - 110, (float)GetScreenHeight() - 60, 220, 40 };
            bool btnBackHover = CheckCollisionPointRec(mousePos, btnBackTo3D);
            DrawRectangleRec(btnBackTo3D, btnBackHover ? ORANGE : DARKGRAY);
            DrawRectangleLines((int)btnBackTo3D.x, (int)btnBackTo3D.y, (int)btnBackTo3D.width, (int)btnBackTo3D.height, RAYWHITE);
            
            int backTextWidth = MeasureText("Back to 3D", 14);
            DrawText("Back to 3D", (int)(btnBackTo3D.x + (btnBackTo3D.width - backTextWidth) / 2), (int)btnBackTo3D.y + 13, 14, RAYWHITE);

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && btnBackHover) {
                is2DMode = false;
            }
        }

        EndDrawing();
    }

    // Unload models
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