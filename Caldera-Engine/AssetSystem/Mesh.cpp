#include "Mesh.h"
#include <cassert>

void Mesh::UploadToGPU(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList) {
    assert(device && "Device is null");
    assert(cmdList && "Command list is null");

    if (vertices.empty() || indices.empty()) {
        return;
    }

    // Create vertex buffer
    const UINT vbSize = static_cast<UINT>(vertices.size() * sizeof(Vertex));

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width = vbSize;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&vertexBuffer)
    );
    assert(SUCCEEDED(hr) && "Failed to create vertex buffer");

    // Copy vertex data
    void* pVertexDataBegin;
    D3D12_RANGE readRange = { 0, 0 };
    vertexBuffer->Map(0, &readRange, &pVertexDataBegin);
    memcpy(pVertexDataBegin, vertices.data(), vbSize);
    vertexBuffer->Unmap(0, nullptr);

    // Setup vertex buffer view
    vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vbView.StrideInBytes = sizeof(Vertex);
    vbView.SizeInBytes = vbSize;

    // Create index buffer
    const UINT ibSize = static_cast<UINT>(indices.size() * sizeof(uint32_t));

    resourceDesc.Width = ibSize;

    hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&indexBuffer)
    );
    assert(SUCCEEDED(hr) && "Failed to create index buffer");

    // Copy index data
    void* pIndexDataBegin;
    indexBuffer->Map(0, &readRange, &pIndexDataBegin);
    memcpy(pIndexDataBegin, indices.data(), ibSize);
    indexBuffer->Unmap(0, nullptr);

    // Setup index buffer view
    ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
    ibView.Format = DXGI_FORMAT_R32_UINT;
    ibView.SizeInBytes = ibSize;
}