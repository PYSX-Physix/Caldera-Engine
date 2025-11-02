#include "AssetManager.h"
#include <iostream>

bool AssetManager::LoadModel(const std::string& path) {
    // Check if already loaded
    if (loadedMeshes.find(path) != loadedMeshes.end()) {
        std::cout << "Model already loaded: " << path << std::endl;
        return true;
    }

    // TODO: Implement actual model loading (e.g., using Assimp)
    Mesh mesh;
    // Load mesh data here...

    loadedMeshes[path] = std::move(mesh);
    std::cout << "Loaded model: " << path << std::endl;
    return true;
}

bool AssetManager::LoadTexture(const std::string& path) {
    // Check if already loaded
    if (loadedTextures.find(path) != loadedTextures.end()) {
        std::cout << "Texture already loaded: " << path << std::endl;
        return true;
    }

    // TODO: Implement actual texture loading
    Texture texture;
    // Load texture data here...

    loadedTextures[path] = std::move(texture);
    std::cout << "Loaded texture: " << path << std::endl;
    return true;
}

void AssetManager::Shutdown() {
    loadedMeshes.clear();
    loadedTextures.clear();
    std::cout << "AssetManager shutdown complete" << std::endl;
}