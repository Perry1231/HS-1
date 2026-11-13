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

struct FpsOption {
    const char* label;
    int value;
};

struct FloatOption {
    const char* label;
    float value;
};

struct ThemeOption {
    const char* label;
    Color bgColor;
    Color gridColor;
};

// Apply window resolution without blocking Alt+Tab switching
void ApplyWindowResolution(int width, int height) {
    int monitor = GetCurrentMonitor();
    int monW = GetMonitorWidth(monitor);
    int monH = GetMonitorHeight(monitor);

    if (width >= 1920 || (width >= monW && height >= monH)) {
        ClearWindowState(FLAG_WINDOW_TOPMOST);
        SetWindowState(FLAG_WINDOW_UNDECORATED);
        SetWindowSize(width, height);
        SetWindowPosition(0, 0);
    } else {
        ClearWindowState(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TOPMOST);
        SetWindowSize(width, height);
        SetWindowPosition(
            (monW - width) / 2,
            (monH - height) / 2
        );
    }
}

int main() {
    // --- PART 1: SYSTEM & WINDOW SETTINGS ---
    ResolutionOption resolutions[] = {
        { "1280 x 720 (HD)",      1280, 720  },
        { "1366 x 768",           1366, 768  },
        { "1600 x 900 (HD+)",     1600, 900  },
        { "1920 x 1080 (FullHD)", 1920, 1080 }
    };
    const int totalResolutions = 4;
    int selectedResolution = 0;

    FpsOption fpsOptions[] = {
        { "30 FPS (Power Save)", 30  },
        { "60 FPS (Standard)",   60  },
        { "120 FPS (Smooth)",    120 },
        { "Unlimited (Max)",     0   }
    };
    const int totalFpsOptions = 4;
    int selectedFps = 1;

    const char* comPorts[] = {
        "COM1", "COM2", "COM3", "COM4", "COM5",
        "COM6", "COM7", "COM8", "COM9", "COM10"
    };
    const int totalComPorts = 10;
    int selectedComPort = 2; // Default: COM3

    // --- PART 2: COMMUNICATION & GRAPHICS ---
    const int baudRates[] = { 115200, 230400, 460800, 921600 };
    const int totalBaudRates = 4;
    int selectedBaudRate = 0; // Default: 115200

    bool showGroundGrid = true;
    bool showModelWireframe = true;

    // --- PART 3: CAMERA & TELEMETRY FILTERING ---
    FloatOption camSpeedOptions[] = {
        { "0.5x (Precision)", 0.75f },
        { "1.0x (Standard)",  1.50f },
        { "2.0x (Fast)",      3.00f },
        { "3.5x (Turbo)",     5.25f }
    };
    const int totalCamSpeeds = 4;
    int selectedCamSpeed = 1;

    FloatOption fovOptions[] = {
        { "45 deg (Narrow)",   45.0f },
        { "60 deg (Standard)", 60.0f },
        { "75 deg (Wide)",     75.0f }
    };
    const int totalFovOptions = 3;
    int selectedFov = 0;

    FloatOption smoothingOptions[] = {
        { "0% (Raw Data)",     0.00f },
        { "25% (Low Filter)",  0.25f },
        { "50% (Balanced)",    0.50f },
        { "75% (High Smooth)", 0.75f }
    };
    const int totalSmoothingOptions = 4;
    int selectedSmoothing = 1;

    // --- PART 4: SCENE THEME, SIMULATION & AUTO-RECONNECT ---
    ThemeOption themes[] = {
        { "Dark Gray (Default)", DARKGRAY,                   LIGHTGRAY },
        { "Pure Black (OLED)",   BLACK,                      DARKGRAY  },
        { "Blueprint CAD",       (Color){ 18, 42, 76, 255 }, SKYBLUE   },
        { "Light Studio",        (Color){ 215, 220, 225, 255 }, GRAY   }
    };
    const int totalThemes = 4;
    int selectedTheme = 0;

    bool demoWaveMode = false;
    bool autoReconnectUart = true;

    // Active dropdown state:
    // -1 = closed, 0 = Res, 1 = FPS, 2 = COM, 3 = Baud, 4 = CamSpeed, 5 = FOV, 6 = Smoothing, 7 = Theme
    int activeSettingsDropdown = -1;

    // --- CUSTOMIZABLE KEYBINDS ---
    int keyForward = KEY_UP;
    int keyBackward = KEY_DOWN;
    int keyLeft = KEY_LEFT;
    int keyRight = KEY_RIGHT;
    int keyUpCam = KEY_LEFT_SHIFT;
    int keyDownCam = KEY_LEFT_CONTROL;

    ChangeDirectory("C:/Users/Dell/Desktop/projetcs/GitHub Projects/HS-1/HS-1/HS-1_Programm");

    // --- LOAD PROGRAM SETTINGS FROM FILE ---
    std::ifstream progIn("program_settings.txt");
    if (progIn.is_open()) {
        int savedRes = 0, savedFps = 1, savedCom = 2, savedBaud = 0;
        int savedGrid = 1, savedWire = 1;
        int savedSpeed = 1, savedFov = 0, savedSmooth = 1;
        int savedTheme = 0, savedDemo = 0, savedAutoRec = 1;
        if (progIn >> savedRes >> savedFps >> savedCom >> savedBaud >> savedGrid >> savedWire
                   >> savedSpeed >> savedFov >> savedSmooth >> savedTheme >> savedDemo >> savedAutoRec
                   >> keyForward >> keyBackward >> keyLeft >> keyRight >> keyUpCam >> keyDownCam) {
            if (savedRes >= 0 && savedRes < totalResolutions)         selectedResolution = savedRes;
            if (savedFps >= 0 && savedFps < totalFpsOptions)           selectedFps = savedFps;
            if (savedCom >= 0 && savedCom < totalComPorts)             selectedComPort = savedCom;
            if (savedBaud >= 0 && savedBaud < totalBaudRates)          selectedBaudRate = savedBaud;
            showGroundGrid = (savedGrid != 0);
            showModelWireframe = (savedWire != 0);
            if (savedSpeed >= 0 && savedSpeed < totalCamSpeeds)        selectedCamSpeed = savedSpeed;
            if (savedFov >= 0 && savedFov < totalFovOptions)           selectedFov = savedFov;
            if (savedSmooth >= 0 && savedSmooth < totalSmoothingOptions) selectedSmoothing = savedSmooth;
            if (savedTheme >= 0 && savedTheme < totalThemes)           selectedTheme = savedTheme;
            demoWaveMode = (savedDemo != 0);
            autoReconnectUart = (savedAutoRec != 0);
        }
        progIn.close();
    }

    InitWindow(resolutions[selectedResolution].width, resolutions[selectedResolution].height, "HS-1 Exoskeleton - Full Body Configurator");
    ApplyWindowResolution(resolutions[selectedResolution].width, resolutions[selectedResolution].height);
    SetTargetFPS(fpsOptions[selectedFps].value);
    SetExitKey(KEY_NULL);

    // --- LOAD ALL 3D STL MODELS ---
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
    float statusMsgTimer = 0.0f;
    const char* statusMsgText = "";
    
    bool bodyConnected[13] = { false }; 

    int selectedPart = 0; 
    bool showDropdownList = false;
    int waitingForKeyIndex = -1;

    // Load saved 3D part transforms
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
    camera.fovy = fovOptions[selectedFov].value;
    camera.projection = CAMERA_PERSPECTIVE;

    bool showControlWindow = false;
    bool showKeybindsWindow = false;
    bool cursorUnlocked = false;

    while (!WindowShouldClose() && !shouldExitApp) {
        if (statusMsgTimer > 0.0f) statusMsgTimer -= GetFrameTime();

        // Demo wave simulation (when Demo Simulation mode is enabled)
        if (demoWaveMode) {
            float t = (float)GetTime() * 2.2f;
            float wave = sinf(t) * 18.0f;
            parts[0].rotX = parts[0].defaultRotX + wave;
            parts[2].rotX = parts[2].defaultRotX + wave * 1.2f;
            parts[7].rotX = parts[7].defaultRotX - wave;
            parts[8].rotX = parts[8].defaultRotX - wave * 1.2f;
            for (int i = 0; i < 13; i++) bodyConnected[i] = true;
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

        // Camera movement logic
        if (!is2DMode && !isProgramSettingsMode && !showControlWindow && !showKeybindsWindow && waitingForKeyIndex == -1) {
            float camSpeed = camSpeedOptions[selectedCamSpeed].value;
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
        ClearBackground(themes[selectedTheme].bgColor);

        if (isProgramSettingsMode) {
            // ================= PROGRAMM SETTINGS PAGE (2 COLUMNS) =================
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), DARKGRAY);
            float cx = GetScreenWidth() / 2.0f;

            // --- CENTERED TITLE ---
            const char* titleText = "PROGRAMM SETTINGS";
            int titleFontSize = 30;
            int titleW = MeasureText(titleText, titleFontSize);
            DrawText(titleText, (int)(cx - titleW / 2.0f), 25, titleFontSize, RAYWHITE);
            DrawLine((int)(cx - 480), 65, (int)(cx + 480), 65, GRAY);

            // --- BACK BUTTON (TOP RIGHT) ---
            Rectangle btnBackTopRight = { (float)GetScreenWidth() - 140, 20, 120, 35 };
            bool btnBackHover = CheckCollisionPointRec(mousePos, btnBackTopRight);
            DrawRectangleRec(btnBackTopRight, btnBackHover ? ORANGE : GRAY);
            DrawRectangleLines((int)btnBackTopRight.x, (int)btnBackTopRight.y, (int)btnBackTopRight.width, (int)btnBackTopRight.height, RAYWHITE);
            
            int backTxtW = MeasureText("Back", 16);
            DrawText("Back", (int)(btnBackTopRight.x + (btnBackTopRight.width - backTxtW) / 2), (int)btnBackTopRight.y + 9, 16, BLACK);

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && btnBackHover) {
                isProgramSettingsMode = false;
                activeSettingsDropdown = -1;
                statusMsgTimer = 0.0f;
            }

            // Two-column layout coordinates
            float leftLabelX  = cx - 470.0f;
            float leftBtnX    = cx - 260.0f;
            float rightLabelX = cx + 30.0f;
            float rightBtnX   = cx + 250.0f;
            float btnW        = 220.0f;
            float btnH        = 34.0f;

            // Column headers
            DrawText("SYSTEM & TELEMETRY", (int)leftLabelX, 80, 16, SKYBLUE);
            DrawText("GRAPHICS, CAMERA & MODE", (int)rightLabelX, 80, 16, SKYBLUE);
            DrawLine((int)cx, 80, (int)cx, 440, GRAY);

            // ================= LEFT COLUMN (6 ROWS) =================
            // 1. Window Resolution
            float r1Y = 110.0f;
            DrawText("Window Resolution:", (int)leftLabelX, (int)r1Y + 8, 16, RAYWHITE);
            Rectangle resBtnRect = { leftBtnX, r1Y, btnW, btnH };
            bool resBtnHover = CheckCollisionPointRec(mousePos, resBtnRect);
            DrawRectangleRec(resBtnRect, resBtnHover ? LIGHTGRAY : WHITE);
            DrawRectangleLines((int)resBtnRect.x, (int)resBtnRect.y, (int)resBtnRect.width, (int)resBtnRect.height, BLACK);
            DrawText(TextFormat("%s [v]", resolutions[selectedResolution].label), (int)resBtnRect.x + 10, (int)resBtnRect.y + 10, 13, BLACK);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && resBtnHover) {
                activeSettingsDropdown = (activeSettingsDropdown == 0) ? -1 : 0;
            }

            // 2. Target FPS
            float r2Y = 160.0f;
            DrawText("Target Frame Rate:", (int)leftLabelX, (int)r2Y + 8, 16, RAYWHITE);
            Rectangle fpsBtnRect = { leftBtnX, r2Y, btnW, btnH };
            bool fpsBtnHover = CheckCollisionPointRec(mousePos, fpsBtnRect);
            DrawRectangleRec(fpsBtnRect, fpsBtnHover ? LIGHTGRAY : WHITE);
            DrawRectangleLines((int)fpsBtnRect.x, (int)fpsBtnRect.y, (int)fpsBtnRect.width, (int)fpsBtnRect.height, BLACK);
            DrawText(TextFormat("%s [v]", fpsOptions[selectedFps].label), (int)fpsBtnRect.x + 10, (int)fpsBtnRect.y + 10, 13, BLACK);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && fpsBtnHover) {
                activeSettingsDropdown = (activeSettingsDropdown == 1) ? -1 : 1;
            }

            // 3. Telemetry COM Port
            float r3Y = 210.0f;
            DrawText("Telemetry COM Port:", (int)leftLabelX, (int)r3Y + 8, 16, RAYWHITE);
            Rectangle comBtnRect = { leftBtnX, r3Y, btnW, btnH };
            bool comBtnHover = CheckCollisionPointRec(mousePos, comBtnRect);
            DrawRectangleRec(comBtnRect, comBtnHover ? LIGHTGRAY : WHITE);
            DrawRectangleLines((int)comBtnRect.x, (int)comBtnRect.y, (int)comBtnRect.width, (int)comBtnRect.height, BLACK);
            DrawText(TextFormat("%s [v]", comPorts[selectedComPort]), (int)comBtnRect.x + 10, (int)comBtnRect.y + 10, 13, BLACK);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && comBtnHover) {
                activeSettingsDropdown = (activeSettingsDropdown == 2) ? -1 : 2;
            }

            // 4. UART Baud Rate
            float r4Y = 260.0f;
            DrawText("UART Baud Rate:", (int)leftLabelX, (int)r4Y + 8, 16, RAYWHITE);
            Rectangle baudBtnRect = { leftBtnX, r4Y, btnW, btnH };
            bool baudBtnHover = CheckCollisionPointRec(mousePos, baudBtnRect);
            DrawRectangleRec(baudBtnRect, baudBtnHover ? LIGHTGRAY : WHITE);
            DrawRectangleLines((int)baudBtnRect.x, (int)baudBtnRect.y, (int)baudBtnRect.width, (int)baudBtnRect.height, BLACK);
            DrawText(TextFormat("%i baud [v]", baudRates[selectedBaudRate]), (int)baudBtnRect.x + 10, (int)baudBtnRect.y + 10, 13, BLACK);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && baudBtnHover) {
                activeSettingsDropdown = (activeSettingsDropdown == 3) ? -1 : 3;
            }

            // 5. Auto-Reconnect UART
            float r5Y = 310.0f;
            DrawText("Auto-Reconnect UART:", (int)leftLabelX, (int)r5Y + 8, 16, RAYWHITE);
            Rectangle autoRecBtnRect = { leftBtnX, r5Y, btnW, btnH };
            bool autoRecHover = CheckCollisionPointRec(mousePos, autoRecBtnRect);
            DrawRectangleRec(autoRecBtnRect, autoReconnectUart ? (autoRecHover ? LIME : GREEN) : (autoRecHover ? GRAY : LIGHTGRAY));
            DrawRectangleLines((int)autoRecBtnRect.x, (int)autoRecBtnRect.y, (int)autoRecBtnRect.width, (int)autoRecBtnRect.height, BLACK);
            DrawText(autoReconnectUart ? "Auto-Reconnect: ON" : "Auto-Reconnect: OFF", (int)autoRecBtnRect.x + 35, (int)autoRecBtnRect.y + 10, 13, BLACK);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && autoRecHover && activeSettingsDropdown == -1) {
                autoReconnectUart = !autoReconnectUart;
            }

            // 6. Zero / Tare All Sensors
            float r6Y = 360.0f;
            DrawText("Sensors Calibration:", (int)leftLabelX, (int)r6Y + 8, 16, RAYWHITE);
            Rectangle zeroBtnRect = { leftBtnX, r6Y, btnW, btnH };
            bool zeroBtnHover = CheckCollisionPointRec(mousePos, zeroBtnRect);
            DrawRectangleRec(zeroBtnRect, zeroBtnHover ? GOLD : ORANGE);
            DrawRectangleLines((int)zeroBtnRect.x, (int)zeroBtnRect.y, (int)zeroBtnRect.width, (int)zeroBtnRect.height, BLACK);
            DrawText("Zero / Tare All Sensors", (int)zeroBtnRect.x + 22, (int)zeroBtnRect.y + 10, 13, BLACK);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && zeroBtnHover && activeSettingsDropdown == -1) {
                for (int i = 0; i < totalParts; i++) {
                    parts[i].rotX = parts[i].defaultRotX;
                    parts[i].rotY = parts[i].defaultRotY;
                    parts[i].rotZ = parts[i].defaultRotZ;
                }
                statusMsgText = "All 13 sensor angles reset to zero!";
                statusMsgTimer = 3.0f;
            }

            // ================= RIGHT COLUMN (6 ROWS) =================
            // 7. 3D Scene Theme
            DrawText("3D Scene Theme:", (int)rightLabelX, (int)r1Y + 8, 16, RAYWHITE);
            Rectangle themeBtnRect = { rightBtnX, r1Y, btnW, btnH };
            bool themeBtnHover = CheckCollisionPointRec(mousePos, themeBtnRect);
            DrawRectangleRec(themeBtnRect, themeBtnHover ? LIGHTGRAY : WHITE);
            DrawRectangleLines((int)themeBtnRect.x, (int)themeBtnRect.y, (int)themeBtnRect.width, (int)themeBtnRect.height, BLACK);
            DrawText(TextFormat("%s [v]", themes[selectedTheme].label), (int)themeBtnRect.x + 10, (int)themeBtnRect.y + 10, 13, BLACK);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && themeBtnHover) {
                activeSettingsDropdown = (activeSettingsDropdown == 7) ? -1 : 7;
            }

            // 8. 3D Scene Overlays (Grid & Wires)
            DrawText("Scene Overlays:", (int)rightLabelX, (int)r2Y + 8, 16, RAYWHITE);
            Rectangle gridBtnRect = { rightBtnX, r2Y, 105, btnH };
            bool gridBtnHover = CheckCollisionPointRec(mousePos, gridBtnRect);
            DrawRectangleRec(gridBtnRect, showGroundGrid ? (gridBtnHover ? LIME : GREEN) : (gridBtnHover ? GRAY : LIGHTGRAY));
            DrawRectangleLines((int)gridBtnRect.x, (int)gridBtnRect.y, (int)gridBtnRect.width, (int)gridBtnRect.height, BLACK);
            DrawText(showGroundGrid ? "Grid: ON" : "Grid: OFF", (int)gridBtnRect.x + 18, (int)gridBtnRect.y + 10, 12, BLACK);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && gridBtnHover && activeSettingsDropdown == -1) {
                showGroundGrid = !showGroundGrid;
            }

            Rectangle wireBtnRect = { rightBtnX + 115, r2Y, 105, btnH };
            bool wireBtnHover = CheckCollisionPointRec(mousePos, wireBtnRect);
            DrawRectangleRec(wireBtnRect, showModelWireframe ? (wireBtnHover ? LIME : GREEN) : (wireBtnHover ? GRAY : LIGHTGRAY));
            DrawRectangleLines((int)wireBtnRect.x, (int)wireBtnRect.y, (int)wireBtnRect.width, (int)wireBtnRect.height, BLACK);
            DrawText(showModelWireframe ? "Wires: ON" : "Wires: OFF", (int)wireBtnRect.x + 14, (int)wireBtnRect.y + 10, 12, BLACK);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && wireBtnHover && activeSettingsDropdown == -1) {
                showModelWireframe = !showModelWireframe;
            }

            // 9. Camera Speed
            DrawText("Camera Speed:", (int)rightLabelX, (int)r3Y + 8, 16, RAYWHITE);
            Rectangle speedBtnRect = { rightBtnX, r3Y, btnW, btnH };
            bool speedBtnHover = CheckCollisionPointRec(mousePos, speedBtnRect);
            DrawRectangleRec(speedBtnRect, speedBtnHover ? LIGHTGRAY : WHITE);
            DrawRectangleLines((int)speedBtnRect.x, (int)speedBtnRect.y, (int)speedBtnRect.width, (int)speedBtnRect.height, BLACK);
            DrawText(TextFormat("%s [v]", camSpeedOptions[selectedCamSpeed].label), (int)speedBtnRect.x + 10, (int)speedBtnRect.y + 10, 13, BLACK);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && speedBtnHover) {
                activeSettingsDropdown = (activeSettingsDropdown == 4) ? -1 : 4;
            }

            // 10. Camera FOV
            DrawText("Camera Field of View:", (int)rightLabelX, (int)r4Y + 8, 16, RAYWHITE);
            Rectangle fovBtnRect = { rightBtnX, r4Y, btnW, btnH };
            bool fovBtnHover = CheckCollisionPointRec(mousePos, fovBtnRect);
            DrawRectangleRec(fovBtnRect, fovBtnHover ? LIGHTGRAY : WHITE);
            DrawRectangleLines((int)fovBtnRect.x, (int)fovBtnRect.y, (int)fovBtnRect.width, (int)fovBtnRect.height, BLACK);
            DrawText(TextFormat("%s [v]", fovOptions[selectedFov].label), (int)fovBtnRect.x + 10, (int)fovBtnRect.y + 10, 13, BLACK);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && fovBtnHover) {
                activeSettingsDropdown = (activeSettingsDropdown == 5) ? -1 : 5;
            }

            // 11. Motion Smoothing
            DrawText("Motion Smoothing:", (int)rightLabelX, (int)r5Y + 8, 16, RAYWHITE);
            Rectangle smoothBtnRect = { rightBtnX, r5Y, btnW, btnH };
            bool smoothBtnHover = CheckCollisionPointRec(mousePos, smoothBtnRect);
            DrawRectangleRec(smoothBtnRect, smoothBtnHover ? LIGHTGRAY : WHITE);
            DrawRectangleLines((int)smoothBtnRect.x, (int)smoothBtnRect.y, (int)smoothBtnRect.width, (int)smoothBtnRect.height, BLACK);
            DrawText(TextFormat("%s [v]", smoothingOptions[selectedSmoothing].label), (int)smoothBtnRect.x + 10, (int)smoothBtnRect.y + 10, 13, BLACK);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && smoothBtnHover) {
                activeSettingsDropdown = (activeSettingsDropdown == 6) ? -1 : 6;
            }

            // 12. Telemetry Source Mode (Live UART vs Demo Wave)
            DrawText("Telemetry Source:", (int)rightLabelX, (int)r6Y + 8, 16, RAYWHITE);
            Rectangle demoBtnRect = { rightBtnX, r6Y, btnW, btnH };
            bool demoBtnHover = CheckCollisionPointRec(mousePos, demoBtnRect);
            DrawRectangleRec(demoBtnRect, demoWaveMode ? (demoBtnHover ? SKYBLUE : BLUE) : (demoBtnHover ? GRAY : LIGHTGRAY));
            DrawRectangleLines((int)demoBtnRect.x, (int)demoBtnRect.y, (int)demoBtnRect.width, (int)demoBtnRect.height, BLACK);
            DrawText(demoWaveMode ? "Mode: DEMO SIMULATION" : "Mode: LIVE UART HARDWARE", (int)demoBtnRect.x + 18, (int)demoBtnRect.y + 10, 12, demoWaveMode ? RAYWHITE : BLACK);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && demoBtnHover && activeSettingsDropdown == -1) {
                demoWaveMode = !demoWaveMode;
                if (!demoWaveMode) {
                    for (int i = 0; i < 13; i++) bodyConnected[i] = false;
                }
            }

            // --- BOTTOM ACTION BUTTONS: SAVE & RESET TO DEFAULT ---
            float bottomY = (float)GetScreenHeight() - 70.0f;

            Rectangle btnSaveProgRect = { cx - 250, bottomY, 240, 42 };
            bool btnSaveProgHover = CheckCollisionPointRec(mousePos, btnSaveProgRect);
            DrawRectangleRec(btnSaveProgRect, btnSaveProgHover ? LIME : GREEN);
            DrawRectangleLines((int)btnSaveProgRect.x, (int)btnSaveProgRect.y, (int)btnSaveProgRect.width, (int)btnSaveProgRect.height, BLACK);
            int saveTxtW = MeasureText("Save Program Settings", 15);
            DrawText("Save Program Settings", (int)(btnSaveProgRect.x + (btnSaveProgRect.width - saveTxtW) / 2), (int)btnSaveProgRect.y + 13, 15, BLACK);

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && btnSaveProgHover && activeSettingsDropdown == -1) {
                std::ofstream progOut("program_settings.txt");
                if (progOut.is_open()) {
                    progOut << selectedResolution << " "
                            << selectedFps << " "
                            << selectedComPort << " "
                            << selectedBaudRate << " "
                            << (showGroundGrid ? 1 : 0) << " "
                            << (showModelWireframe ? 1 : 0) << " "
                            << selectedCamSpeed << " "
                            << selectedFov << " "
                            << selectedSmoothing << " "
                            << selectedTheme << " "
                            << (demoWaveMode ? 1 : 0) << " "
                            << (autoReconnectUart ? 1 : 0) << " "
                            << keyForward << " " << keyBackward << " "
                            << keyLeft << " " << keyRight << " "
                            << keyUpCam << " " << keyDownCam << "\n";
                    progOut.close();
                    statusMsgText = "All settings saved to program_settings.txt!";
                    statusMsgTimer = 3.0f;
                }
            }

            Rectangle btnResetProgRect = { cx + 10, bottomY, 240, 42 };
            bool btnResetProgHover = CheckCollisionPointRec(mousePos, btnResetProgRect);
            DrawRectangleRec(btnResetProgRect, btnResetProgHover ? RED : MAROON);
            DrawRectangleLines((int)btnResetProgRect.x, (int)btnResetProgRect.y, (int)btnResetProgRect.width, (int)btnResetProgRect.height, RAYWHITE);
            int resetTxtW = MeasureText("Reset All to Default", 15);
            DrawText("Reset All to Default", (int)(btnResetProgRect.x + (btnResetProgRect.width - resetTxtW) / 2), (int)btnResetProgRect.y + 13, 15, RAYWHITE);

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && btnResetProgHover && activeSettingsDropdown == -1) {
                selectedResolution = 0;
                selectedFps = 1;
                selectedComPort = 2;
                selectedBaudRate = 0;
                showGroundGrid = true;
                showModelWireframe = true;
                selectedCamSpeed = 1;
                selectedFov = 0;
                selectedSmoothing = 1;
                selectedTheme = 0;
                demoWaveMode = false;
                autoReconnectUart = true;

                for (int i = 0; i < 13; i++) bodyConnected[i] = false;
                ApplyWindowResolution(resolutions[0].width, resolutions[0].height);
                SetTargetFPS(fpsOptions[1].value);
                camera.fovy = fovOptions[0].value;

                statusMsgText = "All settings restored to defaults!";
                statusMsgTimer = 3.0f;
            }

            if (statusMsgTimer > 0.0f) {
                int msgW = MeasureText(statusMsgText, 16);
                DrawText(statusMsgText, (int)(cx - msgW / 2.0f), (int)bottomY - 28, 16, YELLOW);
            }

            // --- RENDER ACTIVE DROPDOWN LIST ON TOP OF ALL BUTTONS ---
            if (activeSettingsDropdown == 0) {
                for (int i = 0; i < totalResolutions; i++) {
                    Rectangle itemRect = { resBtnRect.x, resBtnRect.y + resBtnRect.height + (float)(i * 30), resBtnRect.width, 30 };
                    bool itemHover = CheckCollisionPointRec(mousePos, itemRect);
                    DrawRectangleRec(itemRect, (i == selectedResolution) ? SKYBLUE : (itemHover ? LIGHTGRAY : RAYWHITE));
                    DrawRectangleLines((int)itemRect.x, (int)itemRect.y, (int)itemRect.width, (int)itemRect.height, BLACK);
                    DrawText(resolutions[i].label, (int)itemRect.x + 10, (int)itemRect.y + 8, 13, BLACK);
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && itemHover) {
                        selectedResolution = i;
                        activeSettingsDropdown = -1;
                        ApplyWindowResolution(resolutions[i].width, resolutions[i].height);
                    }
                }
            } else if (activeSettingsDropdown == 1) {
                for (int i = 0; i < totalFpsOptions; i++) {
                    Rectangle itemRect = { fpsBtnRect.x, fpsBtnRect.y + fpsBtnRect.height + (float)(i * 30), fpsBtnRect.width, 30 };
                    bool itemHover = CheckCollisionPointRec(mousePos, itemRect);
                    DrawRectangleRec(itemRect, (i == selectedFps) ? SKYBLUE : (itemHover ? LIGHTGRAY : RAYWHITE));
                    DrawRectangleLines((int)itemRect.x, (int)itemRect.y, (int)itemRect.width, (int)itemRect.height, BLACK);
                    DrawText(fpsOptions[i].label, (int)itemRect.x + 10, (int)itemRect.y + 8, 13, BLACK);
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && itemHover) {
                        selectedFps = i;
                        activeSettingsDropdown = -1;
                        SetTargetFPS(fpsOptions[i].value);
                    }
                }
            } else if (activeSettingsDropdown == 2) {
                for (int i = 0; i < totalComPorts; i++) {
                    Rectangle itemRect = { comBtnRect.x, comBtnRect.y + comBtnRect.height + (float)(i * 26), comBtnRect.width, 26 };
                    bool itemHover = CheckCollisionPointRec(mousePos, itemRect);
                    DrawRectangleRec(itemRect, (i == selectedComPort) ? SKYBLUE : (itemHover ? LIGHTGRAY : RAYWHITE));
                    DrawRectangleLines((int)itemRect.x, (int)itemRect.y, (int)itemRect.width, (int)itemRect.height, BLACK);
                    DrawText(comPorts[i], (int)itemRect.x + 10, (int)itemRect.y + 6, 13, BLACK);
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && itemHover) {
                        selectedComPort = i;
                        activeSettingsDropdown = -1;
                    }
                }
            } else if (activeSettingsDropdown == 3) {
                for (int i = 0; i < totalBaudRates; i++) {
                    Rectangle itemRect = { baudBtnRect.x, baudBtnRect.y + baudBtnRect.height + (float)(i * 30), baudBtnRect.width, 30 };
                    bool itemHover = CheckCollisionPointRec(mousePos, itemRect);
                    DrawRectangleRec(itemRect, (i == selectedBaudRate) ? SKYBLUE : (itemHover ? LIGHTGRAY : RAYWHITE));
                    DrawRectangleLines((int)itemRect.x, (int)itemRect.y, (int)itemRect.width, (int)itemRect.height, BLACK);
                    DrawText(TextFormat("%i baud", baudRates[i]), (int)itemRect.x + 10, (int)itemRect.y + 8, 13, BLACK);
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && itemHover) {
                        selectedBaudRate = i;
                        activeSettingsDropdown = -1;
                    }
                }
            } else if (activeSettingsDropdown == 4) {
                for (int i = 0; i < totalCamSpeeds; i++) {
                    Rectangle itemRect = { speedBtnRect.x, speedBtnRect.y + speedBtnRect.height + (float)(i * 30), speedBtnRect.width, 30 };
                    bool itemHover = CheckCollisionPointRec(mousePos, itemRect);
                    DrawRectangleRec(itemRect, (i == selectedCamSpeed) ? SKYBLUE : (itemHover ? LIGHTGRAY : RAYWHITE));
                    DrawRectangleLines((int)itemRect.x, (int)itemRect.y, (int)itemRect.width, (int)itemRect.height, BLACK);
                    DrawText(camSpeedOptions[i].label, (int)itemRect.x + 10, (int)itemRect.y + 8, 13, BLACK);
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && itemHover) {
                        selectedCamSpeed = i;
                        activeSettingsDropdown = -1;
                    }
                }
            } else if (activeSettingsDropdown == 5) {
                for (int i = 0; i < totalFovOptions; i++) {
                    Rectangle itemRect = { fovBtnRect.x, fovBtnRect.y + fovBtnRect.height + (float)(i * 30), fovBtnRect.width, 30 };
                    bool itemHover = CheckCollisionPointRec(mousePos, itemRect);
                    DrawRectangleRec(itemRect, (i == selectedFov) ? SKYBLUE : (itemHover ? LIGHTGRAY : RAYWHITE));
                    DrawRectangleLines((int)itemRect.x, (int)itemRect.y, (int)itemRect.width, (int)itemRect.height, BLACK);
                    DrawText(fovOptions[i].label, (int)itemRect.x + 10, (int)itemRect.y + 8, 13, BLACK);
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && itemHover) {
                        selectedFov = i;
                        activeSettingsDropdown = -1;
                        camera.fovy = fovOptions[i].value;
                    }
                }
            } else if (activeSettingsDropdown == 6) {
                for (int i = 0; i < totalSmoothingOptions; i++) {
                    Rectangle itemRect = { smoothBtnRect.x, smoothBtnRect.y + smoothBtnRect.height + (float)(i * 30), smoothBtnRect.width, 30 };
                    bool itemHover = CheckCollisionPointRec(mousePos, itemRect);
                    DrawRectangleRec(itemRect, (i == selectedSmoothing) ? SKYBLUE : (itemHover ? LIGHTGRAY : RAYWHITE));
                    DrawRectangleLines((int)itemRect.x, (int)itemRect.y, (int)itemRect.width, (int)itemRect.height, BLACK);
                    DrawText(smoothingOptions[i].label, (int)itemRect.x + 10, (int)itemRect.y + 8, 13, BLACK);
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && itemHover) {
                        selectedSmoothing = i;
                        activeSettingsDropdown = -1;
                    }
                }
            } else if (activeSettingsDropdown == 7) {
                for (int i = 0; i < totalThemes; i++) {
                    Rectangle itemRect = { themeBtnRect.x, themeBtnRect.y + themeBtnRect.height + (float)(i * 30), themeBtnRect.width, 30 };
                    bool itemHover = CheckCollisionPointRec(mousePos, itemRect);
                    DrawRectangleRec(itemRect, (i == selectedTheme) ? SKYBLUE : (itemHover ? LIGHTGRAY : RAYWHITE));
                    DrawRectangleLines((int)itemRect.x, (int)itemRect.y, (int)itemRect.width, (int)itemRect.height, BLACK);
                    DrawText(themes[i].label, (int)itemRect.x + 10, (int)itemRect.y + 8, 13, BLACK);
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && itemHover) {
                        selectedTheme = i;
                        activeSettingsDropdown = -1;
                    }
                }
            }

        } else if (!is2DMode) {
            // ================= 3D MODE =================
            Color hudTextColor = (selectedTheme == 3) ? BLACK : RAYWHITE;
            Color hudSubColor  = (selectedTheme == 3) ? DARKGRAY : LIGHTGRAY;

            BeginMode3D(camera);
                if (showGroundGrid) {
                    DrawGrid(50, 10.0f);
                }

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
                            if (showModelWireframe) {
                                DrawModelWires(*models[i], (Vector3){0, 0, 0}, parts[i].scale, BLACK);
                            }
                        rlPopMatrix();
                    }
                }
            EndMode3D();

            DrawText("HS-1 Exoskeleton - Full Body Configurator", 10, 10, 20, hudTextColor);
            DrawText(TextFormat("%s @ %i baud | Mode: %s | Smooth: %.0f%%", 
                comPorts[selectedComPort], baudRates[selectedBaudRate],
                demoWaveMode ? "DEMO" : "UART", smoothingOptions[selectedSmoothing].value * 100.0f), 10, 35, 14, hudSubColor);
            
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

            Color infoColor = (selectedTheme == 3) ? DARKBLUE : YELLOW;
            Color valColor  = (selectedTheme == 3) ? DARKGREEN : GREEN;

            DrawText(TextFormat("Selected: %s", partNames[selectedPart]), 10, 90, 15, infoColor);
            DrawText(TextFormat("Pos: X=%.1f Y=%.1f Z=%.1f", parts[selectedPart].pos.x, parts[selectedPart].pos.y, parts[selectedPart].pos.z), 10, 110, 15, valColor);
            DrawText(TextFormat("Rot: X=%.1f Y=%.1f Z=%.1f", parts[selectedPart].rotX, parts[selectedPart].rotY, parts[selectedPart].rotZ), 10, 130, 15, valColor);
            DrawText(TextFormat("Scale: %.4f", parts[selectedPart].scale), 10, 150, 15, valColor);

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

            // --- BACK TO 3D BUTTON ---
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