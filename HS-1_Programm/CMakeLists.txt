cmake_minimum_required(VERSION 3.14)
project(HS1_App CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 1. Завантаження залежностей через FetchContent
include(FetchContent)

FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.0.tar.gz
)
FetchContent_MakeAvailable(raylib)

# ---------------------------------------------------------------------
# 2. СПОЧАТКУ СТВОРЮЄМО TARGET (виконуваний файл)
# ---------------------------------------------------------------------
add_executable(HS1_App
    app.cpp
    serial_reader.cpp
)

# ---------------------------------------------------------------------
# 3. ТІЛЬКИ ПІСЛЯ ЦЬОГО ПІДКТЮЧАЄМО БІБЛІОТЕКИ ТА ДИРЕКТОРІЇ
# ---------------------------------------------------------------------
target_include_directories(HS1_App PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(HS1_App PRIVATE 
    raylib 
    nlohmann_json::nlohmann_json
)

if(WIN32)
    target_link_libraries(HS1_App PRIVATE winmm setupapi)
endif()