#include "Renderer.h"
#include "imconfig.h"
#include <dxgidebug.h>
#include <cassert>
#include <d3dcompiler.h>
#include "d3dx12.h"
#pragma comment(lib, "d3dcompiler.lib")

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

	CreateDefaultResources();

    ImGui_ImplDX12_Init(&init_info);
    return true;
}


void Renderer::BeginFrame()
{
    // Wait for previous frame and reset command allocator
    FrameContext* frameCtx = WaitForNextFrame();
    frameCtx->commandAllocator->Reset();

    // Get current back buffer index and reset command list
    frameIndex = swapChain->GetCurrentBackBufferIndex();
    commandList->Reset(frameCtx->commandAllocator.Get(), nullptr);

    // Transition the back buffer from present to render target
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = renderTargets[frameIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &barrier);

    // Set render target and clear it
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHandles[frameIndex];
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {}; // Add depth stencil view handle when implemented
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr); // Replace nullptr with &dsvHandle when depth is added

    // Clear the render target with a dark gray color
    const float clearColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    if (dsvHandle.ptr != 0)
    {
        // Clear depth buffer (when implemented)
        commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    }

    // Set viewport and scissor rect
    D3D12_VIEWPORT viewport = { 0.0f, 0.0f, viewportWidth, viewportHeight, 0.0f, 1.0f };
    D3D12_RECT scissorRect = { 0, 0, (LONG)viewportWidth, (LONG)viewportHeight };

    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    // Render 3D scene
    RenderScene();

    // Begin ImGui frame
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}


void Renderer::CreateGraphicsPipeline()
{
    // Create root signature
    {
        D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
        rootSignatureDesc.NumParameters = 0;  // We'll add parameters later for transformations
        rootSignatureDesc.NumStaticSamplers = 0;
        rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
        device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
    }

    // Create the pipeline state
    {
        // Define the vertex input layout
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        // Create basic shaders
        const char* vertexShader = R"(
            struct VSInput {
                float3 position : POSITION;
                float3 color : COLOR;
            };
            struct PSInput {
                float4 position : SV_POSITION;
                float3 color : COLOR;
            };
            PSInput main(VSInput input) {
                PSInput output;
                output.position = float4(input.position, 1.0f);
                output.color = input.color;
                return output;
            }
        )";

        const char* pixelShader = R"(
            struct PSInput {
                float4 position : SV_POSITION;
                float3 color : COLOR;
            };
            float4 main(PSInput input) : SV_TARGET {
                return float4(input.color, 1.0f);
            }
        )";

        ComPtr<ID3DBlob> vertexShaderBlob;
        ComPtr<ID3DBlob> pixelShaderBlob;
        ComPtr<ID3DBlob> errorBlob;

        D3DCompile(vertexShader, strlen(vertexShader), nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0, &vertexShaderBlob, &errorBlob);
        D3DCompile(pixelShader, strlen(pixelShader), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &pixelShaderBlob, &errorBlob);

        // Create pipeline state object
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        psoDesc.pRootSignature = rootSignature.Get();
        psoDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
        psoDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;

        device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
    }
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

UINT Renderer::AllocateDescriptor()
{
    if (currentDescriptorIndex >= maxDescriptors) {
        // Handle descriptor heap overflow - you might want to create a new heap
        // For now, wrap around to the beginning
        currentDescriptorIndex = 0;
    }
    return currentDescriptorIndex++;
}

ImTextureID Renderer::GetCurrentBackBufferImGui()
{
    // Get the current back buffer index
    UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

    // Get SRV descriptor handle
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();
    srvHandle.ptr += backBufferIndex * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Return the descriptor as a texture ID for ImGui
    return (ImTextureID)srvHeap->GetGPUDescriptorHandleForHeapStart().ptr;
}

void Renderer::SetViewportSize(float width, float height)
{
    if (width != viewportWidth || height != viewportHeight) 
    {
        viewportWidth = width;
        viewportHeight = height;
        viewportResized = true;
    }
}

void Renderer::CreateDefaultResources()
{
    // Define a simple cube vertex structure
    struct Vertex {
        float position[3];
        float color[3];
    };

    // Create cube vertices (8 vertices for a cube)
    Vertex cubeVertices[] = {
        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}}, // 0
        {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}}, // 1
        {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}}, // 2
        {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}}, // 3
        {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}}, // 4
        {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}}, // 5
        {{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}}, // 6
        {{-0.5f,  0.5f,  0.5f}, {0.5f, 0.5f, 0.5f}}  // 7
    };

    // Create cube indices (36 indices for 12 triangles)
    UINT cubeIndices[] = {
        // Front face
        0, 1, 2, 0, 2, 3,
        // Back face
        4, 6, 5, 4, 7, 6,
        // Top face
        3, 2, 6, 3, 6, 7,
        // Bottom face
        0, 5, 1, 0, 4, 5,
        // Left face
        0, 3, 7, 0, 7, 4,
        // Right face
        1, 5, 6, 1, 6, 2
    };

    // Create vertex buffer
    {
        const UINT vertexBufferSize = sizeof(cubeVertices);

        D3D12_HEAP_PROPERTIES heapProps = { D3D12_HEAP_TYPE_UPLOAD };
        D3D12_RESOURCE_DESC bufferDesc = {};
        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferDesc.Width = vertexBufferSize;
        bufferDesc.Height = 1;
        bufferDesc.DepthOrArraySize = 1;
        bufferDesc.MipLevels = 1;
        bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        bufferDesc.SampleDesc.Count = 1;
        bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&vertexBuffer));

        // Copy vertex data to the buffer
        UINT8* pVertexDataBegin;
        vertexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pVertexDataBegin));
        memcpy(pVertexDataBegin, cubeVertices, vertexBufferSize);
        vertexBuffer->Unmap(0, nullptr);

        // Initialize the vertex buffer view
        vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
        vertexBufferView.StrideInBytes = sizeof(Vertex);
        vertexBufferView.SizeInBytes = vertexBufferSize;
    }

    // Create index buffer
    {
        const UINT indexBufferSize = sizeof(cubeIndices);

        D3D12_HEAP_PROPERTIES heapProps = { D3D12_HEAP_TYPE_UPLOAD };
        D3D12_RESOURCE_DESC bufferDesc = {};
        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferDesc.Width = indexBufferSize;
        bufferDesc.Height = 1;
        bufferDesc.DepthOrArraySize = 1;
        bufferDesc.MipLevels = 1;
        bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        bufferDesc.SampleDesc.Count = 1;
        bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&indexBuffer));

        // Copy index data to the buffer
        UINT8* pIndexDataBegin;
        indexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pIndexDataBegin));
        memcpy(pIndexDataBegin, cubeIndices, indexBufferSize);
        indexBuffer->Unmap(0, nullptr);

        // Initialize the index buffer view
        indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
        indexBufferView.Format = DXGI_FORMAT_R32_UINT;
        indexBufferView.SizeInBytes = indexBufferSize;
    }
}

void Renderer::RenderScene()
{
    if (!pipelineState)
    {
        // Skip rendering if pipeline state isn't set up yet
        return;
    }

    // Set the graphics root signature
    commandList->SetGraphicsRootSignature(rootSignature.Get());

    // Set the pipeline state
    commandList->SetPipelineState(pipelineState.Get());

    // Set primitive topology
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Set vertex and index buffers
    if (vertexBuffer && indexBuffer)
    {
        commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
        commandList->IASetIndexBuffer(&indexBufferView);

        // Update constant buffer with latest camera/transform data
        // Assuming we're using slot 0 for our constant buffer
        commandList->SetGraphicsRootConstantBufferView(0, constantBuffer->GetGPUVirtualAddress());

        // Draw the geometry
        // Using 36 indices for a cube (6 faces * 2 triangles * 3 vertices)
        commandList->DrawIndexedInstanced(36, 1, 0, 0, 0);
    }
}


void Renderer::CreateDepthBuffer()
{
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    D3D12_RESOURCE_DESC depthDesc = {};
    depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthDesc.Width = (UINT64)viewportWidth;
    depthDesc.Height = (UINT)viewportHeight;
    depthDesc.DepthOrArraySize = 1;
    depthDesc.MipLevels = 1;
    depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DXGI_FORMAT_D32_FLOAT;
    clearValue.DepthStencil.Depth = 1.0f;

    device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &depthDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_PPV_ARGS(&depthBuffer)
    );

    // Create DSV heap and view
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap));

    dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
    device->CreateDepthStencilView(depthBuffer.Get(), nullptr, dsvHandle);
}

void Renderer::UpdateViewport()
{
	CleanupRenderTargets();
    swapChain->ResizeBuffers(0, (UINT)viewportWidth, (UINT)viewportHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 0);

	CreateRenderTargets();

	float aspectRatio = viewportWidth / viewportHeight;
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
        // FIXED: Changed infoQueue to pInfoQueue
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
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