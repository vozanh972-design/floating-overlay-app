#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include <mutex>
#include <atomic>
#include <chrono>
#include "imgui.h"
#include "core/adb_manager.h"
#include "core/device.h"

enum class ViewId {
    Connect,
    Devices,
    Control,
    Apps,
    Files,
    Shell,
    Settings,
    About,
};

// Global rectangle (client coordinates) the custom title bar reports as its
// "drag to move the window" area each frame; main.cpp's WM_NCHITTEST handler
// reads this to know when to return HTCAPTION.
struct DragRect { float x0=0, y0=0, x1=0, y1=0; bool valid=false; };
extern DragRect g_TitlebarDragRect;

struct AppFonts {
    ImFont* ui       = nullptr;
    ImFont* uiBold   = nullptr;
    ImFont* uiLarge  = nullptr;
    ImFont* mono     = nullptr;
};

class App {
public:
    App();

    void Render(HWND hwnd);  // called once per frame, builds the whole UI

    core::AdbManager adb;

    std::vector<core::Device> devices;
    std::string                selectedSerial;
    std::mutex                 devicesMutex;
    std::atomic<bool>          refreshing{false};

    ViewId currentView = ViewId::Connect;

    AppFonts fonts;

    // Connection mode radio state for the Connect screen.
    int connectMode = 0; // 0 = auto, 1 = manual, 2 = wifi
    char manualAddress[128] = "127.0.0.1:5555";

    std::chrono::steady_clock::time_point connectedSince{};
    std::chrono::steady_clock::time_point lastAutoRefresh{};

    // --- ADB Shell view state ---
    std::string shellOutput;
    char        shellInput[512] = "";

    // --- Settings view state ---
    char        adbPathBuffer[512] = "";
    std::string adbPathStatusMsg;

    // --- Apps (APK manager) view state ---
    std::wstring selectedApkPath;
    std::string  apkStatusMsg;
    char         uninstallPackageBuffer[256] = "com.example.package";

    void RefreshDevicesAsync();
    core::Device* GetSelectedDevice();
    void SelectDevice(const std::string& serial);

private:
    void RenderTitlebar(HWND hwnd);
    void RenderSidebar();
    void RenderContent();
};
