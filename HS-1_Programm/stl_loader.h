#pragma once
#include "raylib.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <cstdint>

#pragma pack(push, 1)
struct STLTriangle {
    float normal[3];
    float v1[3];
    float v2[3];
    float v3[3];
    uint16_t attribute;
};
#pragma pack(pop)

inline Model LoadBinarySTL(const char* filename) {
    Model model = { 0 };
    std::ifstream file(filename, std::ios::binary);

    if (!file.is_open()) {
        std::cout << "[STL Loader] Error: Cannot open " << filename << std::endl;
        return model;
    }

    // Пропускаємо 80 байт заголовка
    file.seekg(80, std::ios::beg);

    uint32_t triangleCount = 0;
    file.read(reinterpret_cast<char*>(&triangleCount), sizeof(triangleCount));

    if (triangleCount == 0) {
        file.close();
        return model;
    }

    std::vector<STLTriangle> triangles(triangleCount);
    file.read(reinterpret_cast<char*>(triangles.data()), triangleCount * sizeof(STLTriangle));
    file.close();

    // Створюємо Mesh для Raylib
    Mesh mesh = { 0 };
    mesh.triangleCount = triangleCount;
    mesh.vertexCount = triangleCount * 3;

    mesh.vertices = (float*)RL_MALLOC(mesh.vertexCount * 3 * sizeof(float));
    mesh.normals = (float*)RL_MALLOC(mesh.vertexCount * 3 * sizeof(float));

    for (uint32_t i = 0; i < triangleCount; i++) {
        const auto& tri = triangles[i];

        // Вершини
        mesh.vertices[i * 9 + 0] = tri.v1[0];
        mesh.vertices[i * 9 + 1] = tri.v1[1];
        mesh.vertices[i * 9 + 2] = tri.v1[2];

        mesh.vertices[i * 9 + 3] = tri.v2[0];
        mesh.vertices[i * 9 + 4] = tri.v2[1];
        mesh.vertices[i * 9 + 5] = tri.v2[2];

        mesh.vertices[i * 9 + 6] = tri.v3[0];
        mesh.vertices[i * 9 + 7] = tri.v3[1];
        mesh.vertices[i * 9 + 8] = tri.v3[2];

        // Нормалі
        for (int v = 0; v < 3; v++) {
            mesh.normals[i * 9 + v * 3 + 0] = tri.normal[0];
            mesh.normals[i * 9 + v * 3 + 1] = tri.normal[1];
            mesh.normals[i * 9 + v * 3 + 2] = tri.normal[2];
        }
    }

    // Завантажуємо геометрію в VRAM (GPU)
    UploadMesh(&mesh, false);

    model = LoadModelFromMesh(mesh);
    std::cout << "[STL Loader] Successfully loaded " << filename << " (" << triangleCount << " triangles)\n";
    return model;
}