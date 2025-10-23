#pragma once

#include <iostream>
#include <unordered_map>
#include "Mesh.h"
#include "Texture.h"

class AssetManager {
public:
    bool LoadModel(const std::string& path);
    bool LoadTexture(const std::string& path);
    void Shutdown();

private:
    std::unordered_map<std::string, Mesh> loadedMeshes;
    std::unordered_map<std::string, Texture> loadedTextures;
};
