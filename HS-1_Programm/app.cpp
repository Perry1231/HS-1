#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "stl_loader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#include <string>

struct PartTransform {
    Vector3 defaultPos;
    float defaultRotX;
    float defaultRotY;
    float defaultRotZ;
    float defaultScale;

    Vector3 pos;       // World pos for Body (root), local joint offset for child parts
    float rotX;        // Static STL mounting Pitch (from config, does not distort children)
    float rotY;        // Static STL mounting Yaw (from config, does not distort children)
    float rotZ;        // Static STL mounting Roll (from config, does not distort children)
    float scale;

    float sensorRotX;  // Dynamic IMU Pitch (propagates hierarchically to children)
    float sensorRotY;  // Dynamic IMU Yaw (propagates hierarchically to children)
    float sensorRotZ;  // Dynamic IMU Roll (propagates hierarchically to children)
};

struct MotionFrame {
    float timestamp;
    Vector3 angles[13]; // sensorRotX, sensorRotY, sensorRotZ for all 13 parts
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

// Automatically locate the folder containing "Models/" relative to the executable
bool SetupPortableWorkingDirectory() {
    const char* appDir = GetApplicationDirectory();
    if (appDir != nullptr) {
        ChangeDirectory(appDir);
    }

    if (DirectoryExists("Models")) return true;
    if (DirectoryExists("../Models")) { ChangeDirectory(".."); return true; }
    if (DirectoryExists("../../Models")) { ChangeDirectory("../.."); return true; }

    return false;
}

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

// Compute hierarchical joint transform (propagates dynamic sensor rotations down the limb chain)
Matrix GetPartJointMatrix(int index, const PartTransform parts[], const int parentIndex[]) {
    Matrix dynRot = MatrixMultiply(
        MatrixMultiply(MatrixRotateZ(parts[index].sensorRotZ * DEG2RAD), MatrixRotateY(parts[index].sensorRotY * DEG2RAD)),
        MatrixRotateX(parts[index].sensorRotX * DEG2RAD)
    );
    Matrix matTrans = MatrixTranslate(parts[index].pos.x, parts[index].pos.y, parts[index].pos.z);
    Matrix jointLocal = MatrixMultiply(dynRot, matTrans);

    int parent = parentIndex[index];
    if (parent != -1) {
        return MatrixMultiply(jointLocal, GetPartJointMatrix(parent, parts, parentIndex));
    }
    return jointLocal;
}

// Compute final world matrix for rendering (applies static STL mounting orientation locally)
Matrix GetPartWorldMatrix(int index, const PartTransform parts[], const int parentIndex[]) {
    Matrix baseRot = MatrixMultiply(
        MatrixMultiply(MatrixRotateZ(parts[index].rotZ * DEG2RAD), MatrixRotateY(parts[index].rotY * DEG2RAD)),
        MatrixRotateX(parts[index].rotX * DEG2RAD)
    );
    return MatrixMultiply(baseRot, GetPartJointMatrix(index, parts, parentIndex));
}

// Load smooth anti-aliased TrueType font (supports Latin + Cyrillic)
Font LoadSmoothFont(int baseSize) {
    const char* fontPaths[] = {
        "Fonts/font.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/consola.ttf"
    };

    for (const char* path : fontPaths) {
        if (FileExists(path)) {
            Font f = LoadFontEx(path, baseSize, nullptr, 400);
            if (f.texture.id != 0) {
                SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
                return f;
            }
        }
    }

    Font def = GetFontDefault();
    SetTextureFilter(def.texture, TEXTURE_FILTER_BILINEAR);
    return def;
}

int main() {
    bool modelsDirFound = SetupPortableWorkingDirectory();

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
    int selectedComPort = 2; // COM3

    // --- PART 2: COMMUNICATION & GRAPHICS ---
    const int baudRates[] = { 115200, 230400, 460800, 921600 };
    const int totalBaudRates = 4;
    int selectedBaudRate = 0; // 115200

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
        { "Dark Gray (Default)", DARKGRAY,                    LIGHTGRAY },
        { "Pure Black (OLED)",   BLACK,                       DARKGRAY  },
        { "Blueprint CAD",       (Color){ 18, 42, 76, 255 },  SKYBLUE   },
        { "Light Studio",        (Color){ 215, 220, 225, 255 }, GRAY    }
    };
    const int totalThemes = 4;
    int selectedTheme = 0;

    bool demoWaveMode = false;
    bool autoReconnectUart = true;
    bool showJointPivots = true;

    int activeSettingsDropdown = -1;

    // --- CUSTOMIZABLE KEYBINDS ---
    int keyForward = KEY_UP;
    int keyBackward = KEY_DOWN;
    int keyLeft = KEY_LEFT;
    int keyRight = KEY_RIGHT;
    int keyUpCam = KEY_LEFT_SHIFT;
    int keyDownCam = KEY_LEFT_CONTROL;

    // --- LOAD PROGRAM SETTINGS FROM LOCAL FILE ---
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

    // Enable 4x MSAA anti-aliasing for smooth 3D edges and lines
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(resolutions[selectedResolution].width, resolutions[selectedResolution].height, "HS-1 Exoskeleton - Full Body Configurator");
    ApplyWindowResolution(resolutions[selectedResolution].width, resolutions[selectedResolution].height);
    SetTargetFPS(fpsOptions[selectedFps].value);
    SetExitKey(KEY_NULL);

    // Load smooth vector UI font and redirect DrawText / MeasureText
    Font uiFont = LoadSmoothFont(36);
    #define DrawText(text, x, y, fontSize, color) \
        DrawTextEx(uiFont, (text), (Vector2){ (float)(x), (float)(y) }, (float)(fontSize), 1.0f, (color))
    #define MeasureText(text, fontSize) \
        ((int)MeasureTextEx(uiFont, (text), (float)(fontSize), 1.0f).x)

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

    int loadedModelsCount = 0;
    for (int i = 0; i < totalParts; i++) {
        if (isLoaded[i]) loadedModelsCount++;
    }

    // --- HIERARCHICAL SKELETON TREE (PARENT INDICES) ---
    const int parentIndex[13] = {
        1,  // 0: Right Biceps   -> attached to Body (1)
       -1,  // 1: Body           -> Root
        0,  // 2: Right Forearm  -> attached to Right Biceps (0)
        2,  // 3: Right Hand     -> attached to Right Forearm (2)
        6,  // 4: Right Foot     -> attached to Right Leg Low (6)
        1,  // 5: Right Leg High -> attached to Body (1)
        5,  // 6: Right Leg Low  -> attached to Right Leg High (5)
        1,  // 7: Left Biceps    -> attached to Body (1)
        7,  // 8: Left Forearm   -> attached to Left Biceps (7)
        8,  // 9: Left Hand      -> attached to Left Forearm (8)
       12,  // 10: Left Foot     -> attached to Left Leg Low (12)
        1,  // 11: Left Leg High -> attached to Body (1)
       11   // 12: Left Leg Low  -> attached to Left Leg High (11)
    };
    
    PartTransform parts[13] = {
        { {-12.4f, 1.6f, 5.2f},     0.0f, -90.0f, 0.0f, 0.05f,   {-12.4f, 1.6f, 5.2f},     0.0f, -90.0f, 0.0f, 0.05f,   0.0f, 0.0f, 0.0f }, // 1. Right Biceps
        { {11.8f, 66.0f, -5.0f},   90.0f,  -1.0f, 0.0f, 0.05f,   {11.8f, 66.0f, -5.0f},   90.0f,  -1.0f, 0.0f, 0.05f,   0.0f, 0.0f, 0.0f }, // 2. Body (Root)
        { {0.0f, -12.1f, 0.0f},     0.0f,  90.0f, 0.0f, 0.05f,   {0.0f, -12.1f, 0.0f},     0.0f,  90.0f, 0.0f, 0.05f,   0.0f, 0.0f, 0.0f }, // 3. Right Forearm
        { {-1.9f, -7.3f, 0.0f},     0.0f,  90.0f, 0.0f, 0.05f,   {-1.9f, -7.3f, 0.0f},     0.0f,  90.0f, 0.0f, 0.05f,   0.0f, 0.0f, 0.0f }, // 4. Right Hand
        { {-0.6f, -6.6f, 9.8f},     0.0f,   2.0f, 0.0f, 0.05f,   {-0.6f, -6.6f, 9.8f},     0.0f,   2.0f, 0.0f, 0.05f,   0.0f, 0.0f, 0.0f }, // 5. Right Foot
        { {-6.8f, -36.0f, 4.2f},   90.0f, 180.0f, 0.0f, 0.05f,   {-6.8f, -36.0f, 4.2f},   90.0f, 180.0f, 0.0f, 0.05f,   0.0f, 0.0f, 0.0f }, // 6. Right Leg High
        { {0.0f, -23.4f, -0.2f}, -270.0f,-180.0f, 0.0f, 0.05f,   {0.0f, -23.4f, -0.2f}, -270.0f,-180.0f, 0.0f, 0.05f,   0.0f, 0.0f, 0.0f }, // 7. Right Leg Low
        { {13.0f, 1.4f, 5.0f},      0.0f,  90.0f, 0.0f, 0.05f,   {13.0f, 1.4f, 5.0f},      0.0f,  90.0f, 0.0f, 0.05f,   0.0f, 0.0f, 0.0f }, // 8. Left Biceps
        { {0.0f, -11.8f, 0.0f},     0.0f,   0.0f, 0.0f, 0.05f,   {0.0f, -11.8f, 0.0f},     0.0f,   0.0f, 0.0f, 0.05f,   0.0f, 0.0f, 0.0f }, // 9. Left Forearm
        { {-1.8f, -7.4f, 0.0f},     0.0f,  90.0f, 0.0f, 0.05f,   {-1.8f, -7.4f, 0.0f},     0.0f,  90.0f, 0.0f, 0.05f,   0.0f, 0.0f, 0.0f }, // 10. Left Hand
        { {-1.4f, -6.4f, 10.0f},    0.0f,  -1.0f, 0.0f, 0.05f,   {-1.4f, -6.4f, 10.0f},    0.0f,  -1.0f, 0.0f, 0.05f,   0.0f, 0.0f, 0.0f }, // 11. Left Foot
        { {6.4f, -36.0f, 4.6f},   -89.0f, 360.0f, 0.0f, 0.05f,   {6.4f, -36.0f, 4.6f},   -89.0f, 360.0f, 0.0f, 0.05f,   0.0f, 0.0f, 0.0f }, // 12. Left Leg High
        { {-0.2f, -23.4f, -0.2f}, 270.0f,   0.0f, 0.0f, 0.05f,   {-0.2f, -23.4f, -0.2f}, 270.0f,   0.0f, 0.0f, 0.05f,   0.0f, 0.0f, 0.0f }  // 13. Left Leg Low
    };

    const char* partNames[13] = { 
        "Right Biceps", "Body (Root)", "Right Forearm", "Right Hand", 
        "Right Foot", "Right Leg High", "Right Leg Low",
        "Left Biceps", "Left Forearm", "Left Hand", 
        "Left Foot", "Left Leg High", "Left Leg Low"
    };

    // Logical symmetrical order for the Part Controls dropdown list
    const int dropdownOrder[13] = {
        1,  // 1. Body (Root)
        0,  // 2. Right Biceps
        7,  // 3. Left Biceps
        2,  // 4. Right Forearm
        8,  // 5. Left Forearm
        3,  // 6. Right Hand
        9,  // 7. Left Hand
        5,  // 8. Right Leg High
        11, // 9. Left Leg High
        6,  // 10. Right Leg Low
        12, // 11. Left Leg Low
        4,  // 12. Right Foot
        10  // 13. Left Foot
    };

    // Symmetrical limb colors (Right == Left)
    Color partColors[13] = {
        RED,     // 0: Right Biceps
        ORANGE,  // 1: Body (Root)
        BLUE,    // 2: Right Forearm
        SKYBLUE, // 3: Right Hand
        GREEN,   // 4: Right Foot
        YELLOW,  // 5: Right Leg High
        PURPLE,  // 6: Right Leg Low
        RED,     // 7: Left Biceps
        BLUE,    // 8: Left Forearm
        SKYBLUE, // 9: Left Hand
        GREEN,   // 10: Left Foot
        YELLOW,  // 11: Left Leg High
        PURPLE   // 12: Left Leg Low
    };

    // Screen modes
    bool is2DMode = false;
    bool isProgramSettingsMode = false;
    bool isRecorderMode = false;
    bool shouldExitApp = false;

    float statusMsgTimer = 0.0f;
    const char* statusMsgText = "";
    
    // Motion Recorder state
    std::vector<MotionFrame> recordedFrames;
    bool isRecording = false;
    bool isPlaying = false;
    bool savedDemoStateBeforePlay = false;
    float recordStartTime = 0.0f;
    float playbackTime = 0.0f;
    float playbackSpeed = 1.0f; // 0.5x, 1.0x, 2.0x
    int playbackFrameIndex = 0;
    float recorderMsgTimer = 0.0f;
    const char* recorderMsgText = "";

    bool bodyConnected[13] = { false }; 

    // Telemetry rate & latency monitor state
    int telemetryPacketCounter = 0;
    float telemetryTimer = 0.0f;
    float measuredTelemetryHz = 0.0f;
    float measuredLatencyMs = 0.0f;
    double lastTelemetryTimestamp = 0.0;

    int selectedPart = -1; // -1 = None selected
    int hoveredPart3D = -1;
    bool showDropdownList = false;
    int waitingForKeyIndex = -1;

    // Load saved world coordinates from file and auto-convert to parent-relative offsets
    std::ifstream inFile("exoskeleton_config.txt");
    if (inFile.is_open()) {
        Vector3 loadedWorldPos[13];
        for (int i = 0; i < totalParts; i++) {
            inFile >> loadedWorldPos[i].x >> loadedWorldPos[i].y >> loadedWorldPos[i].z 
                   >> parts[i].rotX >> parts[i].rotY >> parts[i].rotZ >> parts[i].scale;
        }
        inFile.close();

        for (int i = 0; i < totalParts; i++) {
            parts[i].sensorRotX = 0.0f;
            parts[i].sensorRotY = 0.0f;
            parts[i].sensorRotZ = 0.0f;

            int pIdx = parentIndex[i];
            if (pIdx == -1) {
                parts[i].pos = loadedWorldPos[i];
            } else {
                parts[i].pos = Vector3Subtract(loadedWorldPos[i], loadedWorldPos[pIdx]);
            }
        }
    }

    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 30.0f, 150.0f };
    camera.target = (Vector3){ 0.0f, 20.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = fovOptions[selectedFov].value;
    camera.projection = CAMERA_PERSPECTIVE;

    bool showControlWindow = false;
    bool showKeybindsWindow = false;
    bool showHelpWindow = false;
    bool cursorUnlocked = false;

    while (!WindowShouldClose() && !shouldExitApp) {
        float dt = GetFrameTime();
        if (statusMsgTimer > 0.0f) statusMsgTimer -= dt;
        if (recorderMsgTimer > 0.0f) recorderMsgTimer -= dt;

        // Global per-frame flag: becomes true as soon as any dropdown/modal consumes a click
        bool uiClickConsumed = false;
        bool telemetryFrameArrived = false;

        // Time-Accurate Motion Playback or Demo Simulation update
        if (isPlaying && !recordedFrames.empty()) {
            float totalDuration = recordedFrames.back().timestamp;
            playbackTime += dt * playbackSpeed;
            if (totalDuration > 0.001f && playbackTime > totalDuration) {
                playbackTime = fmodf(playbackTime, totalDuration);
                playbackFrameIndex = 0;
            }

            while (playbackFrameIndex + 1 < (int)recordedFrames.size() &&
                   recordedFrames[playbackFrameIndex + 1].timestamp <= playbackTime) {
                playbackFrameIndex++;
            }

            const MotionFrame& frame = recordedFrames[playbackFrameIndex];
            for (int i = 0; i < totalParts; i++) {
                parts[i].sensorRotX = frame.angles[i].x;
                parts[i].sensorRotY = frame.angles[i].y;
                parts[i].sensorRotZ = frame.angles[i].z;
            }
            telemetryFrameArrived = true;
        } else if (demoWaveMode) {
            float t = (float)GetTime() * 2.2f;
            float shoulderWave = sinf(t) * 25.0f;
            float elbowWave    = (sinf(t - 0.6f) + 1.0f) * 20.0f;

            parts[0].sensorRotX = -shoulderWave;
            parts[2].sensorRotX = -elbowWave;
            parts[7].sensorRotX =  shoulderWave;
            parts[8].sensorRotX = -elbowWave;

            parts[5].sensorRotX  =  shoulderWave * 0.5f;
            parts[6].sensorRotX  =  fabsf(shoulderWave) * 0.6f;
            parts[11].sensorRotX = -shoulderWave * 0.5f;
            parts[12].sensorRotX =  fabsf(shoulderWave) * 0.6f;

            for (int i = 0; i < 13; i++) bodyConnected[i] = true;
            telemetryFrameArrived = true;
        }

        // Update Telemetry Hz & Latency metrics
        if (telemetryFrameArrived) {
            double now = GetTime();
            if (lastTelemetryTimestamp > 0.0) {
                float instantMs = (float)((now - lastTelemetryTimestamp) * 1000.0);
                measuredLatencyMs = (measuredLatencyMs <= 0.01f)
                    ? instantMs
                    : (measuredLatencyMs * 0.85f + instantMs * 0.15f);
            }
            lastTelemetryTimestamp = now;
            telemetryPacketCounter++;
        }

        telemetryTimer += dt;
        if (telemetryTimer >= 0.5f) {
            measuredTelemetryHz = (float)telemetryPacketCounter / telemetryTimer;
            telemetryPacketCounter = 0;
            telemetryTimer = 0.0f;
            if (measuredTelemetryHz < 0.1f) {
                measuredLatencyMs = 0.0f;
                lastTelemetryTimestamp = 0.0;
            }
        }

        // Capture frame if recording is active
        if (isRecording) {
            MotionFrame mf;
            mf.timestamp = (float)GetTime() - recordStartTime;
            for (int i = 0; i < totalParts; i++) {
                mf.angles[i] = (Vector3){ parts[i].sensorRotX, parts[i].sensorRotY, parts[i].sensorRotZ };
            }
            recordedFrames.push_back(mf);
        }

        // Toggle Help overlay with F1
        if (!isProgramSettingsMode && !isRecorderMode && IsKeyPressed(KEY_F1)) {
            showHelpWindow = !showHelpWindow;
            if (showHelpWindow) {
                showControlWindow = false;
                showKeybindsWindow = false;
                showDropdownList = false;
            }
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            if (showHelpWindow) {
                showHelpWindow = false;
            } else {
                cursorUnlocked = !cursorUnlocked;
                if (cursorUnlocked) EnableCursor();
                else DisableCursor();
            }
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

        // Camera movement logic (works in 3D mode & Recorder 3D preview)
        if (!is2DMode && !isProgramSettingsMode && !showControlWindow && !showKeybindsWindow && !showHelpWindow && waitingForKeyIndex == -1) {
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

        if (!is2DMode && !isProgramSettingsMode && !showHelpWindow && IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            UpdateCamera(&camera, CAMERA_FREE);
        }

        if (!isProgramSettingsMode && !showHelpWindow && IsKeyPressed(KEY_R)) {
            camera.position = (Vector3){ 0.0f, 30.0f, 150.0f };
            camera.target = (Vector3){ 0.0f, 20.0f, 0.0f };
        }

        if (!is2DMode && !isProgramSettingsMode && !isRecorderMode && !showHelpWindow && selectedPart != -1) {
            if (IsKeyDown(KEY_KP_ADD) || IsKeyDown(KEY_EQUAL))      parts[selectedPart].scale += 0.001f;
            if (IsKeyDown(KEY_KP_SUBTRACT) || IsKeyDown(KEY_MINUS)) parts[selectedPart].scale -= 0.001f;
        }

        BeginDrawing();
        ClearBackground(themes[selectedTheme].bgColor);

        if (isProgramSettingsMode) {
            // ================= PROGRAMM SETTINGS PAGE (2 COLUMNS) =================
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), DARKGRAY);
            float cx = GetScreenWidth() / 2.0f;

            const char* titleText = "PROGRAMM SETTINGS";
            int titleFontSize = 30;
            int titleW = MeasureText(titleText, titleFontSize);
            DrawText(titleText, (int)(cx - titleW / 2.0f), 25, titleFontSize, RAYWHITE);
            DrawLine((int)(cx - 480), 65, (int)(cx + 480), 65, GRAY);

            float leftLabelX  = cx - 470.0f;
            float leftBtnX    = cx - 260.0f;
            float rightLabelX = cx + 30.0f;
            float rightBtnX   = cx + 250.0f;
            float btnW        = 220.0f;
            float btnH        = 34.0f;

            float r1Y = 110.0f;
            float r2Y = 160.0f;
            float r3Y = 210.0f;
            float r4Y = 260.0f;
            float r5Y = 310.0f;
            float r6Y = 360.0f;

            Rectangle resBtnRect    = { leftBtnX,  r1Y, btnW, btnH };
            Rectangle fpsBtnRect    = { leftBtnX,  r2Y, btnW, btnH };
            Rectangle comBtnRect    = { leftBtnX,  r3Y, btnW, btnH };
            Rectangle baudBtnRect   = { leftBtnX,  r4Y, btnW, btnH };
            Rectangle themeBtnRect  = { rightBtnX, r1Y, btnW, btnH };
            Rectangle speedBtnRect  = { rightBtnX, r3Y, btnW, btnH };
            Rectangle fovBtnRect    = { rightBtnX, r4Y, btnW, btnH };
            Rectangle smoothBtnRect = { rightBtnX, r5Y, btnW, btnH };

            // 1. PROCESS OPEN DROPDOWN INPUT FIRST
            if (activeSettingsDropdown != -1 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                uiClickConsumed = true;

                if (activeSettingsDropdown == 0) {
                    for (int i = 0; i < totalResolutions; i++) {
                        Rectangle itemRect = { resBtnRect.x, resBtnRect.y + resBtnRect.height + (float)(i * 30), resBtnRect.width, 30 };
                        if (CheckCollisionPointRec(mousePos, itemRect)) {
                            selectedResolution = i;
                            ApplyWindowResolution(resolutions[i].width, resolutions[i].height);
                            break;
                        }
                    }
                } else if (activeSettingsDropdown == 1) {
                    for (int i = 0; i < totalFpsOptions; i++) {
                        Rectangle itemRect = { fpsBtnRect.x, fpsBtnRect.y + fpsBtnRect.height + (float)(i * 30), fpsBtnRect.width, 30 };
                        if (CheckCollisionPointRec(mousePos, itemRect)) {
                            selectedFps = i;
                            SetTargetFPS(fpsOptions[i].value);
                            break;
                        }
                    }
                } else if (activeSettingsDropdown == 2) {
                    for (int i = 0; i < totalComPorts; i++) {
                        Rectangle itemRect = { comBtnRect.x, comBtnRect.y + comBtnRect.height + (float)(i * 26), comBtnRect.width, 26 };
                        if (CheckCollisionPointRec(mousePos, itemRect)) {
                            selectedComPort = i;
                            break;
                        }
                    }
                } else if (activeSettingsDropdown == 3) {
                    for (int i = 0; i < totalBaudRates; i++) {
                        Rectangle itemRect = { baudBtnRect.x, baudBtnRect.y + baudBtnRect.height + (float)(i * 30), baudBtnRect.width, 30 };
                        if (CheckCollisionPointRec(mousePos, itemRect)) {
                            selectedBaudRate = i;
                            break;
                        }
                    }
                } else if (activeSettingsDropdown == 4) {
                    for (int i = 0; i < totalCamSpeeds; i++) {
                        Rectangle itemRect = { speedBtnRect.x, speedBtnRect.y + speedBtnRect.height + (float)(i * 30), speedBtnRect.width, 30 };
                        if (CheckCollisionPointRec(mousePos, itemRect)) {
                            selectedCamSpeed = i;
                            break;
                        }
                    }
                } else if (activeSettingsDropdown == 5) {
                    for (int i = 0; i < totalFovOptions; i++) {
                        Rectangle itemRect = { fovBtnRect.x, fovBtnRect.y + fovBtnRect.height + (float)(i * 30), fovBtnRect.width, 30 };
                        if (CheckCollisionPointRec(mousePos, itemRect)) {
                            selectedFov = i;
                            camera.fovy = fovOptions[i].value;
                            break;
                        }
                    }
                } else if (activeSettingsDropdown == 6) {
                    for (int i = 0; i < totalSmoothingOptions; i++) {
                        Rectangle itemRect = { smoothBtnRect.x, smoothBtnRect.y + smoothBtnRect.height + (float)(i * 30), smoothBtnRect.width, 30 };
                        if (CheckCollisionPointRec(mousePos, itemRect)) {
                            selectedSmoothing = i;
                            break;
                        }
                    }
                } else if (activeSettingsDropdown == 7) {
                    for (int i = 0; i < totalThemes; i++) {
                        Rectangle itemRect = { themeBtnRect.x, themeBtnRect.y + themeBtnRect.height + (float)(i * 30), themeBtnRect.width, 30 };
                        if (CheckCollisionPointRec(mousePos, itemRect)) {
                            selectedTheme = i;
                            break;
                        }
                    }
                }

                activeSettingsDropdown = -1;
            }

            bool canClickUI = (!uiClickConsumed && activeSettingsDropdown == -1);

            Rectangle btnBackTopRight = { (float)GetScreenWidth() - 140, 20, 120, 35 };
            bool btnBackHover = canClickUI && CheckCollisionPointRec(mousePos, btnBackTopRight);
            DrawRectangleRec(btnBackTopRight, btnBackHover ? ORANGE : GRAY);
            DrawRectangleLines((int)btnBackTopRight.x, (int)btnBackTopRight.y, (int)btnBackTopRight.width, (int)btnBackTopRight.height, RAYWHITE);
            int backTxtW = MeasureText("Back", 16);
            DrawText("Back", (int)(btnBackTopRight.x + (btnBackTopRight.width - backTxtW) / 2), (int)btnBackTopRight.y + 9, 16, BLACK);

            if (canClickUI && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && btnBackHover) {
                isProgramSettingsMode = false;
                activeSettingsDropdown = -1;
                statusMsgTimer = 0.0f;
            }

            DrawText("SYSTEM & TELEMETRY", (int)leftLabelX, 80, 16, SKYBLUE);
            DrawText("GRAPHICS, CAMERA & MODE", (int)rightLabelX, 80, 16, SKYBLUE);
            DrawLine((int)cx, 80, (int)cx, 440, GRAY);

            auto DrawDropdownHeader = [&](Rectangle rect, const char* text, int dropdownId) {
                bool hover = canClickUI && CheckCollisionPointRec(mousePos, rect);
                DrawRectangleRec(rect, hover ? LIGHTGRAY : WHITE);
                DrawRectangleLines((int)rect.x, (int)rect.y, (int)rect.width, (int)rect.height, BLACK);
                DrawText(text, (int)rect.x + 10, (int)rect.y + 10, 14, BLACK);
                if (canClickUI && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hover) {
                    activeSettingsDropdown = dropdownId;
                    uiClickConsumed = true;
                    canClickUI = false;
                }
            };

            // LEFT COLUMN
            DrawText("Window Resolution:", (int)leftLabelX, (int)r1Y + 8, 16, RAYWHITE);
            DrawDropdownHeader(resBtnRect, TextFormat("%s [v]", resolutions[selectedResolution].label), 0);

            DrawText("Target Frame Rate:", (int)leftLabelX, (int)r2Y + 8, 16, RAYWHITE);
            DrawDropdownHeader(fpsBtnRect, TextFormat("%s [v]", fpsOptions[selectedFps].label), 1);

            DrawText("Telemetry COM Port:", (int)leftLabelX, (int)r3Y + 8, 16, RAYWHITE);
            DrawDropdownHeader(comBtnRect, TextFormat("%s [v]", comPorts[selectedComPort]), 2);

            DrawText("UART Baud Rate:", (int)leftLabelX, (int)r4Y + 8, 16, RAYWHITE);
            DrawDropdownHeader(baudBtnRect, TextFormat("%i baud [v]", baudRates[selectedBaudRate]), 3);

            DrawText("Auto-Reconnect UART:", (int)leftLabelX, (int)r5Y + 8, 16, RAYWHITE);
            Rectangle autoRecBtnRect = { leftBtnX, r5Y, btnW, btnH };
            bool autoRecHover = canClickUI && CheckCollisionPointRec(mousePos, autoRecBtnRect);
            DrawRectangleRec(autoRecBtnRect, autoReconnectUart ? (autoRecHover ? LIME : GREEN) : (autoRecHover ? GRAY : LIGHTGRAY));
            DrawRectangleLines((int)autoRecBtnRect.x, (int)autoRecBtnRect.y, (int)autoRecBtnRect.width, (int)autoRecBtnRect.height, BLACK);
            DrawText(autoReconnectUart ? "Auto-Reconnect: ON" : "Auto-Reconnect: OFF", (int)autoRecBtnRect.x + 35, (int)autoRecBtnRect.y + 10, 14, BLACK);
            if (canClickUI && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && autoRecHover) {
                autoReconnectUart = !autoReconnectUart;
            }

            DrawText("Sensors Calibration:", (int)leftLabelX, (int)r6Y + 8, 16, RAYWHITE);
            Rectangle zeroBtnRect = { leftBtnX, r6Y, btnW, btnH };
            bool zeroBtnHover = canClickUI && CheckCollisionPointRec(mousePos, zeroBtnRect);
            DrawRectangleRec(zeroBtnRect, zeroBtnHover ? GOLD : ORANGE);
            DrawRectangleLines((int)zeroBtnRect.x, (int)zeroBtnRect.y, (int)zeroBtnRect.width, (int)zeroBtnRect.height, BLACK);
            DrawText("Zero / Tare All Sensors", (int)zeroBtnRect.x + 25, (int)zeroBtnRect.y + 10, 14, BLACK);
            if (canClickUI && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && zeroBtnHover) {
                for (int i = 0; i < totalParts; i++) {
                    parts[i].sensorRotX = 0.0f;
                    parts[i].sensorRotY = 0.0f;
                    parts[i].sensorRotZ = 0.0f;
                }
                statusMsgText = "All 13 sensor angles reset to zero!";
                statusMsgTimer = 3.0f;
            }

            // RIGHT COLUMN
            DrawText("3D Scene Theme:", (int)rightLabelX, (int)r1Y + 8, 16, RAYWHITE);
            DrawDropdownHeader(themeBtnRect, TextFormat("%s [v]", themes[selectedTheme].label), 7);

            DrawText("Scene Overlays:", (int)rightLabelX, (int)r2Y + 8, 16, RAYWHITE);
            Rectangle gridBtnRect = { rightBtnX, r2Y, 105, btnH };
            bool gridBtnHover = canClickUI && CheckCollisionPointRec(mousePos, gridBtnRect);
            DrawRectangleRec(gridBtnRect, showGroundGrid ? (gridBtnHover ? LIME : GREEN) : (gridBtnHover ? GRAY : LIGHTGRAY));
            DrawRectangleLines((int)gridBtnRect.x, (int)gridBtnRect.y, (int)gridBtnRect.width, (int)gridBtnRect.height, BLACK);
            DrawText(showGroundGrid ? "Grid: ON" : "Grid: OFF", (int)gridBtnRect.x + 22, (int)gridBtnRect.y + 10, 13, BLACK);
            if (canClickUI && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && gridBtnHover) showGroundGrid = !showGroundGrid;

            Rectangle wireBtnRect = { rightBtnX + 115, r2Y, 105, btnH };
            bool wireBtnHover = canClickUI && CheckCollisionPointRec(mousePos, wireBtnRect);
            DrawRectangleRec(wireBtnRect, showModelWireframe ? (wireBtnHover ? LIME : GREEN) : (wireBtnHover ? GRAY : LIGHTGRAY));
            DrawRectangleLines((int)wireBtnRect.x, (int)wireBtnRect.y, (int)wireBtnRect.width, (int)wireBtnRect.height, BLACK);
            DrawText(showModelWireframe ? "Wires: ON" : "Wires: OFF", (int)wireBtnRect.x + 18, (int)wireBtnRect.y + 10, 13, BLACK);
            if (canClickUI && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && wireBtnHover) showModelWireframe = !showModelWireframe;

            DrawText("Camera Speed:", (int)rightLabelX, (int)r3Y + 8, 16, RAYWHITE);
            DrawDropdownHeader(speedBtnRect, TextFormat("%s [v]", camSpeedOptions[selectedCamSpeed].label), 4);

            DrawText("Camera Field of View:", (int)rightLabelX, (int)r4Y + 8, 16, RAYWHITE);
            DrawDropdownHeader(fovBtnRect, TextFormat("%s [v]", fovOptions[selectedFov].label), 5);

            DrawText("Motion Smoothing:", (int)rightLabelX, (int)r5Y + 8, 16, RAYWHITE);
            DrawDropdownHeader(smoothBtnRect, TextFormat("%s [v]", smoothingOptions[selectedSmoothing].label), 6);

            DrawText("Telemetry Source:", (int)rightLabelX, (int)r6Y + 8, 16, RAYWHITE);
            Rectangle demoBtnRect = { rightBtnX, r6Y, btnW, btnH };
            bool demoBtnHover = canClickUI && CheckCollisionPointRec(mousePos, demoBtnRect);
            DrawRectangleRec(demoBtnRect, demoWaveMode ? (demoBtnHover ? SKYBLUE : BLUE) : (demoBtnHover ? GRAY : LIGHTGRAY));
            DrawRectangleLines((int)demoBtnRect.x, (int)demoBtnRect.y, (int)demoBtnRect.width, (int)demoBtnRect.height, BLACK);
            DrawText(demoWaveMode ? "Mode: DEMO SIMULATION" : "Mode: LIVE UART HARDWARE", (int)demoBtnRect.x + 16, (int)demoBtnRect.y + 10, 13, demoWaveMode ? RAYWHITE : BLACK);
            if (canClickUI && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && demoBtnHover) {
                demoWaveMode = !demoWaveMode;
                if (!demoWaveMode) {
                    for (int i = 0; i < 13; i++) {
                        bodyConnected[i] = false;
                        parts[i].sensorRotX = 0.0f;
                        parts[i].sensorRotY = 0.0f;
                        parts[i].sensorRotZ = 0.0f;
                    }
                }
            }

            // BOTTOM ACTION BUTTONS
            float bottomY = (float)GetScreenHeight() - 70.0f;

            Rectangle btnSaveProgRect = { cx - 250, bottomY, 240, 42 };
            bool btnSaveProgHover = canClickUI && CheckCollisionPointRec(mousePos, btnSaveProgRect);
            DrawRectangleRec(btnSaveProgRect, btnSaveProgHover ? LIME : GREEN);
            DrawRectangleLines((int)btnSaveProgRect.x, (int)btnSaveProgRect.y, (int)btnSaveProgRect.width, (int)btnSaveProgRect.height, BLACK);
            int saveTxtW = MeasureText("Save Program Settings", 16);
            DrawText("Save Program Settings", (int)(btnSaveProgRect.x + (btnSaveProgRect.width - saveTxtW) / 2), (int)btnSaveProgRect.y + 13, 16, BLACK);

            if (canClickUI && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && btnSaveProgHover) {
                std::ofstream progOut("program_settings.txt");
                if (progOut.is_open()) {
                    progOut << selectedResolution << " " << selectedFps << " " << selectedComPort << " "
                            << selectedBaudRate << " " << (showGroundGrid ? 1 : 0) << " " << (showModelWireframe ? 1 : 0) << " "
                            << selectedCamSpeed << " " << selectedFov << " " << selectedSmoothing << " "
                            << selectedTheme << " " << (demoWaveMode ? 1 : 0) << " " << (autoReconnectUart ? 1 : 0) << " "
                            << keyForward << " " << keyBackward << " " << keyLeft << " " << keyRight << " "
                            << keyUpCam << " " << keyDownCam << "\n";
                    progOut.close();
                    statusMsgText = "All settings saved to program_settings.txt!";
                    statusMsgTimer = 3.0f;
                }
            }

            Rectangle btnResetProgRect = { cx + 10, bottomY, 240, 42 };
            bool btnResetProgHover = canClickUI && CheckCollisionPointRec(mousePos, btnResetProgRect);
            DrawRectangleRec(btnResetProgRect, btnResetProgHover ? RED : MAROON);
            DrawRectangleLines((int)btnResetProgRect.x, (int)btnResetProgRect.y, (int)btnResetProgRect.width, (int)btnResetProgRect.height, RAYWHITE);
            int resetTxtW = MeasureText("Reset All to Default", 16);
            DrawText("Reset All to Default", (int)(btnResetProgRect.x + (btnResetProgRect.width - resetTxtW) / 2), (int)btnResetProgRect.y + 13, 16, RAYWHITE);

            if (canClickUI && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && btnResetProgHover) {
                selectedResolution = 0; selectedFps = 1; selectedComPort = 2; selectedBaudRate = 0;
                showGroundGrid = true; showModelWireframe = true; selectedCamSpeed = 1; selectedFov = 0;
                selectedSmoothing = 1; selectedTheme = 0; demoWaveMode = false; autoReconnectUart = true;
                for (int i = 0; i < 13; i++) {
                    bodyConnected[i] = false;
                    parts[i].sensorRotX = parts[i].sensorRotY = parts[i].sensorRotZ = 0.0f;
                }
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

            // 2. RENDER OPEN DROPDOWN OVERLAY ON TOP OF EVERYTHING
            if (activeSettingsDropdown == 0) {
                for (int i = 0; i < totalResolutions; i++) {
                    Rectangle itemRect = { resBtnRect.x, resBtnRect.y + resBtnRect.height + (float)(i * 30), resBtnRect.width, 30 };
                    bool itemHover = CheckCollisionPointRec(mousePos, itemRect);
                    DrawRectangleRec(itemRect, (i == selectedResolution) ? SKYBLUE : (itemHover ? LIGHTGRAY : RAYWHITE));
                    DrawRectangleLines((int)itemRect.x, (int)itemRect.y, (int)itemRect.width, (int)itemRect.height, BLACK);
                    DrawText(resolutions[i].label, (int)itemRect.x + 10, (int)itemRect.y + 8, 14, BLACK);
                }
            } else if (activeSettingsDropdown == 1) {
                for (int i = 0; i < totalFpsOptions; i++) {
                    Rectangle itemRect = { fpsBtnRect.x, fpsBtnRect.y + fpsBtnRect.height + (float)(i * 30), fpsBtnRect.width, 30 };
                    bool itemHover = CheckCollisionPointRec(mousePos, itemRect);
                    DrawRectangleRec(itemRect, (i == selectedFps) ? SKYBLUE : (itemHover ? LIGHTGRAY : RAYWHITE));
                    DrawRectangleLines((int)itemRect.x, (int)itemRect.y, (int)itemRect.width, (int)itemRect.height, BLACK);
                    DrawText(fpsOptions[i].label, (int)itemRect.x + 10, (int)itemRect.y + 8, 14, BLACK);
                }
            } else if (activeSettingsDropdown == 2) {
                for (int i = 0; i < totalComPorts; i++) {
                    Rectangle itemRect = { comBtnRect.x, comBtnRect.y + comBtnRect.height + (float)(i * 26), comBtnRect.width, 26 };
                    bool itemHover = CheckCollisionPointRec(mousePos, itemRect);
                    DrawRectangleRec(itemRect, (i == selectedComPort) ? SKYBLUE : (itemHover ? LIGHTGRAY : RAYWHITE));
                    DrawRectangleLines((int)itemRect.x, (int)itemRect.y, (int)itemRect.width, (int)itemRect.height, BLACK);
                    DrawText(comPorts[i], (int)itemRect.x + 10, (int)itemRect.y + 6, 14, BLACK);
                }
            } else if (activeSettingsDropdown == 3) {
                for (int i = 0; i < totalBaudRates; i++) {
                    Rectangle itemRect = { baudBtnRect.x, baudBtnRect.y + baudBtnRect.height + (float)(i * 30), baudBtnRect.width, 30 };
                    bool itemHover = CheckCollisionPointRec(mousePos, itemRect);
                    DrawRectangleRec(itemRect, (i == selectedBaudRate) ? SKYBLUE : (itemHover ? LIGHTGRAY : RAYWHITE));
                    DrawRectangleLines((int)itemRect.x, (int)itemRect.y, (int)itemRect.width, (int)itemRect.height, BLACK);
                    DrawText(TextFormat("%i baud", baudRates[i]), (int)itemRect.x + 10, (int)itemRect.y + 8, 14, BLACK);
                }
            } else if (activeSettingsDropdown == 4) {
                for (int i = 0; i < totalCamSpeeds; i++) {
                    Rectangle itemRect = { speedBtnRect.x, speedBtnRect.y + speedBtnRect.height + (float)(i * 30), speedBtnRect.width, 30 };
                    bool itemHover = CheckCollisionPointRec(mousePos, itemRect);
                    DrawRectangleRec(itemRect, (i == selectedCamSpeed) ? SKYBLUE : (itemHover ? LIGHTGRAY : RAYWHITE));
                    DrawRectangleLines((int)itemRect.x, (int)itemRect.y, (int)itemRect.width, (int)itemRect.height, BLACK);
                    DrawText(camSpeedOptions[i].label, (int)itemRect.x + 10, (int)itemRect.y + 8, 14, BLACK);
                }
            } else if (activeSettingsDropdown == 5) {
                for (int i = 0; i < totalFovOptions; i++) {
                    Rectangle itemRect = { fovBtnRect.x, fovBtnRect.y + fovBtnRect.height + (float)(i * 30), fovBtnRect.width, 30 };
                    bool itemHover = CheckCollisionPointRec(mousePos, itemRect);
                    DrawRectangleRec(itemRect, (i == selectedFov) ? SKYBLUE : (itemHover ? LIGHTGRAY : RAYWHITE));
                    DrawRectangleLines((int)itemRect.x, (int)itemRect.y, (int)itemRect.width, (int)itemRect.height, BLACK);
                    DrawText(fovOptions[i].label, (int)itemRect.x + 10, (int)itemRect.y + 8, 14, BLACK);
                }
            } else if (activeSettingsDropdown == 6) {
                for (int i = 0; i < totalSmoothingOptions; i++) {
                    Rectangle itemRect = { smoothBtnRect.x, smoothBtnRect.y + smoothBtnRect.height + (float)(i * 30), smoothBtnRect.width, 30 };
                    bool itemHover = CheckCollisionPointRec(mousePos, itemRect);
                    DrawRectangleRec(itemRect, (i == selectedSmoothing) ? SKYBLUE : (itemHover ? LIGHTGRAY : RAYWHITE));
                    DrawRectangleLines((int)itemRect.x, (int)itemRect.y, (int)itemRect.width, (int)itemRect.height, BLACK);
                    DrawText(smoothingOptions[i].label, (int)itemRect.x + 10, (int)itemRect.y + 8, 14, BLACK);
                }
            } else if (activeSettingsDropdown == 7) {
                for (int i = 0; i < totalThemes; i++) {
                    Rectangle itemRect = { themeBtnRect.x, themeBtnRect.y + themeBtnRect.height + (float)(i * 30), themeBtnRect.width, 30 };
                    bool itemHover = CheckCollisionPointRec(mousePos, itemRect);
                    DrawRectangleRec(itemRect, (i == selectedTheme) ? SKYBLUE : (itemHover ? LIGHTGRAY : RAYWHITE));
                    DrawRectangleLines((int)itemRect.x, (int)itemRect.y, (int)itemRect.width, (int)itemRect.height, BLACK);
                    DrawText(themes[i].label, (int)itemRect.x + 10, (int)itemRect.y + 8, 14, BLACK);
                }
            }

        } else if (isRecorderMode) {
            // ================= MOTION RECORDER TAB (LEFT: 3D SKELETON | RIGHT: CONTROLS) =================
            int screenW = GetScreenWidth();
            int screenH = GetScreenHeight();
            int panelW  = 340;
            int viewW   = screenW - panelW;

            Color hudTextColor = (selectedTheme == 3) ? BLACK : RAYWHITE;
            Color hudSubColor  = (selectedTheme == 3) ? DARKGRAY : LIGHTGRAY;

            // Precalculate joint and world matrices
            Matrix jointMatrices[13];
            Matrix worldMatrices[13];
            for (int i = 0; i < totalParts; i++) {
                jointMatrices[i] = GetPartJointMatrix(i, parts, parentIndex);
                worldMatrices[i] = GetPartWorldMatrix(i, parts, parentIndex);
            }

            // --- LEFT SIDE: 3D SKELETON VIEWPORT ---
            BeginScissorMode(0, 0, viewW, screenH);
                BeginMode3D(camera);
                    if (showGroundGrid) DrawGrid(50, 10.0f);

                    Model* models[13] = { 
                        &modelBicepsR, &modelBody, &modelForearmR, &modelHandR, &modelFootR, &modelLegHighR, &modelLegLowR,
                        &modelBicepsL, &modelForearmL, &modelHandL, &modelFootL, &modelLegHighL, &modelLegLowL 
                    };

                    for (int i = 0; i < 13; i++) {
                        Vector3 jointPos = { jointMatrices[i].m12, jointMatrices[i].m13, jointMatrices[i].m14 };
                        int pIdx = parentIndex[i];
                        if (pIdx != -1 && showJointPivots) {
                            Vector3 parentJointPos = { jointMatrices[pIdx].m12, jointMatrices[pIdx].m13, jointMatrices[pIdx].m14 };
                            DrawLine3D(parentJointPos, jointPos, YELLOW);
                            DrawSphere(jointPos, 0.55f, WHITE);
                        }
                        if (isLoaded[i]) {
                            rlPushMatrix();
                                rlMultMatrixf(MatrixToFloat(worldMatrices[i]));
                                DrawModel(*models[i], (Vector3){0, 0, 0}, parts[i].scale, partColors[i]);
                                if (showModelWireframe) DrawModelWires(*models[i], (Vector3){0, 0, 0}, parts[i].scale, BLACK);
                            rlPopMatrix();
                        }
                    }
                EndMode3D();

                // Left Viewport HUD
                DrawText("HS-1 LIVE SKELETON VIEWPORT", 20, 18, 22, hudTextColor);
                DrawText("Hold RMB + Mouse to orbit | Arrows to move | R to reset camera", 20, 46, 14, hudSubColor);

                if (isRecording) {
                    DrawRectangle(20, 72, 165, 28, (Color){ 180, 20, 20, 220 });
                    DrawRectangleLines(20, 72, 165, 28, RAYWHITE);
                    if ((int)(GetTime() * 4.0) % 2 == 0) DrawCircle(36, 86, 6.0f, WHITE);
                    DrawText("REC IN PROGRESS", 50, 79, 14, RAYWHITE);
                } else if (isPlaying) {
                    DrawRectangle(20, 72, 175, 28, (Color){ 20, 140, 40, 220 });
                    DrawRectangleLines(20, 72, 175, 28, RAYWHITE);
                    DrawText(TextFormat("PLAYBACK (%.1fx)", playbackSpeed), 32, 79, 14, RAYWHITE);
                }
            EndScissorMode();

            // --- RIGHT SIDE: MOTION RECORDER CONTROL PANEL ---
            float px = (float)viewW;
            DrawRectangle((int)px, 0, panelW, screenH, (Color){ 26, 30, 36, 255 });
            DrawLine((int)px, 0, (int)px, screenH, SKYBLUE);

            // Panel Header & Back Button
            DrawText("MOTION RECORDER", (int)px + 20, 20, 20, RAYWHITE);

            Rectangle btnBackRec = { px + panelW - 115.0f, 16.0f, 98.0f, 32.0f };
            bool btnBackRecHover = CheckCollisionPointRec(mousePos, btnBackRec);
            DrawRectangleRec(btnBackRec, btnBackRecHover ? ORANGE : GRAY);
            DrawRectangleLines((int)btnBackRec.x, (int)btnBackRec.y, (int)btnBackRec.width, (int)btnBackRec.height, RAYWHITE);
            int backRecW = MeasureText("Back", 15);
            DrawText("Back", (int)(btnBackRec.x + (btnBackRec.width - backRecW) / 2), (int)btnBackRec.y + 9, 15, BLACK);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && btnBackRecHover) {
                isRecorderMode = false;
                isRecording = false;
                if (isPlaying) {
                    isPlaying = false;
                    demoWaveMode = savedDemoStateBeforePlay;
                }
            }

            DrawLine((int)px + 15, 60, (int)px + panelW - 15, 60, GRAY);

            // Telemetry Status Box
            float durationSec = recordedFrames.empty() ? 0.0f : recordedFrames.back().timestamp;
            const char* recStateLabel = isRecording ? "RECORDING" : (isPlaying ? "PLAYING" : "READY / IDLE");
            Color recStateColor = isRecording ? RED : (isPlaying ? LIME : SKYBLUE);

            DrawText("Status:", (int)px + 20, 75, 15, LIGHTGRAY);
            DrawText(recStateLabel, (int)px + 95, 75, 15, recStateColor);

            DrawText("Frames:", (int)px + 20, 100, 15, LIGHTGRAY);
            DrawText(TextFormat("%i", (int)recordedFrames.size()), (int)px + 95, 100, 15, RAYWHITE);

            DrawText("Duration:", (int)px + 20, 125, 15, LIGHTGRAY);
            DrawText(TextFormat("%.2f s", durationSec), (int)px + 95, 125, 15, RAYWHITE);

            DrawText("Source:", (int)px + 20, 150, 15, LIGHTGRAY);
            DrawText(demoWaveMode ? "DEMO SIMULATION" : comPorts[selectedComPort], (int)px + 95, 150, 15, GOLD);

            // Timeline Progress Bar
            float barX = px + 20.0f;
            float barY = 182.0f;
            float barW = panelW - 40.0f;
            DrawRectangle((int)barX, (int)barY, (int)barW, 14, DARKGRAY);
            DrawRectangleLines((int)barX, (int)barY, (int)barW, 14, GRAY);

            if (!recordedFrames.empty()) {
                float progress = 1.0f;
                if (isPlaying && durationSec > 0.001f) {
                    progress = Clamp(playbackTime / durationSec, 0.0f, 1.0f);
                }
                DrawRectangle((int)barX + 1, (int)barY + 1, (int)((barW - 2) * progress), 12, isRecording ? RED : LIME);
            }

            // Playback Speed Selector (0.5x | 1.0x | 2.0x)
            DrawText("Playback Speed:", (int)px + 20, 212, 14, LIGHTGRAY);
            float speeds[3] = { 0.5f, 1.0f, 2.0f };
            const char* speedLabels[3] = { "0.5x", "1.0x", "2.0x" };
            for (int s = 0; s < 3; s++) {
                Rectangle sBtn = { px + 140.0f + s * 60.0f, 207.0f, 54.0f, 24.0f };
                bool sHover = CheckCollisionPointRec(mousePos, sBtn);
                bool isSelectedSpeed = (fabsf(playbackSpeed - speeds[s]) < 0.05f);
                DrawRectangleRec(sBtn, isSelectedSpeed ? SKYBLUE : (sHover ? GRAY : DARKGRAY));
                DrawRectangleLines((int)sBtn.x, (int)sBtn.y, (int)sBtn.width, (int)sBtn.height, BLACK);
                DrawText(speedLabels[s], (int)sBtn.x + 12, (int)sBtn.y + 5, 13, isSelectedSpeed ? BLACK : RAYWHITE);
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && sHover) {
                    playbackSpeed = speeds[s];
                }
            }

            // --- VERTICAL STACK OF MAIN BUTTONS ON THE RIGHT ---
            float btnX = px + 25.0f;
            float btnW = panelW - 50.0f;
            float btnH = 46.0f;
            float curY = 250.0f;
            float gapY = 14.0f;

            // 1. REC Button
            Rectangle btnRec = { btnX, curY, btnW, btnH };
            bool hoverRec = CheckCollisionPointRec(mousePos, btnRec);
            DrawRectangleRec(btnRec, isRecording ? RED : (hoverRec ? MAROON : (Color){ 170, 30, 30, 255 }));
            DrawRectangleLines((int)btnRec.x, (int)btnRec.y, (int)btnRec.width, (int)btnRec.height, RAYWHITE);
            int wRec = MeasureText("REC (Start Capture)", 16);
            DrawText("REC (Start Capture)", (int)(btnRec.x + (btnRec.width - wRec) / 2), (int)btnRec.y + 15, 16, RAYWHITE);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hoverRec) {
                if (isPlaying) {
                    isPlaying = false;
                    demoWaveMode = savedDemoStateBeforePlay;
                }
                recordedFrames.clear();
                isRecording = true;
                recordStartTime = (float)GetTime();
                playbackFrameIndex = 0;
                playbackTime = 0.0f;
                recorderMsgText = "Recording started...";
                recorderMsgTimer = 2.5f;
            }
            curY += btnH + gapY;

            // 2. STOP Button
            Rectangle btnStop = { btnX, curY, btnW, btnH };
            bool hoverStop = CheckCollisionPointRec(mousePos, btnStop);
            DrawRectangleRec(btnStop, hoverStop ? LIGHTGRAY : GRAY);
            DrawRectangleLines((int)btnStop.x, (int)btnStop.y, (int)btnStop.width, (int)btnStop.height, BLACK);
            int wStop = MeasureText("STOP", 16);
            DrawText("STOP", (int)(btnStop.x + (btnStop.width - wStop) / 2), (int)btnStop.y + 15, 16, BLACK);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hoverStop) {
                if (isRecording || isPlaying) {
                    isRecording = false;
                    if (isPlaying) {
                        isPlaying = false;
                        demoWaveMode = savedDemoStateBeforePlay;
                    }
                    recorderMsgText = TextFormat("Stopped (%i frames)", (int)recordedFrames.size());
                    recorderMsgTimer = 3.0f;
                }
            }
            curY += btnH + gapY;

            // 3. PLAY Button
            Rectangle btnPlay = { btnX, curY, btnW, btnH };
            bool hoverPlay = CheckCollisionPointRec(mousePos, btnPlay);
            DrawRectangleRec(btnPlay, isPlaying ? LIME : (hoverPlay ? GREEN : DARKGREEN));
            DrawRectangleLines((int)btnPlay.x, (int)btnPlay.y, (int)btnPlay.width, (int)btnPlay.height, RAYWHITE);
            int wPlay = MeasureText("PLAY (Replay Motion)", 16);
            DrawText("PLAY (Replay Motion)", (int)(btnPlay.x + (btnPlay.width - wPlay) / 2), (int)btnPlay.y + 15, 16, isPlaying ? BLACK : RAYWHITE);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hoverPlay) {
                if (!recordedFrames.empty()) {
                    isRecording = false;
                    if (!isPlaying) {
                        savedDemoStateBeforePlay = demoWaveMode;
                        demoWaveMode = false;
                    }
                    isPlaying = true;
                    playbackFrameIndex = 0;
                    playbackTime = 0.0f;
                    recorderMsgText = "Playing recorded trajectory...";
                    recorderMsgTimer = 2.5f;
                } else {
                    recorderMsgText = "No frames! Press REC or LOAD CSV.";
                    recorderMsgTimer = 3.0f;
                }
            }
            curY += btnH + gapY;

            // 4. EXPORT CSV Button
            Rectangle btnExport = { btnX, curY, btnW, btnH };
            bool hoverExport = CheckCollisionPointRec(mousePos, btnExport);
            DrawRectangleRec(btnExport, hoverExport ? SKYBLUE : BLUE);
            DrawRectangleLines((int)btnExport.x, (int)btnExport.y, (int)btnExport.width, (int)btnExport.height, RAYWHITE);
            int wExp = MeasureText("EXPORT CSV", 16);
            DrawText("EXPORT CSV", (int)(btnExport.x + (btnExport.width - wExp) / 2), (int)btnExport.y + 15, 16, RAYWHITE);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hoverExport) {
                if (!recordedFrames.empty()) {
                    std::ofstream csvOut("hs1_motion_record.csv");
                    if (csvOut.is_open()) {
                        csvOut << "Frame,Timestamp_s";
                        for (int p = 0; p < totalParts; p++) {
                            csvOut << ",Part" << p << "_Pitch,Part" << p << "_Yaw,Part" << p << "_Roll";
                        }
                        csvOut << "\n";

                        for (size_t f = 0; f < recordedFrames.size(); f++) {
                            csvOut << f << "," << recordedFrames[f].timestamp;
                            for (int p = 0; p < totalParts; p++) {
                                csvOut << "," << recordedFrames[f].angles[p].x
                                       << "," << recordedFrames[f].angles[p].y
                                       << "," << recordedFrames[f].angles[p].z;
                            }
                            csvOut << "\n";
                        }
                        csvOut.close();
                        recorderMsgText = "Saved: hs1_motion_record.csv";
                        recorderMsgTimer = 3.5f;
                    }
                } else {
                    recorderMsgText = "Nothing to export! Press REC first.";
                    recorderMsgTimer = 3.0f;
                }
            }
            curY += btnH + gapY;

            // 5. LOAD CSV Button
            Rectangle btnLoadCsv = { btnX, curY, btnW, 36.0f };
            bool hoverLoadCsv = CheckCollisionPointRec(mousePos, btnLoadCsv);
            DrawRectangleRec(btnLoadCsv, hoverLoadCsv ? GOLD : DARKGRAY);
            DrawRectangleLines((int)btnLoadCsv.x, (int)btnLoadCsv.y, (int)btnLoadCsv.width, (int)btnLoadCsv.height, LIGHTGRAY);
            int wLoad = MeasureText("LOAD CSV FOR PLAYBACK", 14);
            DrawText("LOAD CSV FOR PLAYBACK", (int)(btnLoadCsv.x + (btnLoadCsv.width - wLoad) / 2), (int)btnLoadCsv.y + 11, 14, hoverLoadCsv ? BLACK : RAYWHITE);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hoverLoadCsv) {
                std::ifstream csvIn("hs1_motion_record.csv");
                if (csvIn.is_open()) {
                    std::string line;
                    std::getline(csvIn, line);
                    std::vector<MotionFrame> loaded;
                    while (std::getline(csvIn, line)) {
                        if (line.empty()) continue;
                        std::stringstream ss(line);
                        std::string cell;
                        MotionFrame mf;
                        if (!std::getline(ss, cell, ',')) continue;
                        if (!std::getline(ss, cell, ',')) continue;
                        mf.timestamp = std::stof(cell);

                        bool validRow = true;
                        for (int p = 0; p < totalParts; p++) {
                            if (!std::getline(ss, cell, ',')) { validRow = false; break; }
                            mf.angles[p].x = std::stof(cell);
                            if (!std::getline(ss, cell, ',')) { validRow = false; break; }
                            mf.angles[p].y = std::stof(cell);
                            if (!std::getline(ss, cell, ',')) { validRow = false; break; }
                            mf.angles[p].z = std::stof(cell);
                        }
                        if (validRow) loaded.push_back(mf);
                    }
                    csvIn.close();
                    if (!loaded.empty()) {
                        isRecording = false;
                        isPlaying = false;
                        recordedFrames = loaded;
                        playbackFrameIndex = 0;
                        playbackTime = 0.0f;
                        recorderMsgText = TextFormat("Loaded %i frames from CSV!", (int)recordedFrames.size());
                        recorderMsgTimer = 3.5f;
                    } else {
                        recorderMsgText = "CSV file is empty or invalid.";
                        recorderMsgTimer = 3.0f;
                    }
                } else {
                    recorderMsgText = "hs1_motion_record.csv not found!";
                    recorderMsgTimer = 3.0f;
                }
            }

            if (recorderMsgTimer > 0.0f) {
                DrawText(recorderMsgText, (int)px + 25, (int)curY + 48, 15, YELLOW);
            }

        } else if (!is2DMode) {
            // ================= 3D HIERARCHICAL SKELETON MODE =================
            Color hudTextColor = (selectedTheme == 3) ? BLACK : RAYWHITE;
            Color hudSubColor  = (selectedTheme == 3) ? DARKGRAY : LIGHTGRAY;

            // Precalculate joint and final world matrices for all 13 segments
            Matrix jointMatrices[13];
            Matrix worldMatrices[13];
            for (int i = 0; i < totalParts; i++) {
                jointMatrices[i] = GetPartJointMatrix(i, parts, parentIndex);
                worldMatrices[i] = GetPartWorldMatrix(i, parts, parentIndex);
            }

            Model* models[13] = { 
                &modelBicepsR, &modelBody, &modelForearmR, &modelHandR, &modelFootR, &modelLegHighR, &modelLegLowR,
                &modelBicepsL, &modelForearmL, &modelHandL, &modelFootL, &modelLegHighL, &modelLegLowL 
            };

            // 1. PROCESS OPEN PART DROPDOWN CLICK FIRST
            if (showControlWindow && showDropdownList && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Rectangle winRect = { (float)GetScreenWidth() - 320, 238, 300, 390 };
                for (int row = 0; row < totalParts; row++) {
                    int partIdx = dropdownOrder[row];
                    Rectangle itemRect = { winRect.x + 15, winRect.y + 75 + (float)(row * 20), 270, 20 };
                    if (CheckCollisionPointRec(mousePos, itemRect)) {
                        selectedPart = (selectedPart == partIdx) ? -1 : partIdx;
                        break;
                    }
                }
                showDropdownList = false;
                uiClickConsumed = true;
            }

            bool canInteract3D = (!uiClickConsumed && !showDropdownList && !showHelpWindow);

            // Check if mouse is hovering over any 2D UI panel/button so 3D raycast doesn't trigger through UI
            Rectangle topMenuArea   = { (float)GetScreenWidth() - 190, 0, 190, 235 };
            Rectangle controlWinBox = { (float)GetScreenWidth() - 320, 238, 300, 390 };
            Rectangle keybindWinBox = { (float)GetScreenWidth() - 320, 238, 300, 310 };
            Rectangle btnTo2DBox    = { (float)GetScreenWidth() / 2.0f - 110, (float)GetScreenHeight() - 60, 220, 40 };
            Rectangle btnExitBox    = { (float)GetScreenWidth() - 130, (float)GetScreenHeight() - 55, 110, 35 };

            bool pointerOverUI = CheckCollisionPointRec(mousePos, topMenuArea) ||
                                 CheckCollisionPointRec(mousePos, btnTo2DBox) ||
                                 CheckCollisionPointRec(mousePos, btnExitBox) ||
                                 (showControlWindow && CheckCollisionPointRec(mousePos, controlWinBox)) ||
                                 (showKeybindsWindow && CheckCollisionPointRec(mousePos, keybindWinBox));

            // 3D Mouse Raycast Picking (Click once to select, click again to deselect)
            hoveredPart3D = -1;
            if (canInteract3D && !pointerOverUI) {
                Ray pickRay = GetMouseRay(mousePos, camera);
                float closestDist = 1e30f;

                for (int i = 0; i < totalParts; i++) {
                    if (!isLoaded[i] || models[i]->meshCount == 0) continue;
                    Matrix meshTransform = MatrixMultiply(MatrixScale(parts[i].scale, parts[i].scale, parts[i].scale), worldMatrices[i]);
                    RayCollision hit = GetRayCollisionMesh(pickRay, models[i]->meshes[0], meshTransform);
                    if (hit.hit && hit.distance < closestDist) {
                        closestDist = hit.distance;
                        hoveredPart3D = i;
                    }
                }

                if (hoveredPart3D != -1 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    if (selectedPart == hoveredPart3D) {
                        selectedPart = -1; // Deselect when clicking the already selected part
                    } else {
                        selectedPart = hoveredPart3D;
                    }
                    uiClickConsumed = true;
                }
            }

            BeginMode3D(camera);
                if (showGroundGrid) {
                    DrawGrid(50, 10.0f);
                }

                for (int i = 0; i < 13; i++) {
                    Vector3 jointPos = { jointMatrices[i].m12, jointMatrices[i].m13, jointMatrices[i].m14 };

                    // Draw kinematic bone link between parent joint and child joint
                    int pIdx = parentIndex[i];
                    if (pIdx != -1 && showJointPivots) {
                        Vector3 parentJointPos = { jointMatrices[pIdx].m12, jointMatrices[pIdx].m13, jointMatrices[pIdx].m14 };
                        DrawLine3D(parentJointPos, jointPos, YELLOW);
                        DrawSphere(jointPos, (i == selectedPart) ? 0.9f : 0.5f, (i == selectedPart) ? LIME : WHITE);
                    }

                    if (isLoaded[i]) {
                        rlPushMatrix();
                            rlMultMatrixf(MatrixToFloat(worldMatrices[i]));
                            DrawModel(*models[i], (Vector3){0, 0, 0}, parts[i].scale, partColors[i]);
                            if (i == selectedPart) {
                                DrawModelWires(*models[i], (Vector3){0, 0, 0}, parts[i].scale, LIME);
                            } else if (i == hoveredPart3D) {
                                DrawModelWires(*models[i], (Vector3){0, 0, 0}, parts[i].scale, GOLD);
                            } else if (showModelWireframe) {
                                DrawModelWires(*models[i], (Vector3){0, 0, 0}, parts[i].scale, BLACK);
                            }
                        rlPopMatrix();
                    }
                }
            EndMode3D();

            // Count active connected nodes for HUD status
            int connectedNodesCount = 0;
            for (int i = 0; i < totalParts; i++) {
                if (bodyConnected[i]) connectedNodesCount++;
            }

            DrawText("HS-1 Exoskeleton - Hierarchical Kinematic Configurator", 10, 10, 21, hudTextColor);
            DrawText(TextFormat("%s @ %i baud | Mode: %s | Nodes: %i/13", 
                comPorts[selectedComPort], baudRates[selectedBaudRate],
                demoWaveMode ? "DEMO" : "UART", connectedNodesCount), 10, 36, 15, hudSubColor);

            // Live Telemetry Rate & Latency Badge
            Color rateColor = (measuredTelemetryHz >= 50.0f) ? LIME : ((measuredTelemetryHz > 0.1f) ? YELLOW : hudSubColor);
            DrawText(TextFormat("Telemetry Rate: %.0f Hz | Frame Delta: %.1f ms | Click 3D part to toggle select",
                measuredTelemetryHz, measuredLatencyMs), 10, 58, 14, rateColor);

            if (!modelsDirFound || loadedModelsCount == 0) {
                DrawRectangle(10, 82, 490, 26, MAROON);
                DrawRectangleLines(10, 82, 490, 26, RED);
                DrawText("WARNING: 'Models/' folder not found next to HS1_App.exe!", 20, 88, 14, RAYWHITE);
            }
            
            // --- 2D MAP BUTTON (BOTTOM CENTER) ---
            Rectangle btnTo2D = { (float)GetScreenWidth() / 2.0f - 110, (float)GetScreenHeight() - 60, 220, 40 };
            bool btn2DHover = canInteract3D && CheckCollisionPointRec(mousePos, btnTo2D);
            DrawRectangleRec(btnTo2D, btn2DHover ? SKYBLUE : BLUE);
            DrawRectangleLines((int)btnTo2D.x, (int)btnTo2D.y, (int)btnTo2D.width, (int)btnTo2D.height, BLACK);
            int map2dW = MeasureText("Open 2D Body Map", 15);
            DrawText("Open 2D Body Map", (int)(btnTo2D.x + (btnTo2D.width - map2dW) / 2), (int)btnTo2D.y + 12, 15, RAYWHITE);

            if (canInteract3D && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && btn2DHover) {
                is2DMode = true;
                showControlWindow = false;
                showKeybindsWindow = false;
                showHelpWindow = false;
            }

            // --- EXIT APP BUTTON (BOTTOM RIGHT) ---
            Rectangle btnExitApp = { (float)GetScreenWidth() - 130, (float)GetScreenHeight() - 55, 110, 35 };
            bool btnExitHover = canInteract3D && CheckCollisionPointRec(mousePos, btnExitApp);
            DrawRectangleRec(btnExitApp, btnExitHover ? RED : MAROON);
            DrawRectangleLines((int)btnExitApp.x, (int)btnExitApp.y, (int)btnExitApp.width, (int)btnExitApp.height, RAYWHITE);
            int exitTxtW = MeasureText("Exit [X]", 15);
            DrawText("Exit [X]", (int)(btnExitApp.x + (btnExitApp.width - exitTxtW) / 2), (int)btnExitApp.y + 10, 15, RAYWHITE);

            if (canInteract3D && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && btnExitHover) shouldExitApp = true;

            // --- TOP RIGHT MENU BUTTONS ---
            Rectangle btnOpenRect = { (float)GetScreenWidth() - 180, 10, 170, 30 };
            bool btnOpenHover = canInteract3D && CheckCollisionPointRec(mousePos, btnOpenRect);
            DrawRectangleRec(btnOpenRect, btnOpenHover ? LIGHTGRAY : GRAY);
            DrawRectangleLines((int)btnOpenRect.x, (int)btnOpenRect.y, (int)btnOpenRect.width, (int)btnOpenRect.height, BLACK);
            int openW = MeasureText("Part Controls", 15);
            DrawText("Part Controls", (int)(btnOpenRect.x + (btnOpenRect.width - openW) / 2), (int)btnOpenRect.y + 8, 15, BLACK);
            if (canInteract3D && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && btnOpenHover) {
                showControlWindow = !showControlWindow; showKeybindsWindow = false; showHelpWindow = false; showDropdownList = false;
            }

            Rectangle btnKeybindsRect = { (float)GetScreenWidth() - 180, 50, 170, 30 };
            bool btnKeybindsHover = canInteract3D && CheckCollisionPointRec(mousePos, btnKeybindsRect);
            DrawRectangleRec(btnKeybindsRect, btnKeybindsHover ? LIGHTGRAY : GRAY);
            DrawRectangleLines((int)btnKeybindsRect.x, (int)btnKeybindsRect.y, (int)btnKeybindsRect.width, (int)btnKeybindsRect.height, BLACK);
            int kbW = MeasureText("Keybinds Setup", 15);
            DrawText("Keybinds Setup", (int)(btnKeybindsRect.x + (btnKeybindsRect.width - kbW) / 2), (int)btnKeybindsRect.y + 8, 15, BLACK);
            if (canInteract3D && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && btnKeybindsHover) {
                showKeybindsWindow = !showKeybindsWindow; showControlWindow = false; showHelpWindow = false;
            }

            Rectangle btnProgSettingsRect = { (float)GetScreenWidth() - 180, 90, 170, 30 };
            bool btnProgSettingsHover = canInteract3D && CheckCollisionPointRec(mousePos, btnProgSettingsRect);
            DrawRectangleRec(btnProgSettingsRect, btnProgSettingsHover ? LIGHTGRAY : GRAY);
            DrawRectangleLines((int)btnProgSettingsRect.x, (int)btnProgSettingsRect.y, (int)btnProgSettingsRect.width, (int)btnProgSettingsRect.height, BLACK);
            int psW = MeasureText("Programm Settings", 15);
            DrawText("Programm Settings", (int)(btnProgSettingsRect.x + (btnProgSettingsRect.width - psW) / 2), (int)btnProgSettingsRect.y + 8, 15, BLACK);
            if (canInteract3D && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && btnProgSettingsHover) {
                isProgramSettingsMode = true; showControlWindow = false; showKeybindsWindow = false; showHelpWindow = false; showDropdownList = false;
            }

            // Toggle Joint Skeleton Overlay Button
            Rectangle btnJointsRect = { (float)GetScreenWidth() - 180, 130, 170, 26 };
            bool btnJointsHover = canInteract3D && CheckCollisionPointRec(mousePos, btnJointsRect);
            DrawRectangleRec(btnJointsRect, showJointPivots ? (btnJointsHover ? GOLD : ORANGE) : (btnJointsHover ? GRAY : DARKGRAY));
            DrawRectangleLines((int)btnJointsRect.x, (int)btnJointsRect.y, (int)btnJointsRect.width, (int)btnJointsRect.height, BLACK);
            const char* jLabel = showJointPivots ? "Skeleton Bones: ON" : "Skeleton Bones: OFF";
            int jW = MeasureText(jLabel, 13);
            DrawText(jLabel, (int)(btnJointsRect.x + (btnJointsRect.width - jW) / 2), (int)btnJointsRect.y + 6, 13, showJointPivots ? BLACK : RAYWHITE);
            if (canInteract3D && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && btnJointsHover) {
                showJointPivots = !showJointPivots;
            }

            // --- 5. HELP BUTTON ---
            Rectangle btnHelpRect = { (float)GetScreenWidth() - 180, 164, 170, 28 };
            bool btnHelpHover = canInteract3D && CheckCollisionPointRec(mousePos, btnHelpRect);
            DrawRectangleRec(btnHelpRect, showHelpWindow ? SKYBLUE : (btnHelpHover ? LIGHTGRAY : BLUE));
            DrawRectangleLines((int)btnHelpRect.x, (int)btnHelpRect.y, (int)btnHelpRect.width, (int)btnHelpRect.height, BLACK);
            int helpTxtW = MeasureText("Help [F1]", 14);
            DrawText("Help [F1]", (int)(btnHelpRect.x + (btnHelpRect.width - helpTxtW) / 2), (int)btnHelpRect.y + 7, 14, showHelpWindow ? BLACK : RAYWHITE);
            if (canInteract3D && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && btnHelpHover) {
                showHelpWindow = !showHelpWindow;
                if (showHelpWindow) {
                    showControlWindow = false;
                    showKeybindsWindow = false;
                    showDropdownList = false;
                }
            }

            // --- 6. MOTION RECORDER TAB BUTTON ---
            Rectangle btnRecTabRect = { (float)GetScreenWidth() - 180, 200, 170, 28 };
            bool btnRecTabHover = canInteract3D && CheckCollisionPointRec(mousePos, btnRecTabRect);
            DrawRectangleRec(btnRecTabRect, btnRecTabHover ? RED : MAROON);
            DrawRectangleLines((int)btnRecTabRect.x, (int)btnRecTabRect.y, (int)btnRecTabRect.width, (int)btnRecTabRect.height, RAYWHITE);
            int recTabW = MeasureText("Motion Recorder", 14);
            DrawText("Motion Recorder", (int)(btnRecTabRect.x + (btnRecTabRect.width - recTabW) / 2), (int)btnRecTabRect.y + 7, 14, RAYWHITE);
            if (canInteract3D && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && btnRecTabHover) {
                isRecorderMode = true;
                showControlWindow = false;
                showKeybindsWindow = false;
                showHelpWindow = false;
                showDropdownList = false;
            }

            // Part Controls Window
            if (showControlWindow) {
                Rectangle winRect = { (float)GetScreenWidth() - 320, 238, 300, 390 };
                DrawRectangleRec(winRect, LIGHTGRAY);
                DrawRectangleLines((int)winRect.x, (int)winRect.y, (int)winRect.width, (int)winRect.height, BLACK);
                
                DrawRectangle((int)winRect.x, (int)winRect.y, (int)winRect.width, 30, DARKGRAY);
                DrawText("Select & Control Part", (int)winRect.x + 10, (int)winRect.y + 8, 15, RAYWHITE);

                Rectangle btnCloseRect = { winRect.x + winRect.width - 25, winRect.y + 5, 20, 20 };
                bool btnCloseHover = canInteract3D && CheckCollisionPointRec(mousePos, btnCloseRect);
                DrawRectangleRec(btnCloseRect, btnCloseHover ? RED : DARKGRAY);
                DrawText("X", (int)btnCloseRect.x + 6, (int)btnCloseRect.y + 3, 14, RAYWHITE);
                if (canInteract3D && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && btnCloseHover) {
                    showControlWindow = false; showDropdownList = false;
                }

                Rectangle dropDownRect = { winRect.x + 15, winRect.y + 45, 270, 30 };
                bool dropHover = canInteract3D && CheckCollisionPointRec(mousePos, dropDownRect);
                DrawRectangleRec(dropDownRect, dropHover ? GRAY : WHITE);
                DrawRectangleLines((int)dropDownRect.x, (int)dropDownRect.y, (int)dropDownRect.width, (int)dropDownRect.height, BLACK);
                const char* currentPartLabel = (selectedPart != -1) ? partNames[selectedPart] : "None (Click 3D)";
                DrawText(TextFormat("Part: %s [v]", currentPartLabel), (int)dropDownRect.x + 10, (int)dropDownRect.y + 8, 13, BLACK);
                if (canInteract3D && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && dropHover) {
                    showDropdownList = true;
                    canInteract3D = false;
                }

                bool hasActivePart = (selectedPart != -1);

                auto DrawHoldButton = [&](Rectangle rect, const char* text) {
                    bool hover = hasActivePart && canInteract3D && CheckCollisionPointRec(mousePos, rect);
                    bool down = hover && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
                    DrawRectangleRec(rect, !hasActivePart ? (Color){ 185, 185, 185, 255 } : (down ? DARKGRAY : (hover ? GRAY : LIGHTGRAY)));
                    DrawRectangleLines((int)rect.x, (int)rect.y, (int)rect.width, (int)rect.height, hasActivePart ? BLACK : GRAY);
                    int tw = MeasureText(text, 12);
                    DrawText(text, (int)(rect.x + (rect.width - tw) / 2), (int)rect.y + 9, 12, hasActivePart ? BLACK : DARKGRAY);
                    return down;
                };

                auto DrawPressButton = [&](Rectangle rect, const char* text, bool allowWithoutPart = false) {
                    bool enabled = (hasActivePart || allowWithoutPart);
                    bool hover = enabled && canInteract3D && CheckCollisionPointRec(mousePos, rect);
                    DrawRectangleRec(rect, !enabled ? GRAY : (hover ? GRAY : DARKGRAY));
                    DrawRectangleLines((int)rect.x, (int)rect.y, (int)rect.width, (int)rect.height, BLACK);
                    int tw = MeasureText(text, 12);
                    DrawText(text, (int)(rect.x + (rect.width - tw) / 2), (int)rect.y + 11, 12, RAYWHITE);
                    return (enabled && canInteract3D && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hover);
                };

                float moveStep = 0.2f;
                Rectangle btnUp = { winRect.x + 125, winRect.y + 90, 50, 30 };
                Rectangle btnDown = { winRect.x + 125, winRect.y + 160, 50, 30 };
                Rectangle btnLeft = { winRect.x + 20, winRect.y + 125, 65, 30 };
                Rectangle btnRight = { winRect.x + 215, winRect.y + 125, 65, 30 };
                Rectangle btnBack = { winRect.x + 90, winRect.y + 125, 30, 30 };
                Rectangle btnFwd  = { winRect.x + 180, winRect.y + 125, 30, 30 };

                if (DrawHoldButton(btnUp, "UP"))       parts[selectedPart].pos.y += moveStep;
                if (DrawHoldButton(btnDown, "DOWN"))   parts[selectedPart].pos.y -= moveStep;
                if (DrawHoldButton(btnLeft, "LEFT"))   parts[selectedPart].pos.x -= moveStep;
                if (DrawHoldButton(btnRight, "RIGHT")) parts[selectedPart].pos.x += moveStep;
                if (DrawHoldButton(btnBack, "BK"))     parts[selectedPart].pos.z += moveStep;
                if (DrawHoldButton(btnFwd, "FW"))      parts[selectedPart].pos.z -= moveStep;

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
                if (DrawPressButton(btnReset, "Reset Default", false) && selectedPart != -1) {
                    parts[selectedPart].pos = parts[selectedPart].defaultPos;
                    parts[selectedPart].rotX = parts[selectedPart].defaultRotX;
                    parts[selectedPart].rotY = parts[selectedPart].defaultRotY;
                    parts[selectedPart].rotZ = parts[selectedPart].defaultRotZ;
                    parts[selectedPart].scale = parts[selectedPart].defaultScale;
                    parts[selectedPart].sensorRotX = 0.0f;
                    parts[selectedPart].sensorRotY = 0.0f;
                    parts[selectedPart].sensorRotZ = 0.0f;
                }

                Rectangle btnSave = { winRect.x + 160, winRect.y + 295, 120, 35 };
                if (DrawPressButton(btnSave, "Save Settings", true)) {
                    std::ofstream outFile("exoskeleton_config.txt");
                    if (outFile.is_open()) {
                        for (int i = 0; i < totalParts; i++) {
                            Matrix jMat = GetPartJointMatrix(i, parts, parentIndex);
                            outFile << jMat.m12 << " " << jMat.m13 << " " << jMat.m14 << " " 
                                    << parts[i].rotX << " " << parts[i].rotY << " " << parts[i].rotZ << " " << parts[i].scale << "\n";
                        }
                        outFile.close();
                    }
                }

                DrawText("Config saved to exoskeleton_config.txt", (int)winRect.x + 35, (int)winRect.y + 345, 12, DARKGRAY);

                // Render dropdown overlay on top of Part Controls buttons
                if (showDropdownList) {
                    Rectangle fullListRect = { winRect.x + 15, winRect.y + 75, 270, 20.0f * totalParts };
                    DrawRectangleRec(fullListRect, WHITE);

                    for (int row = 0; row < totalParts; row++) {
                        int partIdx = dropdownOrder[row];
                        Rectangle itemRect = { winRect.x + 15, winRect.y + 75 + (float)(row * 20), 270, 20 };
                        bool itemHover = CheckCollisionPointRec(mousePos, itemRect);
                        Color rowBg = (partIdx == selectedPart) ? SKYBLUE : (itemHover ? LIGHTGRAY : WHITE);

                        DrawRectangleRec(itemRect, rowBg);
                        DrawRectangleLines((int)itemRect.x, (int)itemRect.y, (int)itemRect.width, (int)itemRect.height, BLACK);
                        DrawText(TextFormat("%i. %s", row + 1, partNames[partIdx]), (int)itemRect.x + 6, (int)itemRect.y + 3, 13, BLACK);
                    }
                }
            }

            // Keybinds Setup Window
            if (showKeybindsWindow) {
                Rectangle winKeyRect = { (float)GetScreenWidth() - 320, 238, 300, 310 };
                DrawRectangleRec(winKeyRect, LIGHTGRAY);
                DrawRectangleLines((int)winKeyRect.x, (int)winKeyRect.y, (int)winKeyRect.width, (int)winKeyRect.height, BLACK);
                
                DrawRectangle((int)winKeyRect.x, (int)winKeyRect.y, (int)winKeyRect.width, 30, DARKGRAY);
                DrawText("Keybinds Configuration", (int)winKeyRect.x + 10, (int)winKeyRect.y + 8, 15, RAYWHITE);

                Rectangle btnCloseKey = { winKeyRect.x + winKeyRect.width - 25, winKeyRect.y + 5, 20, 20 };
                bool btnCloseKeyHover = canInteract3D && CheckCollisionPointRec(mousePos, btnCloseKey);
                DrawRectangleRec(btnCloseKey, btnCloseKeyHover ? RED : DARKGRAY);
                DrawText("X", (int)btnCloseKey.x + 6, (int)btnCloseKey.y + 3, 14, RAYWHITE);
                if (canInteract3D && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && btnCloseKeyHover) {
                    showKeybindsWindow = false; waitingForKeyIndex = -1;
                }

                const char* bindLabels[6] = { "Forward", "Backward", "Move Left", "Move Right", "Move Up", "Move Down" };
                int* bindKeys[6] = { &keyForward, &keyBackward, &keyLeft, &keyRight, &keyUpCam, &keyDownCam };

                for (int i = 0; i < 6; i++) {
                    float yPos = winKeyRect.y + 45 + (float)(i * 38);
                    DrawText(bindLabels[i], (int)winKeyRect.x + 15, (int)yPos + 8, 14, BLACK);

                    Rectangle btnBindRect = { winKeyRect.x + 160, yPos, 125, 30 };
                    bool btnBindHover = canInteract3D && CheckCollisionPointRec(mousePos, btnBindRect);
                    bool isWaiting = (waitingForKeyIndex == i);
                    Color btnColor = isWaiting ? YELLOW : (btnBindHover ? GRAY : WHITE);
                    
                    DrawRectangleRec(btnBindRect, btnColor);
                    DrawRectangleLines((int)btnBindRect.x, (int)btnBindRect.y, (int)btnBindRect.width, (int)btnBindRect.height, BLACK);
                    const char* keyName = isWaiting ? "Press Key..." : TextFormat("Key: %i", *bindKeys[i]);
                    DrawText(keyName, (int)btnBindRect.x + 10, (int)btnBindRect.y + 8, 13, BLACK);

                    if (canInteract3D && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && btnBindHover) waitingForKeyIndex = i;
                }
            }

            Color infoColor = (selectedTheme == 3) ? DARKBLUE : YELLOW;
            Color valColor  = (selectedTheme == 3) ? DARKGREEN : GREEN;

            if (selectedPart != -1) {
                int pIdx = parentIndex[selectedPart];
                const char* parentName = (pIdx == -1) ? "None (World Root)" : partNames[pIdx];

                DrawText(TextFormat("Selected: %s (click again to deselect)", partNames[selectedPart]), 10, 115, 16, infoColor);
                if (hoveredPart3D != -1) {
                    const char* hoverAction = (hoveredPart3D == selectedPart) ? "LMB to deselect" : "LMB to select";
                    DrawText(TextFormat("Hovering: %s (%s)", partNames[hoveredPart3D], hoverAction), 10, 137, 15, GOLD);
                } else {
                    DrawText(TextFormat("Parent Joint: %s", parentName), 10, 137, 15, hudSubColor);
                }
                DrawText(TextFormat("Local Offset: X=%.1f Y=%.1f Z=%.1f", parts[selectedPart].pos.x, parts[selectedPart].pos.y, parts[selectedPart].pos.z), 10, 159, 16, valColor);
                DrawText(TextFormat("Mount Rot: X=%.1f Y=%.1f Z=%.1f", parts[selectedPart].rotX, parts[selectedPart].rotY, parts[selectedPart].rotZ), 10, 181, 16, valColor);
                DrawText(TextFormat("Sensor Rot: X=%.1f Y=%.1f Z=%.1f", parts[selectedPart].sensorRotX, parts[selectedPart].sensorRotY, parts[selectedPart].sensorRotZ), 10, 203, 16, valColor);
            } else {
                DrawText("Selected: None", 10, 115, 16, hudSubColor);
                if (hoveredPart3D != -1) {
                    DrawText(TextFormat("Hovering: %s (LMB to select)", partNames[hoveredPart3D]), 10, 137, 15, GOLD);
                } else {
                    DrawText("Click any 3D segment to inspect or edit", 10, 137, 15, hudSubColor);
                }
            }

            // ================= HELP MODAL WINDOW OVERLAY =================
            if (showHelpWindow) {
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){ 0, 0, 0, 150 });

                float modalW = 760.0f;
                float modalH = 520.0f;
                float modalX = (GetScreenWidth() - modalW) / 2.0f;
                float modalY = (GetScreenHeight() - modalH) / 2.0f;

                DrawRectangle((int)modalX, (int)modalY, (int)modalW, (int)modalH, (Color){ 32, 36, 42, 255 });
                DrawRectangleLines((int)modalX, (int)modalY, (int)modalW, (int)modalH, SKYBLUE);

                DrawRectangle((int)modalX, (int)modalY, (int)modalW, 38, (Color){ 20, 60, 100, 255 });
                DrawText("HS-1 EXOSKELETON - USER GUIDE & SYSTEM OVERVIEW", (int)modalX + 16, (int)modalY + 10, 18, RAYWHITE);

                Rectangle btnCloseHelp = { modalX + modalW - 34, modalY + 6, 26, 26 };
                bool closeHelpHover = CheckCollisionPointRec(mousePos, btnCloseHelp);
                DrawRectangleRec(btnCloseHelp, closeHelpHover ? RED : MAROON);
                DrawRectangleLines((int)btnCloseHelp.x, (int)btnCloseHelp.y, (int)btnCloseHelp.width, (int)btnCloseHelp.height, RAYWHITE);
                DrawText("X", (int)btnCloseHelp.x + 8, (int)btnCloseHelp.y + 5, 15, RAYWHITE);
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && closeHelpHover) {
                    showHelpWindow = false;
                }

                float tx = modalX + 22.0f;
                float ty = modalY + 52.0f;

                DrawText("1. ABOUT HS-1 FULL-BODY TELEMETRY CONFIGURATOR", (int)tx, (int)ty, 16, SKYBLUE);
                ty += 22.0f;
                DrawText("- Real-time 3D kinematic visualizer for the 13-node HS-1 motion tracking suit.", (int)tx + 10, (int)ty, 14, RAYWHITE);
                ty += 19.0f;
                DrawText("- Hardware nodes: ESP32-C3 SuperMini + BNO055 9-DOF IMU sensors over UART.", (int)tx + 10, (int)ty, 14, LIGHTGRAY);
                ty += 19.0f;
                DrawText("- Hierarchical Skeleton Tree: Limb rotations propagate naturally from Body ->", (int)tx + 10, (int)ty, 14, LIGHTGRAY);
                ty += 17.0f;
                DrawText("  Biceps -> Forearm -> Hand and Body -> Upper Leg -> Lower Leg -> Foot.", (int)tx + 10, (int)ty, 14, LIGHTGRAY);

                ty += 26.0f;
                DrawText("2. 3D CAMERA & KEYBOARD SHORTCUTS", (int)tx, (int)ty, 16, GOLD);
                ty += 22.0f;
                DrawText("- Left Mouse Button (LMB) on 3D Model: Click to select, click again to deselect", (int)tx + 10, (int)ty, 14, RAYWHITE);
                ty += 19.0f;
                DrawText("- Hold Right Mouse Button (RMB) + Move Mouse: Rotate 3D camera freely", (int)tx + 10, (int)ty, 14, RAYWHITE);
                ty += 19.0f;
                DrawText("- Arrow Keys (or custom binds): Move camera Forward / Back / Left / Right", (int)tx + 10, (int)ty, 14, RAYWHITE);
                ty += 19.0f;
                DrawText("- Left Shift / Left Ctrl: Elevate camera Up / Down", (int)tx + 10, (int)ty, 14, RAYWHITE);
                ty += 19.0f;
                DrawText("- R Key: Reset 3D camera to default front view | '+' / '-': Scale part", (int)tx + 10, (int)ty, 14, RAYWHITE);
                ty += 19.0f;
                DrawText("- F1 / ESC Key: Toggle Help window / unlock mouse cursor", (int)tx + 10, (int)ty, 14, RAYWHITE);

                ty += 26.0f;
                DrawText("3. MENUS & WORKFLOW", (int)tx, (int)ty, 16, LIME);
                ty += 22.0f;
                DrawText("- Part Controls: Adjust local joint offsets & mounting angles, then Save.", (int)tx + 10, (int)ty, 14, RAYWHITE);
                ty += 19.0f;
                DrawText("- Programm Settings: Set COM Port, Baud Rate, Resolution, FPS, Theme, Demo.", (int)tx + 10, (int)ty, 14, RAYWHITE);
                ty += 19.0f;
                DrawText("- Motion Recorder: Record joint trajectories (REC / STOP / PLAY) & Export CSV.", (int)tx + 10, (int)ty, 14, RAYWHITE);
                ty += 19.0f;
                DrawText("- Open 2D Body Map: View connection status (Green = Online, White = Offline).", (int)tx + 10, (int)ty, 14, RAYWHITE);

                Rectangle btnGotIt = { modalX + (modalW - 160.0f) / 2.0f, modalY + modalH - 46.0f, 160.0f, 32.0f };
                bool gotItHover = CheckCollisionPointRec(mousePos, btnGotIt);
                DrawRectangleRec(btnGotIt, gotItHover ? SKYBLUE : BLUE);
                DrawRectangleLines((int)btnGotIt.x, (int)btnGotIt.y, (int)btnGotIt.width, (int)btnGotIt.height, RAYWHITE);
                int gotItW = MeasureText("Close Help", 15);
                DrawText("Close Help", (int)(btnGotIt.x + (btnGotIt.width - gotItW) / 2.0f), (int)btnGotIt.y + 8, 15, RAYWHITE);
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && gotItHover) {
                    showHelpWindow = false;
                }
            }

        } else {
            // ================= 2D FULLSCREEN BODY MAP MODE =================
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), BLACK);

            DrawText("HS-1 EXOSKELETON - 2D BODY CONNECTION STATUS", 50, 30, 24, RAYWHITE);
            DrawText("Green = Connected to Real Block | White = Disconnected", 50, 60, 15, LIGHTGRAY);

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
                
                int textWidth = MeasureText(layout[i].label, 13);
                DrawText(layout[i].label, (int)(layout[i].rect.x + (layout[i].rect.width - textWidth) / 2), (int)(layout[i].rect.y + 16), 13, textColor);
            }

            // --- BACK TO 3D BUTTON ---
            Rectangle btnBackTo3D = { cx - 110, (float)GetScreenHeight() - 60, 220, 40 };
            bool btnBackHover = CheckCollisionPointRec(mousePos, btnBackTo3D);
            DrawRectangleRec(btnBackTo3D, btnBackHover ? ORANGE : DARKGRAY);
            DrawRectangleLines((int)btnBackTo3D.x, (int)btnBackTo3D.y, (int)btnBackTo3D.width, (int)btnBackTo3D.height, RAYWHITE);
            
            int backTextWidth = MeasureText("Back to 3D", 15);
            DrawText("Back to 3D", (int)(btnBackTo3D.x + (btnBackTo3D.width - backTextWidth) / 2), (int)btnBackTo3D.y + 12, 15, RAYWHITE);

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && btnBackHover) {
                is2DMode = false;
            }
        }

        EndDrawing();
    }

    #undef DrawText
    #undef MeasureText

    if (uiFont.texture.id != GetFontDefault().texture.id) {
        UnloadFont(uiFont);
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