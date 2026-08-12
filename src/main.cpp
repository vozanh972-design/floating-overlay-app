// main.cpp
// Real native Windows application: Win32 window + Direct3D 11 device +
// Dear ImGui immediate-mode renderer. No Python, no bundled interpreter —
// this compiles straight to machine code via MSVC.

#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <tchar.h>
#include <dwmapi.h>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "app.h"
#include "ui/theme.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

// ---------------------------------------------------------------------- //
// D3D11 device / swapchain plumbing (standard Dear ImGui example pattern).
// ---------------------------------------------------------------------- //
static ID3D11Device*           g_pd3dDevice = nullptr;
static ID3D11DeviceContext*    g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*         g_pSwapChain = nullptr;
static bool                    g_SwapChainOccluded = false;
static UINT                    g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

static bool CreateDeviceD3D(HWND hWnd);
static void CleanupDeviceD3D();
static void CreateRenderTarget();
static void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static App* g_App = nullptr;

// Extra pixels around the window edge that are still treated as a resize
// handle even though the window has no OS-drawn border.
static constexpr int kResizeBorder = 6;

// ---------------------------------------------------------------------- //
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    WNDCLASSEXW wc = {
        sizeof(WNDCLASSEXW), CS_CLASSDC, WndProc, 0L, 0L,
        hInstance, nullptr, LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr,
        L"ADBConnectLDPlayerToolWndClass", nullptr
    };
    RegisterClassExW(&wc);

    // WS_OVERLAPPEDWINDOW keeps resize/minimize/maximize/Aero-snap behavior;
    // we strip the visual caption/border in WM_NCCALCSIZE below so we can
    // draw our own custom titlebar identical to the reference design.
    HWND hwnd = CreateWindowExW(
        0, wc.lpszClassName, L"ADB Connect - LDPlayer Tool",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1536, 1024,
        nullptr, nullptr, wc.hInstance, nullptr);

    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Dark titlebar for the (invisible) native chrome + taskbar thumbnails.
    BOOL useDark = TRUE;
    DwmSetWindowAttribute(hwnd, 20 /*DWMWA_USE_IMMERSIVE_DARK_MODE*/, &useDark, sizeof(useDark));

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr; // no imgui.ini clutter next to the exe

    Theme::ApplyStyle();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    App app;
    g_App = &app;
    app.fonts.ui      = Theme::LoadUIFont(17.0f);
    app.fonts.uiBold  = Theme::LoadBoldFont(17.0f);
    app.fonts.uiLarge = Theme::LoadBoldFont(20.0f);
    app.fonts.mono    = Theme::LoadMonoFont(15.0f);

    bool done = false;
    while (!done) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
            Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        app.Render(hwnd);

        ImGui::Render();
        const float clearColor[4] = { 0.04f, 0.047f, 0.07f, 1.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clearColor);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        HRESULT hr = g_pSwapChain->Present(1, 0); // vsync on
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}

// ---------------------------------------------------------------------- //
static bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd{};
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
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    HRESULT res = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2,
        D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) {
        res = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2,
            D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    }
    if (res != S_OK) return false;

    CreateRenderTarget();
    return true;
}

static void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

static void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer) {
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }
}

static void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// ---------------------------------------------------------------------- //
// WndProc: standard Dear ImGui forwarding + the borderless-window tricks
// (WM_NCCALCSIZE to remove the visible caption, WM_NCHITTEST to still allow
// dragging via our custom titlebar and resizing via the window edges).
// ---------------------------------------------------------------------- //
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;

    switch (msg) {
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED) {
            g_ResizeWidth = LOWORD(lParam);
            g_ResizeHeight = HIWORD(lParam);
        }
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_KEYMENU) return 0; // disable ALT menu beep
        break;

    case WM_NCCALCSIZE: {
        if (wParam == TRUE) {
            // Accept the proposed rect as-is (removes the standard caption),
            // but when maximized, inset by the frame size so the window
            // doesn't spill over the edges of the monitor.
            if (IsZoomed(hWnd)) {
                NCCALCSIZE_PARAMS* params = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
                int frame = GetSystemMetrics(SM_CXSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
                params->rgrc[0].left   += frame;
                params->rgrc[0].right  -= frame;
                params->rgrc[0].bottom -= frame;
                params->rgrc[0].top    += frame;
            }
            return 0;
        }
        break;
    }

    case WM_NCHITTEST: {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        RECT rc; GetWindowRect(hWnd, &rc);

        if (!IsZoomed(hWnd)) {
            bool left   = pt.x < rc.left + kResizeBorder;
            bool right  = pt.x >= rc.right - kResizeBorder;
            bool top    = pt.y < rc.top + kResizeBorder;
            bool bottom = pt.y >= rc.bottom - kResizeBorder;

            if (top && left) return HTTOPLEFT;
            if (top && right) return HTTOPRIGHT;
            if (bottom && left) return HTBOTTOMLEFT;
            if (bottom && right) return HTBOTTOMRIGHT;
            if (left) return HTLEFT;
            if (right) return HTRIGHT;
            if (top) return HTTOP;
            if (bottom) return HTBOTTOM;
        }

        if (g_TitlebarDragRect.valid) {
            POINT client = pt;
            ScreenToClient(hWnd, &client);
            if (client.x >= g_TitlebarDragRect.x0 && client.x <= g_TitlebarDragRect.x1 &&
                client.y >= g_TitlebarDragRect.y0 && client.y <= g_TitlebarDragRect.y1) {
                return HTCAPTION;
            }
        }
        return HTCLIENT;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}
