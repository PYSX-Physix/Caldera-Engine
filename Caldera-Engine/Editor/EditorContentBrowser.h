#pragma once

#include "../Rendering/Renderer.h"
#include <filesystem>
#include <string>
#include <unordered_map>
#include <d3d12.h>
#include <wrl/client.h>
#include "imgui.h"

using namespace Microsoft::WRL;

class Renderer;

class EditorContentBrowser {
public:
    void SetRootDirectory(const std::filesystem::path& root);
    void Render(bool* isOpen);
    void SetRenderer(Renderer* r);

    bool canOpen = true;

private:
    std::filesystem::path rootPath;
    std::filesystem::path currentDirectory;
    std::filesystem::path selectedFile;
    bool isGridView = true;

    std::unordered_map<std::string, ComPtr<ID3D12Resource>> previewTextures;
    std::unordered_map<std::string, ImTextureID> previewTextureIDs;
    std::unordered_map<std::string, ImTextureID> fileTypeIcons;

    Renderer* renderer = nullptr;

    void RenderNavigationBar();
    void DrawDirectoryTree(const std::filesystem::path& path);
    void DrawAssetList(const std::filesystem::path& path);
    void DrawPreviewPanel();
    void DrawStatusBar();
    void DrawContextMenu(const std::filesystem::path& path);

    ImTextureID GetFileIcon(const std::filesystem::path& path);
    ImTextureID LoadPreviewTexture(const std::filesystem::path& filePath);
    ImTextureID CreateTextureFromFile(
        const std::filesystem::path& path,
        ID3D12Device* device,
        ID3D12DescriptorHeap* srvHeap,
        ComPtr<ID3D12Resource>& outTexture
    );
};