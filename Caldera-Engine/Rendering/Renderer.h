#pragma once
#include <d3d12.h>
#include <dxgi1_5.h>
#include <wrl/client.h>
#include <windows.h>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

// Constants
constexpr int NUM_FRAMES_IN_FLIGHT = 2;
constexpr int NUM_BACK_BUFFERS = 2;
constexpr int SRV_HEAP_SIZE = 128;

struct FrameContext {
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
    UINT64 fenceValue = 0;
};

class DescriptorHeapAllocator {
public:
    void Create(ID3D12Device* device, ID3D12DescriptorHeap* heap);
    void Destroy();
    void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* outCpu, D3D12_GPU_DESCRIPTOR_HANDLE* outGpu);
    void Free(D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu);

private:
    ID3D12DescriptorHeap* heap = nullptr;
    D3D12_DESCRIPTOR_HEAP_TYPE heapType;
    D3D12_CPU_DESCRIPTOR_HANDLE startCpu;
    D3D12_GPU_DESCRIPTOR_HANDLE startGpu;
    UINT handleIncrement = 0;
    ImVector<int> freeIndices;
};

class Renderer {
public:
    bool Initialize(HWND hwnd);
    void BeginFrame();
    void EndFrame();
    void Shutdown();

    ID3D12Device* GetDevice() const { return device.Get(); }
    ID3D12GraphicsCommandList* GetCommandList() const { return commandList.Get(); }
    ID3D12DescriptorHeap* GetSrvHeap() const { return srvHeap.Get(); }

    void HandleResize(HWND hwnd, UINT width, UINT height);


private:
    bool CreateDevice(HWND hwnd);
    void CreateRenderTargets();
    void CleanupRenderTargets();
    void WaitForGPU();
    FrameContext* WaitForNextFrame();

    Microsoft::WRL::ComPtr<ID3D12Device> device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap;
    DescriptorHeapAllocator srvAllocator;

    Microsoft::WRL::ComPtr<ID3D12Resource> renderTargets[NUM_BACK_BUFFERS];
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[NUM_BACK_BUFFERS];

    FrameContext frameContexts[NUM_FRAMES_IN_FLIGHT];
    HANDLE fenceEvent = nullptr;
    HANDLE swapChainWaitableObject = nullptr;
    UINT64 fenceValue = 0;
    UINT frameIndex = 0;
    bool tearingSupported = false;
    bool swapChainOccluded = false;
    bool multipleViewports = false;
};
