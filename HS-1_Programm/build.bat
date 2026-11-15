@echo off
echo Compiling HS1_App.exe (Portable Static Build)...
g++ main.cpp -o HS1_App.exe -O2 -Wall -std=c++17 -static -static-libgcc -static-libstdc++ -lraylib -lopengl32 -lgdi32 -lwinmm
if %errorlevel% neq 0 (
    echo [ERROR] Compilation failed!
    pause
    exit /b %errorlevel%
)
echo [SUCCESS] HS1_App.exe is ready!
pause