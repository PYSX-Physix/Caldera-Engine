#include "Renderer.h"
#include "EditorContentBrowser.h"
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "d3dx12.h"
#include "imgui.h"

void EditorContentBrowser::SetRootDirectory(const std::filesystem::path& root) {
    rootPath = root;
    currentDirectory = root;
}

void EditorContentBrowser::SetRenderer(Renderer* r) {
    renderer = r;
}

void EditorContentBrowser::Render(bool* isOpen) {
    if (!isOpen || !*isOpen) return;

    ImGui::Begin("Content Browser", isOpen, ImGuiWindowFlags_NoScrollbar);

    // Top bar with navigation
    RenderNavigationBar();

    // Split view: Directory tree on left, content on right
    const float treeViewWidth = 200.0f;
    ImGui::BeginChild("DirectoryTree", ImVec2(treeViewWidth, 0), true);
    DrawDirectoryTree(rootPath);
    ImGui::EndChild();

    ImGui::SameLine();

    // Main content area with grid/list view
    ImGui::BeginChild("ContentArea", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())); // Leave space for status bar
    DrawAssetList(currentDirectory);
    ImGui::EndChild();

    // Status bar
    DrawStatusBar();

    // Preview panel (if asset is selected)
    if (!selectedFile.empty()) {
        ImGui::Begin("Asset Preview", nullptr, ImGuiWindowFlags_NoCollapse);
        DrawPreviewPanel();
        ImGui::End();
    }

    ImGui::End();
}

void EditorContentBrowser::RenderNavigationBar() {
    // Path breadcrumbs
    ImGui::BeginGroup();
    std::filesystem::path currentPath = currentDirectory;
    std::vector<std::filesystem::path> pathParts;

    while (currentPath != rootPath.parent_path()) {
        pathParts.push_back(currentPath);
        currentPath = currentPath.parent_path();
    }

    for (auto it = pathParts.rbegin(); it != pathParts.rend(); ++it) {
        if (it != pathParts.rbegin()) {
            ImGui::SameLine();
            ImGui::TextUnformatted(">");
            ImGui::SameLine();
        }
        if (ImGui::Button(it->filename().string().c_str())) {
            currentDirectory = *it;
        }
    }
    ImGui::EndGroup();

    // View options
    ImGui::SameLine(ImGui::GetWindowWidth() - 100);
    if (ImGui::Button(isGridView ? "Grid" : "List")) {
        isGridView = !isGridView;
    }
}

void EditorContentBrowser::DrawDirectoryTree(const std::filesystem::path& path) {
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_OpenOnDoubleClick |
        ImGuiTreeNodeFlags_SpanAvailWidth;

    if (path == currentDirectory) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    bool isDirectory = std::filesystem::is_directory(path);
    if (!isDirectory) return;

    std::string name = path.filename().string();
    if (name.empty()) name = path.string(); // For root

    bool opened = ImGui::TreeNodeEx(name.c_str(), flags);

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        currentDirectory = path;
    }

    if (opened) {
        try {
            for (const auto& entry : std::filesystem::directory_iterator(path)) {
                if (std::filesystem::is_directory(entry)) {
                    DrawDirectoryTree(entry.path());
                }
            }
        }
        catch (const std::filesystem::filesystem_error&) {}
        ImGui::TreePop();
    }
}

void EditorContentBrowser::DrawAssetList(const std::filesystem::path& path) {
    // Grid/List view settings
    const float padding = 8.0f;
    const float thumbnailSize = isGridView ? 96.0f : 32.0f;
    const float cellSize = thumbnailSize + padding;

    if (isGridView) {
        float panelWidth = ImGui::GetContentRegionAvail().x;
        int columnCount = static_cast<int>(panelWidth / cellSize);
        if (columnCount < 1) columnCount = 1;

        ImGui::Columns(columnCount, 0, false);
    }

    try {
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            const auto& entryPath = entry.path();
            std::string filename = entryPath.filename().string();
            ImTextureID icon = GetFileIcon(entryPath); // Implement based on file type

            if (isGridView) {
                // Grid item
                ImGui::BeginGroup();
                ImGui::Image(icon, ImVec2(thumbnailSize, thumbnailSize));
                ImGui::TextWrapped("%s", filename.c_str());
                ImGui::EndGroup();

                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                    if (entry.is_directory()) {
                        currentDirectory = entryPath;
                    }
                    else {
                        selectedFile = entryPath;
                    }
                }
                ImGui::NextColumn();
            }
            else {
                // List item
                ImGui::BeginGroup();
                ImGui::Image(icon, ImVec2(thumbnailSize, thumbnailSize));
                ImGui::SameLine();
                if (ImGui::Selectable(filename.c_str(), selectedFile == entryPath)) {
                    selectedFile = entryPath;
                }
                ImGui::EndGroup();
            }

            // Context menu
            if (ImGui::BeginPopupContextItem(filename.c_str())) {
                DrawContextMenu(entryPath);
                ImGui::EndPopup();
            }
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: %s", e.what());
    }

    if (isGridView) {
        ImGui::Columns(1);
    }
}

void EditorContentBrowser::DrawPreviewPanel()
{
}

void EditorContentBrowser::DrawStatusBar() {
    ImGui::Separator();
    if (!selectedFile.empty()) {
        auto fileSize = std::filesystem::file_size(selectedFile);
        ImGui::Text("Selected: %s (%.2f KB)",
            selectedFile.filename().string().c_str(),
            static_cast<float>(fileSize) / 1024.0f);
    }
}

void EditorContentBrowser::DrawContextMenu(const std::filesystem::path& path) {
    if (ImGui::MenuItem("Open")) {
        if (std::filesystem::is_directory(path)) {
            currentDirectory = path;
        }
        else {
            selectedFile = path;
        }
    }
    if (ImGui::MenuItem("Delete")) {
        // TODO: Add confirmation dialog
        try {
            std::filesystem::remove(path);
        }
        catch (const std::filesystem::filesystem_error&) {}
    }
    // Add more context menu items as needed
}

ImTextureID EditorContentBrowser::GetFileIcon(const std::filesystem::path& path)
{
    static bool initialized = false;
    static ImTextureID defaultFileIcon = (ImTextureID)0;
    static ImTextureID folderIcon = (ImTextureID)0;
    static std::unordered_map<std::string, ImTextureID> fileTypeIcons;

    // Initialize icons if not done yet
    if (!initialized && renderer) {
        // Load default icons
        defaultFileIcon = CreateTextureFromFile(
            rootPath / "Editor/Icons/file.png",
            renderer->GetDevice(),
            renderer->GetSrvHeap(),
            previewTextures["default_file"]
        );

        folderIcon = CreateTextureFromFile(
            rootPath / "Editor/Icons/folder.png",
            renderer->GetDevice(),
            renderer->GetSrvHeap(),
            previewTextures["folder"]
        );

        // Initialize file type specific icons
        struct IconMapping {
            std::vector<std::string> extensions;
            std::string iconPath;
        };

        std::vector<IconMapping> iconMappings = {
            // 3D asset files
            {{"fbx", "obj", "gltf", "glb"}, "Editor/Icons/model.png"},
            // Image files
            {{"png", "jpg", "jpeg", "tga", "bmp"}, "Editor/Icons/image.png"},
            // Shader files
            {{"hlsl", "glsl", "shader"}, "Editor/Icons/shader.png"},
            // Material files
            {{"mat", "material"}, "Editor/Icons/material.png"},
            // Scene files
            {{"scene"}, "Editor/Icons/scene.png"}
        };

        // Load all file type specific icons
        for (const auto& mapping : iconMappings) {
            ImTextureID typeIcon = CreateTextureFromFile(
                rootPath / mapping.iconPath,
                renderer->GetDevice(),
                renderer->GetSrvHeap(),
                previewTextures[mapping.iconPath]
            );

            // Associate icon with all specified extensions
            for (const auto& ext : mapping.extensions) {
                fileTypeIcons["." + ext] = typeIcon;
            }
        }

        initialized = true;
    }

    // Return appropriate icon based on file type
    if (std::filesystem::is_directory(path)) {
        return folderIcon ? folderIcon : (ImTextureID)0;
    }

    // Get file extension and convert to lowercase
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    // Look up icon for this file type
    auto it = fileTypeIcons.find(extension);
    if (it != fileTypeIcons.end() && it->second != (ImTextureID)0) {
        return it->second;
    }

    // Return default file icon if no specific icon found
    return defaultFileIcon ? defaultFileIcon : (ImTextureID)0;
}

ImTextureID EditorContentBrowser::LoadPreviewTexture(const std::filesystem::path& filePath)
{
    return ImTextureID();
}

ImTextureID EditorContentBrowser::CreateTextureFromFile(
    const std::filesystem::path& path,
    ID3D12Device* device,
    ID3D12DescriptorHeap* srvHeap,
    ComPtr<ID3D12Resource>& outTexture)
{
    if (!device || !srvHeap || !std::filesystem::exists(path)) {
        return (ImTextureID)0;
    }

    // Load the image using stb_image
    int width, height, channels;
    unsigned char* imageData = stbi_load(path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!imageData) {
        return NULL;
    }

    // Create the texture resource
    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.MipLevels = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    HRESULT hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&outTexture)
    );

    if (FAILED(hr)) {
        stbi_image_free(imageData);
        return NULL;
    }

    // Create upload buffer
    const UINT64 uploadBufferSize = width * height * 4; // 4 bytes per pixel (RGBA)
    ComPtr<ID3D12Resource> uploadBuffer;
    heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

    hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &uploadDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&uploadBuffer)
    );

    if (FAILED(hr)) {
        stbi_image_free(imageData);
        return NULL;
    }

    // Copy data to upload buffer
    void* mappedData;
    D3D12_RANGE readRange = { 0, 0 };
    hr = uploadBuffer->Map(0, &readRange, &mappedData);
    if (FAILED(hr)) {
        stbi_image_free(imageData);
        return NULL;
    }

    memcpy(mappedData, imageData, uploadBufferSize);
    uploadBuffer->Unmap(0, nullptr);

    // Create command allocator and list
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    ComPtr<ID3D12GraphicsCommandList> commandList;

    hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
    if (FAILED(hr)) {
        stbi_image_free(imageData);
        return NULL;
    }

    hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(),
        nullptr, IID_PPV_ARGS(&commandList));
    if (FAILED(hr)) {
        stbi_image_free(imageData);
        return NULL;
    }

    // Copy data to texture
    D3D12_TEXTURE_COPY_LOCATION dst = {};
    dst.pResource = outTexture.Get();
    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION src = {};
    src.pResource = uploadBuffer.Get();
    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint.Offset = 0;
    src.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    src.PlacedFootprint.Footprint.Width = width;
    src.PlacedFootprint.Footprint.Height = height;
    src.PlacedFootprint.Footprint.Depth = 1;
    src.PlacedFootprint.Footprint.RowPitch = width * 4;

    commandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

    // Transition texture to shader resource
    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        outTexture.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );
    commandList->ResourceBarrier(1, &barrier);

    // Execute command list
    hr = commandList->Close();
    if (SUCCEEDED(hr)) {
        ID3D12CommandQueue* commandQueue = renderer->GetCommandQueue();
        ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
        commandQueue->ExecuteCommandLists(1, ppCommandLists);

        // Wait for GPU
        renderer->WaitForGPU();
    }

    // Create SRV
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    // Get descriptor handle
    UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    UINT descriptorIndex = renderer->AllocateDescriptor();

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = srvHeap->GetGPUDescriptorHandleForHeapStart();

    cpuHandle.ptr += descriptorIndex * descriptorSize;
    gpuHandle.ptr += descriptorIndex * descriptorSize;

    device->CreateShaderResourceView(outTexture.Get(), &srvDesc, cpuHandle);

    // Cleanup
    stbi_image_free(imageData);
    commandAllocator.Reset();
    commandList.Reset();
    uploadBuffer.Reset();

    // Convert GPU handle to ImTextureID
    #ifdef _WIN64
        return (ImTextureID)gpuHandle.ptr;
    #else
        return (ImTextureID)(UINT32)gpuHandle.ptr;
    #endif
}
