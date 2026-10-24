cmake_minimum_required(VERSION 3.14...3.30)

# Глобальний фікс для CMake 4.4+ та Raylib
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

project(HS1_Project CXX)

set(CMAKE_CXX_STANDARD 17)

# --- 1. RAYLIB INTEGRATION ---
include(FetchContent)
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(raylib)

# --- 2. EXECUTABLE TARGET ---
add_executable(HS1_App app.cpp)

# --- 3. INCLUDE DIRECTORIES & LIBRARIES ---
target_include_directories(HS1_App PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(HS1_App PRIVATE raylib)

# --- 4. SYSTEM LIBRARIES (FOR WINDOWS) ---
if(WIN32)
    target_link_libraries(HS1_App PRIVATE winmm m)
endif()