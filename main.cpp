#include <windows.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <tchar.h>

#define IMGUI_IMPL_WIN32_ENABLE

#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"



// Link required libraries
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "d3d11.lib")

// Globals
HWND hwnd = nullptr;
ID3D11Device *g_pd3dDevice = nullptr;
ID3D11DeviceContext *g_pd3dDeviceContext = nullptr;
IDXGISwapChain *g_pSwapChain = nullptr;
ID3D11RenderTargetView *g_mainRenderTargetView = nullptr;

// Forward declarations
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam); // 👈 Add this!
void CreateRenderTarget();
void CleanupRenderTarget();
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    // Register class
    WNDCLASSEX wc = {
        sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L,
        GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
        _T("OverlayClass"), NULL};
    RegisterClassEx(&wc);

    // Create window (borderless, topmost, transparent, layered)
    hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT,
        wc.lpszClassName, _T("Overlay Window"),
        WS_POPUP, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        NULL, NULL, wc.hInstance, NULL);

    // Make window transparent
    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);

    LONG style = GetWindowLong(hwnd, GWL_EXSTYLE);
    SetWindowLong(hwnd, GWL_EXSTYLE, style | WS_EX_LAYERED | WS_EX_TRANSPARENT);

    // Exclude from screen capture
    BOOL exclude = TRUE;
    DwmSetWindowAttribute(hwnd, 0x1B /* DWMWA_EXCLUDED_FROM_CAPTURE */, &exclude, sizeof(exclude));

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show window
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Main loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        // Start ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Your UI
        ImGui::Begin("Overlay");
        ImGui::Text("This is a hidden overlay");
        ImGui::End();

        ImGui::Render();
        const float clear_color_with_alpha[4] = {0, 0, 0, 0};
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0); // Vsync
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// DirectX Init
bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // For newer systems

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    const D3D_DRIVER_TYPE driverTypes[] = {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    const D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };

    for (UINT driverTypeIndex = 0; driverTypeIndex < ARRAYSIZE(driverTypes); driverTypeIndex++)
    {
        if (D3D11CreateDeviceAndSwapChain(NULL, driverTypes[driverTypeIndex], NULL,
                                          createDeviceFlags, featureLevels, ARRAYSIZE(featureLevels),
                                          D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice,
                                          NULL, &g_pd3dDeviceContext) == S_OK)
            break;
    }

    if (!g_pSwapChain)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain)
    {
        g_pSwapChain->Release();
        g_pSwapChain = nullptr;
    }
    if (g_pd3dDeviceContext)
    {
        g_pd3dDeviceContext->Release();
        g_pd3dDeviceContext = nullptr;
    }
    if (g_pd3dDevice)
    {
        g_pd3dDevice->Release();
        g_pd3dDevice = nullptr;
    }
}

void CreateRenderTarget()
{
    ID3D11Texture2D *pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView)
    {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }
}
