cmake_minimum_required(VERSION 3.11)
project(HS1_Visualizer CXX)

set(CMAKE_CXX_STANDARD 17)

# --- 1. AUTOMATIC PYTHON SCRIPT EXECUTION ---
# Automatically runs your model fixing script before building the project
execute_process(
    COMMAND python fix_models.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE PYTHON_RESULT
)

if(NOT PYTHON_RESULT EQUAL 0)
    message(STATUS "Python script not executed (this is normal if the script name differs or isn't needed).")
endif()


# --- 2. RAYLIB INTEGRATION ---
# Automatically downloads and builds Raylib from GitHub
include(FetchContent)
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.0.zip
)
FetchContent_MakeAvailable(raylib)


# --- 3. EXECUTABLE TARGET ---
add_executable(HS1_Visualizer main.cpp)


# --- 4. INCLUDE DIRECTORIES & LIBRARIES ---
# Include current directory so the compiler can find json.hpp
target_include_directories(HS1_Visualizer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Link Raylib to the project
target_link_libraries(HS1_Visualizer PRIVATE raylib)


# --- 5. SYSTEM LIBRARIES (FOR WINDOWS) ---
if(WIN32)
    # Required for Raylib timers/audio and system calls under Windows
    target_link_libraries(HS1_Visualizer PRIVATE winmm m)
endif()