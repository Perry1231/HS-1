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

void ForceConvertSTL(const fs::path& filePath) {
    std::ifstream inFile(filePath, std::ios::binary);
    if (!inFile.is_open()) return;

    std::string content((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    inFile.close();

    std::stringstream ss(content);
    std::string line;
    std::vector<Triangle> triangles;
    Triangle currentTriangle = {};
    std::vector<Vector3D> currentVertices;

    while (std::getline(ss, line)) {
        std::stringstream lineStream(line);
        std::string token;
        lineStream >> token;

        if (token == "facet") {
            lineStream >> token; // normal
            lineStream >> currentTriangle.normal.x >> currentTriangle.normal.y >> currentTriangle.normal.z;
        } else if (token == "vertex") {
            Vector3D v;
            lineStream >> v.x >> v.y >> v.z;
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

    if (triangles.empty()) {
        std::cout << "[WARN] " << filePath.filename().string() << " could not be parsed as ASCII text. (Might be corrupted binary).\n";
        return;
    }

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
            ForceConvertSTL(entry.path());
        }
    }
    return 0;
}