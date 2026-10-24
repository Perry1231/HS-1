cmake_minimum_required(VERSION 3.14...3.30)

# 1. Пригнічуємо політики FetchContent
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

project(HS1_App CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# 2. Завантаження JSON
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(json)

# 3. Завантаження Raylib
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(raylib)

# 4. Основний проєкт
add_executable(HS1_App
    app.cpp
    serial_reader.cpp
)

target_include_directories(HS1_App PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(HS1_App PRIVATE 
    raylib 
    nlohmann_json::nlohmann_json
)

if(WIN32)
    target_link_libraries(HS1_App PRIVATE winmm setupapi)
endif()