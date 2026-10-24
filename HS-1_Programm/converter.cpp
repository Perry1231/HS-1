#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <cstdint>
#include <filesystem>

namespace fs = std::filesystem;

struct Vector3D { float x, y, z; };

struct Triangle {
    Vector3D normal;
    Vector3D v1, v2, v3;
};

void ConvertSTLToBinary(const fs::path& filePath) {
    std::ifstream inFile(filePath, std::ios::binary);
    if (!inFile.is_open()) return;

    std::string line;
    std::getline(inFile, line);

    // Якщо файл не починається з solid, він або вже бінарний, або порожній
    if (line.rfind("solid", 0) != 0) {
        std::cout << "[SKIP] File " << filePath.filename().string() << " is already Binary or custom.\n";
        return;
    }

    inFile.seekg(0, std::ios::beg);
    std::vector<Triangle> triangles;
    Triangle currentTriangle;
    std::vector<Vector3D> currentVertices;

    while (std::getline(inFile, line)) {
        std::stringstream ss(line);
        std::string token;
        ss >> token;

        if (token == "facet") {
            ss >> token; // normal
            ss >> currentTriangle.normal.x >> currentTriangle.normal.y >> currentTriangle.normal.z;
        } else if (token == "vertex") {
            Vector3D v;
            ss >> v.x >> v.y >> v.z;
            currentVertices.push_back(v);
            if (currentVertices.size() == 3) {
                currentTriangle.v1 = currentVertices[0];
                currentTriangle.v2 = currentVertices[1];
                currentTriangle.v3 = currentVertices[2];
                triangles.push_back(currentTriangle);
                currentVertices.clear();
            }
        }
    }
    inFile.close();

    if (triangles.empty()) {
        std::cout << "[INFO] No ASCII triangles parsed in " << filePath.filename().string() << "\n";
        return;
    }

    // Записуємо чистий Binary STL
    std::ofstream outFile(filePath, std::ios::binary | std::ios::trunc);
    uint8_t header[80] = {0};
    outFile.write(reinterpret_cast<char*>(header), 80);

    uint32_t count = static_cast<uint32_t>(triangles.size());
    outFile.write(reinterpret_cast<char*>(&count), sizeof(count));

    uint16_t attrib = 0;
    for (const auto& tri : triangles) {
        outFile.write(reinterpret_cast<const char*>(&tri.normal), sizeof(Vector3D));
        outFile.write(reinterpret_cast<const char*>(&tri.v1), sizeof(Vector3D));
        outFile.write(reinterpret_cast<const char*>(&tri.v2), sizeof(Vector3D));
        outFile.write(reinterpret_cast<const char*>(&tri.v3), sizeof(Vector3D));
        outFile.write(reinterpret_cast<const char*>(&attrib), sizeof(attrib));
    }
    outFile.close();

    std::cout << "[SUCCESS] Converted " << filePath.filename().string() << " (" << count << " triangles)\n";
}

int main() {
    fs::path modelsDir = "Models";
    if (!fs::exists(modelsDir)) {
        std::cout << "Folder Models/ not found!\n";
        return 1;
    }

    for (const auto& entry : fs::directory_iterator(modelsDir)) {
        if (entry.path().extension() == ".stl") {
            ConvertSTLToBinary(entry.path());
        }
    }
    return 0;
}