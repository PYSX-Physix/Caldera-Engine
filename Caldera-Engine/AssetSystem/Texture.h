#pragma once

#include <string>
#include <d3d12.h>
#include <wrl/client.h>

class Texture {
public:
    std::string name;
    Microsoft::WRL::ComPtr<ID3D12Resource> textureResource;
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;
    D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;

    bool LoadFromFile(const std::string& path, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
};
