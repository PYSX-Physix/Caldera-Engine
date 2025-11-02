//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once

#include <d3d12.h>

//------------------------------------------------------------------------------------------------
inline UINT64 GetRequiredIntermediateSize(
    _In_ ID3D12Resource* pDestinationResource,
    _In_range_(0, D3D12_REQ_SUBRESOURCES) UINT FirstSubresource,
    _In_range_(0, D3D12_REQ_SUBRESOURCES - FirstSubresource) UINT NumSubresources)
{
    D3D12_RESOURCE_DESC Desc = pDestinationResource->GetDesc();
    UINT64 RequiredSize = 0;

    ID3D12Device* pDevice = nullptr;
    pDestinationResource->GetDevice(__uuidof(*pDevice), reinterpret_cast<void**>(&pDevice));
    pDevice->GetCopyableFootprints(&Desc, FirstSubresource, NumSubresources, 0, nullptr, nullptr, nullptr, &RequiredSize);
    pDevice->Release();

    return RequiredSize;
}

//------------------------------------------------------------------------------------------------
struct CD3DX12_HEAP_PROPERTIES : public D3D12_HEAP_PROPERTIES
{
    CD3DX12_HEAP_PROPERTIES() = default;
    explicit CD3DX12_HEAP_PROPERTIES(const D3D12_HEAP_PROPERTIES& o) :
        D3D12_HEAP_PROPERTIES(o)
    {
    }
    CD3DX12_HEAP_PROPERTIES(
        D3D12_CPU_PAGE_PROPERTY cpuPageProperty,
        D3D12_MEMORY_POOL memoryPoolPreference,
        UINT creationNodeMask = 1,
        UINT nodeMask = 1)
    {
        Type = D3D12_HEAP_TYPE_CUSTOM;
        CPUPageProperty = cpuPageProperty;
        MemoryPoolPreference = memoryPoolPreference;
        CreationNodeMask = creationNodeMask;
        VisibleNodeMask = nodeMask;
    }
    explicit CD3DX12_HEAP_PROPERTIES(
        D3D12_HEAP_TYPE type,
        UINT creationNodeMask = 1,
        UINT nodeMask = 1)
    {
        Type = type;
        CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        CreationNodeMask = creationNodeMask;
        VisibleNodeMask = nodeMask;
    }
};

//------------------------------------------------------------------------------------------------
struct CD3DX12_RESOURCE_DESC : public D3D12_RESOURCE_DESC
{
    CD3DX12_RESOURCE_DESC() = default;
    explicit CD3DX12_RESOURCE_DESC(const D3D12_RESOURCE_DESC& o) :
        D3D12_RESOURCE_DESC(o)
    {
    }
    CD3DX12_RESOURCE_DESC(
        D3D12_RESOURCE_DIMENSION dimension,
        UINT64 alignment,
        UINT64 width,
        UINT height,
        UINT16 depthOrArraySize,
        UINT16 mipLevels,
        DXGI_FORMAT format,
        UINT sampleCount,
        UINT sampleQuality,
        D3D12_TEXTURE_LAYOUT layout,
        D3D12_RESOURCE_FLAGS flags)
    {
        Dimension = dimension;
        Alignment = alignment;
        Width = width;
        Height = height;
        DepthOrArraySize = depthOrArraySize;
        MipLevels = mipLevels;
        Format = format;
        SampleDesc.Count = sampleCount;
        SampleDesc.Quality = sampleQuality;
        Layout = layout;
        Flags = flags;
    }
    static inline CD3DX12_RESOURCE_DESC Buffer(
        UINT64 width,
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
        UINT64 alignment = 0)
    {
        return CD3DX12_RESOURCE_DESC(D3D12_RESOURCE_DIMENSION_BUFFER, alignment, width, 1, 1, 1,
            DXGI_FORMAT_UNKNOWN, 1, 0, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, flags);
    }
};

//------------------------------------------------------------------------------------------------
struct CD3DX12_RESOURCE_BARRIER : public D3D12_RESOURCE_BARRIER
{
    CD3DX12_RESOURCE_BARRIER() 
    {
        ZeroMemory(this, sizeof(this));
    }
    explicit CD3DX12_RESOURCE_BARRIER(const D3D12_RESOURCE_BARRIER& o) :
        D3D12_RESOURCE_BARRIER(o)
    {
    }
    static inline CD3DX12_RESOURCE_BARRIER Transition(
        _In_ ID3D12Resource* pResource,
        D3D12_RESOURCE_STATES stateBefore,
        D3D12_RESOURCE_STATES stateAfter,
        UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
        D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE)
    {
        CD3DX12_RESOURCE_BARRIER result;
        ZeroMemory(&result, sizeof(result));
        D3D12_RESOURCE_BARRIER& barrier = result;
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = flags;
        barrier.Transition.pResource = pResource;
        barrier.Transition.StateBefore = stateBefore;
        barrier.Transition.StateAfter = stateAfter;
        barrier.Transition.Subresource = subresource;
        return result;
    }
    static inline CD3DX12_RESOURCE_BARRIER Aliasing(
        _In_ ID3D12Resource* pResourceBefore,
        _In_ ID3D12Resource* pResourceAfter)
    {
        CD3DX12_RESOURCE_BARRIER result;
        ZeroMemory(&result, sizeof(result));
        D3D12_RESOURCE_BARRIER& barrier = result;
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Aliasing.pResourceBefore = pResourceBefore;
        barrier.Aliasing.pResourceAfter = pResourceAfter;
        return result;
    }
    static inline CD3DX12_RESOURCE_BARRIER UAV(
        _In_ ID3D12Resource* pResource)
    {
        CD3DX12_RESOURCE_BARRIER result;
        ZeroMemory(&result, sizeof(result));
        D3D12_RESOURCE_BARRIER& barrier = result;
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.UAV.pResource = pResource;
        return result;
    }
};