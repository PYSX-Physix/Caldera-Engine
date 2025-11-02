#include "Rendering/Renderer.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include <windows.h>

#include "Editor/Caldera-Editor.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

Renderer renderer;
EditorBase edbase;

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (renderer.GetDevice() && wParam != SIZE_MINIMIZED) {
            renderer.CleanupRenderTargets();
            DXGI_SWAP_CHAIN_DESC1 desc = {};
            renderer.swapChain->GetDesc1(&desc);
            HRESULT result = renderer.swapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), desc.Format, desc.Flags);
            IM_ASSERT(SUCCEEDED(result) && "Failed to resize swapchain.");
            renderer.CreateRenderTargets();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    ImGui_ImplWin32_EnableDpiAwareness();
    float scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_OWNDC | CS_HREDRAW | CS_VREDRAW, WndProc, 0L, 0L, hInstance, nullptr, nullptr, nullptr, nullptr, L"CalderaEngine", nullptr };
    ::RegisterClassEx(&wc);

    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Caldera Engine", WS_OVERLAPPEDWINDOW, 100, 100, int(1280 * scale), int(800 * scale), nullptr, nullptr, wc.hInstance, nullptr);

    if (!renderer.Initialize(hwnd)) {
        renderer.Shutdown();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Set the renderer for the editor after initialization
    edbase.SetRenderer(&renderer);

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    bool done = false;
    while (!done) {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done) break;

        /* DO NOT PUT LAYOUT CODE HERE ONLY PUT EDITOR BASE FUNCTION FOR CREATING LAYOUT*/
        renderer.BeginFrame();
        ImGui::DockSpaceOverViewport();
        {
            edbase.ConstructEditorLayout();
        }

        renderer.EndFrame();
    }

    renderer.Shutdown();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}