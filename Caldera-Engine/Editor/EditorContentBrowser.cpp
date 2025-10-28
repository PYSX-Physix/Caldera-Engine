#include "EditorContentBrowser.h"
#include "imgui.h"
#include "imgui_impl_dx12.h"
#include <print>

// Filesystem
#include <filesystem>

// COM and WIC
#include <wincodec.h>              // WIC interfaces
#include <comdef.h>                // _com_error for debugging
#include <wrl/client.h>            // Microsoft::WRL::ComPtr

// DirectX 12
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dx12.h>                // Optional: helper macros like CD3DX12_HEAP_PROPERTIES

// STL
#include <vector>
#include <string>

void EditorContentBrowser::SetRootDirectory(const std::filesystem::path& root) {
    rootPath = root;
    currentDirectory = root;
    canOpen = true;
}

void EditorContentBrowser::Render(bool* isOpen) 
{
    {
    
        if (!ImGui::Begin("Content Browser", isOpen)) {
            ImGui::End();
            return;
        }

        ImGui::Columns(3, nullptr, true);

        // Column 1: Folder Tree
        ImGui::BeginChild("Directories", ImVec2(0, 0), true);
        DrawDirectoryTree(rootPath);
        ImGui::EndChild();
        ImGui::NextColumn();

        // Column 2: File List
        ImGui::BeginChild("Assets", ImVec2(0, 0), true);
        DrawAssetList(currentDirectory);
        ImGui::EndChild();
        ImGui::NextColumn();

        // Column 3: Preview Panel
        ImGui::BeginChild("Preview", ImVec2(0, 0), true);
        DrawPreviewPanel();
        ImGui::EndChild();

        ImGui::Columns(1);
		ImGui::End();
    }
}

void EditorContentBrowser::SetRenderer(Renderer* r)
{
	renderer = r;
}


void EditorContentBrowser::DrawDirectoryTree(const std::filesystem::path& path) {
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (entry.is_directory()) {
            const std::string name = entry.path().filename().string();
            ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
            bool nodeOpen = ImGui::TreeNodeEx(name.c_str(), nodeFlags);

            if (ImGui::IsItemClicked()) {
                currentDirectory = entry.path();
            }

            if (nodeOpen) {
                DrawDirectoryTree(entry.path());
                ImGui::TreePop();
            }

        }
    }
}


void EditorContentBrowser::DrawAssetList(const std::filesystem::path& path) {
    try {
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (!entry.is_directory()) {
                const std::string name = entry.path().filename().string();
                const std::string ext = entry.path().extension().string();

                if (ImGui::Selectable(name.c_str(), selectedFile == entry.path())) {
                    selectedFile = entry.path();
                }

                // Optional: show metadata
                ImGui::SameLine(300);
                ImGui::TextDisabled("%s", ext.c_str());
            }
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        ImGui::Text("Error reading directory: %s", e.what());
    }
}

void EditorContentBrowser::DrawPreviewPanel() {
    if (selectedFile.empty()) {
        ImGui::Text("No file selected.");
        return;
    }

    ImGui::Text("Selected: %s", selectedFile.filename().string().c_str());

    // Optional: show thumbnail if it's an image
    std::string ext = selectedFile.extension().string();
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") {
        // Load and display texture (requires your engine's texture loader)
        ImTextureID texID = LoadPreviewTexture(selectedFile); // hypothetical
        ImGui::Image(texID, ImVec2(128, 128));
    }

    // Show metadata
    auto size = std::filesystem::file_size(selectedFile);
    ImGui::Text("Size: %llu bytes", size);
}

ImTextureID EditorContentBrowser::LoadPreviewTexture(const std::filesystem::path& path) {
    if (previewTextureIDs.contains(path))
        return previewTextureIDs[path];

    // Load image using WIC (or stb_image)
    ComPtr<ID3D12Resource> texture;

    ImTextureID texID = CreateTextureFromFile(
        selectedFile,
        renderer->GetDevice(),
        renderer->GetSrvHeap(),
        texture
    );

    if (texID) {
        previewTextures[path] = texture;
        previewTextureIDs[path] = texID;
    }

    return texID;
}

ImTextureID EditorContentBrowser::CreateTextureFromFile(const std::filesystem::path& path, ID3D12Device* device, ID3D12DescriptorHeap* srvHeap, ComPtr<ID3D12Resource>& outTexture)
{
    // 1. Load image using WIC
    ComPtr<IWICImagingFactory> wicFactory;
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicFactory));

    ComPtr<IWICBitmapDecoder> decoder;
    wicFactory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);

    ComPtr<IWICBitmapFrameDecode> frame;
    decoder->GetFrame(0, &frame);

    ComPtr<IWICFormatConverter> converter;
    wicFactory->CreateFormatConverter(&converter);
    converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);

    UINT width, height;
    converter->GetSize(&width, &height);
    std::vector<BYTE> pixels(width * height * 4);
    converter->CopyPixels(nullptr, width * 4, pixels.size(), pixels.data());

    // 2. Create texture resource
    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
 
    device->CreateCommittedResource(
        &heapProps, // named variable
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&outTexture)
    );

    // 3. Create upload heap and copy data
    // (You’ll need to create a command list and copy the pixel data here)

    // 4. Create SRV
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;

    D3D12_CPU_DESCRIPTOR_HANDLE handle = srvHeap->GetCPUDescriptorHandleForHeapStart();
    device->CreateShaderResourceView(outTexture.Get(), &srvDesc, handle);

    // 5. Return ImTextureID
    return (ImTextureID)srvHeap->GetGPUDescriptorHandleForHeapStart().ptr;
}