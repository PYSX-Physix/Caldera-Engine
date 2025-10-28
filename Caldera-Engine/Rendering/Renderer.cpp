#include "Renderer.h"
#include "imconfig.h"
#include <dxgidebug.h>
#include <cassert>

using Microsoft::WRL::ComPtr;

Renderer::Renderer()
    : frameIndex(0),
    fenceValue(0),
    swapChainWaitableObject(nullptr)
{
    for (auto& handle : rtvHandles)
        handle.ptr = 0;
}


bool Renderer::Initialize(HWND hwnd) {
    if (!CreateDevice(hwnd)) return false;

    srvAllocator.Create(device.Get(), srvHeap.Get());
    // ImGui setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; 
    io.ConfigDpiScaleFonts = true;
    io.ConfigDpiScaleViewports = true;
    float scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    // Scale all widget sizes (buttons, padding, etc.)
    ImGui::GetStyle().ScaleAllSizes(scale);

    // Scale fonts
    
    io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");

    ImGui::StyleColorsDark();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 3.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        style.FontSizeBase = 16.0f;
    }

    ImGui_ImplWin32_Init(hwnd);

    ImGui_ImplDX12_InitInfo init_info = {};
    init_info.Device = device.Get();
    init_info.CommandQueue = commandQueue.Get();
    init_info.NumFramesInFlight = NUM_FRAMES_IN_FLIGHT;
    init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    init_info.SrvDescriptorHeap = srvHeap.Get();
    init_info.UserData = this;

    init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* cpu, D3D12_GPU_DESCRIPTOR_HANDLE* gpu) {
        Renderer* self = reinterpret_cast<Renderer*>(info->UserData);
        self->srvAllocator.Alloc(cpu, gpu);
        };

    init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu) {
        Renderer* self = reinterpret_cast<Renderer*>(info->UserData);
        self->srvAllocator.Free(cpu, gpu);
        };


    ImGui_ImplDX12_Init(&init_info);
    return true;
}

void Renderer::BeginFrame() {
    FrameContext* frameCtx = WaitForNextFrame();
    frameCtx->commandAllocator->Reset();

    frameIndex = swapChain->GetCurrentBackBufferIndex();
    commandList->Reset(frameCtx->commandAllocator.Get(), nullptr);

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = renderTargets[frameIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &barrier);

    commandList->OMSetRenderTargets(1, &rtvHandles[frameIndex], FALSE, nullptr);
    const float clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
    commandList->ClearRenderTargetView(rtvHandles[frameIndex], clearColor, 0, nullptr);

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

}

void Renderer::HandleResize(HWND hwnd, UINT width, UINT height) {
    float dpiScale = ImGui_ImplWin32_GetDpiScaleForMonitor(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST));

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear(); // Clear existing fonts
    io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 16.0f * dpiScale); // Re-add with scaled size
    io.Fonts->Build(); // Rebuild font atlas

    ImGui::GetStyle().ScaleAllSizes(dpiScale);
    io.FontGlobalScale = 1.0f; // Reset global scale since font size is now baked in

    CleanupRenderTargets();
    swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
    CreateRenderTargets();
}



void Renderer::EndFrame() {
    ImGui::Render();
    commandList->SetDescriptorHeaps(1, srvHeap.GetAddressOf());
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault(nullptr, (void*)commandList.Get()); // Must happen before Close()
    }

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = renderTargets[frameIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &barrier);

    commandList->Close();
    ID3D12CommandList* lists[] = { commandList.Get() };
    commandQueue->ExecuteCommandLists(1, lists);

    swapChain->Present(1, 0);
    fenceValue++;
    commandQueue->Signal(fence.Get(), fenceValue);
    frameContexts[frameIndex].fenceValue = fenceValue;

}

void Renderer::Shutdown() {
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyPlatformWindows();
    ImGui::DestroyContext();

    CleanupRenderTargets();
    CloseHandle(fenceEvent);
}

bool Renderer::CreateDevice(HWND hwnd) {
    // Device creation, swap chain setup, descriptor heaps, command queue, etc.
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC1 sd;
    {
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = NUM_BACK_BUFFERS;
        sd.Width = 0;
        sd.Height = 0;
        sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        sd.Scaling = DXGI_SCALING_STRETCH;
        sd.Stereo = FALSE;
    }

    // [DEBUG] Enable debug interface
#ifdef DX12_ENABLE_DEBUG_LAYER
    ID3D12Debug* pdx12Debug = nullptr;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pdx12Debug))))
        pdx12Debug->EnableDebugLayer();
#endif

    // Create device
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    if (D3D12CreateDevice(nullptr, featureLevel, IID_PPV_ARGS(&device)) != S_OK)
        return false;

    // [DEBUG] Setup debug interface to break on any warnings/errors
#ifdef DX12_ENABLE_DEBUG_LAYER
    if (pdx12Debug != nullptr)
    {
        ID3D12InfoQueue* pInfoQueue = nullptr;
        device->QueryInterface(IID_PPV_ARGS(&pInfoQueue));
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
        pInfoQueue->Release();
        pdx12Debug->Release();
    }
#endif

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.NumDescriptors = NUM_BACK_BUFFERS;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NodeMask = 1;
        if (device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&rtvHeap)) != S_OK)
            return false;

        SIZE_T rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
        {
            rtvHandles[i] = rtvHandle;
            rtvHandle.ptr += rtvDescriptorSize;
        }
    }

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = SRV_HEAP_SIZE;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srvHeap)) != S_OK)
            return false;
        srvAllocator.Create(device.Get(), srvHeap.Get());
    }

    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 1;
        if (device->CreateCommandQueue(&desc, IID_PPV_ARGS(&commandQueue)) != S_OK)
            return false;
    }

    for (UINT i = 0; i < NUM_FRAMES_IN_FLIGHT; i++)
        if (device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frameContexts[i].commandAllocator)) != S_OK)
            return false;

    if (device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, frameContexts[0].commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)) != S_OK ||
        commandList->Close() != S_OK)
        return false;

    if (device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)) != S_OK)
        return false;

    fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (fenceEvent == nullptr)
        return false;

    {
        IDXGIFactory5* dxgiFactory = nullptr;
        IDXGISwapChain1* swapChain1 = nullptr;
        if (CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)) != S_OK)
            return false;

        BOOL allow_tearing = FALSE;
        dxgiFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allow_tearing, sizeof(allow_tearing));
        tearingSupported = (allow_tearing == TRUE);
        if (tearingSupported)
            sd.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

        if (dxgiFactory->CreateSwapChainForHwnd(commandQueue.Get(), hwnd, &sd, nullptr, nullptr, &swapChain1) != S_OK)
            return false;
        if (swapChain1->QueryInterface(IID_PPV_ARGS(&swapChain)) != S_OK)
            return false;
        if (tearingSupported)
            dxgiFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

        swapChain1->Release();
        dxgiFactory->Release();
        swapChain->SetMaximumFrameLatency(NUM_BACK_BUFFERS);
        swapChainWaitableObject = swapChain->GetFrameLatencyWaitableObject();
    }

    CreateRenderTargets();
    return true;
}


void Renderer::CreateRenderTargets() {
    for (UINT i = 0; i < NUM_BACK_BUFFERS; i++) {
        ComPtr<ID3D12Resource> backBuffer;
        swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));
        device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandles[i]);
        renderTargets[i] = backBuffer;
    }
}

void Renderer::CleanupRenderTargets() {
    for (UINT i = 0; i < NUM_BACK_BUFFERS; i++) {
        renderTargets[i].Reset();
    }
}

FrameContext* Renderer::WaitForNextFrame() {
    FrameContext* ctx = &frameContexts[frameIndex % NUM_FRAMES_IN_FLIGHT];
    if (fence->GetCompletedValue() < ctx->fenceValue) {
        fence->SetEventOnCompletion(ctx->fenceValue, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
    }
    return ctx;
}

void Renderer::WaitForGPU() {
    commandQueue->Signal(fence.Get(), ++fenceValue);
    fence->SetEventOnCompletion(fenceValue, fenceEvent);
    WaitForSingleObject(fenceEvent, INFINITE);
}

DescriptorHeapAllocator::DescriptorHeapAllocator()
    : heap(nullptr),
    heapType(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
    startCpu({ 0 }),
    startGpu({ 0 }),
    descriptorSize(0)
{
}


void DescriptorHeapAllocator::Create(ID3D12Device* device, ID3D12DescriptorHeap* heap) {
    this->heap = heap;
    this->heapType = heap->GetDesc().Type;
    this->handleIncrement = device->GetDescriptorHandleIncrementSize(heapType);

    this->startCpu = heap->GetCPUDescriptorHandleForHeapStart();
    this->startGpu = heap->GetGPUDescriptorHandleForHeapStart();

    freeIndices.clear();
    for (int i = 0; i < SRV_HEAP_SIZE; ++i) {
        freeIndices.push_back(i);
    }
}


void DescriptorHeapAllocator::Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle) {
    IM_ASSERT(freeIndices.Size > 0);
    int idx = freeIndices.back();
    freeIndices.pop_back();
    out_cpu_desc_handle->ptr = startCpu.ptr + (idx * handleIncrement);
    out_gpu_desc_handle->ptr = startGpu.ptr + (idx * handleIncrement);
}

void DescriptorHeapAllocator::Free(D3D12_CPU_DESCRIPTOR_HANDLE cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_desc_handle) {
    int cpu_idx = static_cast<int>((cpu_desc_handle.ptr - startCpu.ptr) / handleIncrement);
    int gpu_idx = static_cast<int>((gpu_desc_handle.ptr - startGpu.ptr) / handleIncrement);
    IM_ASSERT(cpu_idx == gpu_idx);
    freeIndices.push_back(cpu_idx);
}
