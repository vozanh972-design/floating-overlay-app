#include "app.h"
#include <windows.h>
#include <windowsx.h>
#include <thread>
#include "ui/theme.h"
#include "ui/icons.h"
#include "ui/views.h"
#include <algorithm>

DragRect g_TitlebarDragRect;

App::App() {
    RefreshDevicesAsync();
    std::string p = core::WideToUtf8Str(adb.GetAdbPath());
    strncpy_s(adbPathBuffer, p.c_str(), sizeof(adbPathBuffer) - 1);
}

core::Device* App::GetSelectedDevice() {
    std::lock_guard<std::mutex> lock(devicesMutex);
    for (auto& d : devices) if (d.serial == selectedSerial) return &d;
    return nullptr;
}

void App::SelectDevice(const std::string& serial) {
    selectedSerial = serial;
    auto* d = GetSelectedDevice();
    if (d && !d->infoLoaded) {
        std::thread([this, serial]() {
            core::Device copy;
            {
                std::lock_guard<std::mutex> lock(devicesMutex);
                for (auto& dd : devices) if (dd.serial == serial) copy = dd;
            }
            if (copy.serial.empty()) return;
            adb.LoadExtendedInfo(copy);
            std::lock_guard<std::mutex> lock(devicesMutex);
            for (auto& dd : devices) {
                if (dd.serial == serial) dd = copy;
            }
        }).detach();
    }
}

void App::RefreshDevicesAsync() {
    if (refreshing.exchange(true)) return;
    std::thread([this]() {
        auto result = adb.ListDevicesRaw();
        std::vector<core::Device> parsed;
        if (result.Success()) {
            parsed = adb.ParseDevices(result.stdOut);
        }
        {
            std::lock_guard<std::mutex> lock(devicesMutex);
            for (auto& nd : parsed) {
                for (auto& od : devices) {
                    if (od.serial == nd.serial && od.infoLoaded) {
                        nd = od;
                        break;
                    }
                }
            }
            devices = parsed;
            bool stillPresent = false;
            for (auto& d : devices) if (d.serial == selectedSerial) { stillPresent = true; break; }
            if (selectedSerial.empty() || !stillPresent) {
                selectedSerial = devices.empty() ? "" : devices.front().serial;
                if (!selectedSerial.empty()) connectedSince = std::chrono::steady_clock::now();
            }
        }
        if (!selectedSerial.empty()) SelectDevice(selectedSerial);
        refreshing = false;
    }).detach();
}

// -------------------------------------------------------------------------- //
void App::Render(HWND hwnd) {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->Pos);
    ImGui::SetNextWindowSize(vp->Size);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::Begin("##root", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings);

    RenderTitlebar(hwnd);

    ImGui::BeginChild("##body", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar);
    RenderSidebar();
    ImGui::SameLine(0, 0);
    ImGui::BeginChild("##content", ImVec2(0, 0), false);
    RenderContent();
    ImGui::EndChild();
    ImGui::EndChild();

    ImGui::End();
    ImGui::PopStyleVar(2);
}

// -------------------------------------------------------------------------- //
void App::RenderTitlebar(HWND hwnd) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 winPos = ImGui::GetWindowPos();
    float w = ImGui::GetWindowWidth();
    float h = Theme::TitlebarHeight;

    ImVec2 p0 = winPos;
    ImVec2 p1 = ImVec2(winPos.x + w, winPos.y + h);
    dl->AddRectFilled(p0, p1, Theme::BgTitlebar);
    dl->AddLine(ImVec2(p0.x, p1.y), p1, Theme::BorderSoft, 1.0f);

    ImGui::BeginChild("##titlebar", ImVec2(w, h), false, ImGuiWindowFlags_NoScrollbar);

    // App icon: small rounded gradient square with a stylized link glyph.
    ImVec2 iconP0 = ImVec2(winPos.x + 18, winPos.y + h * 0.5f - 11);
    ImVec2 iconP1 = ImVec2(iconP0.x + 22, iconP0.y + 22);
    dl->AddRectFilledMultiColor(iconP0, iconP1,
        Theme::Accent, Theme::AccentHover, Theme::AccentHover, Theme::Accent);
    ImVec2 iconCenter = ImVec2((iconP0.x + iconP1.x) * 0.5f, (iconP0.y + iconP1.y) * 0.5f);
    Icons::Draw(dl, Icons::Type::Link, iconCenter, 13.0f, IM_COL32(255, 255, 255, 235), 1.8f);

    ImGui::SetCursorScreenPos(ImVec2(winPos.x + 50, winPos.y + h * 0.5f - 9));
    ImGui::PushFont(fonts.uiBold);
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Theme::TextPrimary),
                        "ADB Connect - LDPlayer Tool v1.0.0");
    ImGui::PopFont();

    // Window control buttons: minimize / maximize / close.
    const float btnW = 46.0f, btnH = h;
    float rightX = winPos.x + w;

    auto drawBtn = [&](const char* id, float xRight, ImU32 hoverCol, Icons::Type icon) -> bool {
        ImVec2 bp0 = ImVec2(xRight - btnW, winPos.y);
        ImVec2 bp1 = ImVec2(xRight, winPos.y + btnH);
        ImGui::SetCursorScreenPos(bp0);
        ImGui::InvisibleButton(id, ImVec2(btnW, btnH));
        bool hovered = ImGui::IsItemHovered();
        bool clicked = ImGui::IsItemClicked();
        if (hovered) dl->AddRectFilled(bp0, bp1, hoverCol);
        ImVec2 center = ImVec2((bp0.x + bp1.x) * 0.5f, (bp0.y + bp1.y) * 0.5f);
        ImU32 iconColor = (hovered && icon == Icons::Type::Close) ? IM_COL32(255,255,255,255) : Theme::TextSecondary;
        Icons::Draw(dl, icon, center, 11.0f, iconColor, 1.4f);
        return clicked;
    };

    if (drawBtn("##close", rightX, Theme::Danger, Icons::Type::Close)) {
        PostMessageW(hwnd, WM_CLOSE, 0, 0);
    }
    rightX -= btnW;
    if (drawBtn("##max", rightX, Theme::BgHover, Icons::Type::Maximize)) {
        if (IsZoomed(hwnd)) ShowWindow(hwnd, SW_RESTORE);
        else ShowWindow(hwnd, SW_MAXIMIZE);
    }
    rightX -= btnW;
    if (drawBtn("##min", rightX, Theme::BgHover, Icons::Type::Minimize)) {
        ShowWindow(hwnd, SW_MINIMIZE);
    }

    // Report the draggable region (everything left of the buttons) to WndProc.
    float dragRightEdge = rightX;
    g_TitlebarDragRect = { winPos.x, winPos.y, dragRightEdge, winPos.y + h, true };

    ImGui::EndChild();
    ImGui::SetCursorScreenPos(ImVec2(winPos.x, winPos.y + h));
}

// -------------------------------------------------------------------------- //
void App::RenderSidebar() {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(Theme::BgSidebar));
    ImGui::BeginChild("##sidebar", ImVec2(Theme::SidebarWidth, 0), false);

    struct NavItem { ViewId id; Icons::Type icon; const char* label; };
    static const NavItem items[] = {
        { ViewId::Connect,  Icons::Type::Link,     "Kết nối" },
        { ViewId::Devices,  Icons::Type::Phone,     "Thiết bị" },
        { ViewId::Control,  Icons::Type::Gamepad,   "Điều khiển" },
        { ViewId::Apps,     Icons::Type::Grid,      "Ứng dụng" },
        { ViewId::Files,    Icons::Type::Folder,    "Tập tin" },
        { ViewId::Shell,    Icons::Type::Terminal,  "ADB Shell" },
        { ViewId::Settings, Icons::Type::Gear,      "Cài đặt" },
        { ViewId::About,    Icons::Type::Info,      "Giới thiệu" },
    };

    ImGui::Dummy(ImVec2(0, 10));
    ImDrawList* dl = ImGui::GetWindowDrawList();

    for (auto& item : items) {
        ImVec2 cursor = ImGui::GetCursorScreenPos();
        float rowW = Theme::SidebarWidth - 16;
        float rowH = 40.0f;
        ImGui::SetCursorScreenPos(ImVec2(cursor.x + 8, cursor.y));
        ImGui::PushID((int)item.id);
        ImGui::InvisibleButton("navrow", ImVec2(rowW, rowH));
        bool hovered = ImGui::IsItemHovered();
        bool active = (currentView == item.id);
        if (ImGui::IsItemClicked()) currentView = item.id;
        ImGui::PopID();

        ImVec2 p0 = ImVec2(cursor.x + 8, cursor.y);
        ImVec2 p1 = ImVec2(p0.x + rowW, p0.y + rowH);
        if (active) {
            dl->AddRectFilled(p0, p1, Theme::AccentSoft, Theme::RoundingSmall);
            dl->AddRectFilled(ImVec2(p0.x, p0.y + 6), ImVec2(p0.x + 3, p1.y - 6), Theme::Accent, 2.0f);
        } else if (hovered) {
            dl->AddRectFilled(p0, p1, Theme::BgHover, Theme::RoundingSmall);
        }

        ImU32 fg = active ? Theme::Accent : (hovered ? Theme::TextPrimary : Theme::TextSecondary);
        ImVec2 iconCenter = ImVec2(p0.x + 22, p0.y + rowH * 0.5f);
        Icons::Draw(dl, item.icon, iconCenter, 16.0f, fg, 1.5f);

        ImGui::PushFont(fonts.ui);
        ImVec2 labelSize = ImGui::CalcTextSize(item.label);
        dl->AddText(ImGui::GetFont(), ImGui::GetFontSize(),
                    ImVec2(p0.x + 42, p0.y + rowH * 0.5f - labelSize.y * 0.5f), fg, item.label);
        ImGui::PopFont();

        ImGui::SetCursorScreenPos(ImVec2(cursor.x, cursor.y + rowH + 2));
    }

    // Bottom status block (ADB Status + version), pinned to the bottom.
    float bottomBlockH = 96.0f;
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImGui::Dummy(ImVec2(0, avail.y > bottomBlockH ? avail.y - bottomBlockH : 0));

    ImGui::Dummy(ImVec2(0, 4));
    ImGui::SetCursorPosX(20);
    ImGui::PushFont(fonts.ui);
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Theme::TextMuted), "ADB Status");
    ImGui::PopFont();

    ImGui::SetCursorPosX(20);
    ImVec2 dotPos = ImGui::GetCursorScreenPos();
    bool online = adb.IsAdbAvailable();
    dl->AddCircleFilled(ImVec2(dotPos.x + 5, dotPos.y + 9), 4.5f, online ? Theme::Success : Theme::Danger);
    ImGui::Dummy(ImVec2(16, 0));
    ImGui::SameLine();
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(online ? Theme::Success : Theme::Danger),
                        online ? "Online" : "Offline");

    ImGui::Dummy(ImVec2(0, 10));
    ImGui::SetCursorPosX(20);
    ImGui::PushFont(fonts.ui);
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Theme::TextMuted), "Phiên bản ADB");
    ImGui::SetCursorPosX(20);
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Theme::TextSecondary), "1.0.41");
    ImGui::PopFont();

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

// -------------------------------------------------------------------------- //
void App::RenderContent() {
    switch (currentView) {
        case ViewId::Connect:  Views::RenderConnect(*this);  break;
        case ViewId::Devices:  Views::RenderDevices(*this);  break;
        case ViewId::Control:  Views::RenderControl(*this);  break;
        case ViewId::Apps:     Views::RenderApps(*this);     break;
        case ViewId::Files:    Views::RenderFiles(*this);    break;
        case ViewId::Shell:    Views::RenderShell(*this);    break;
        case ViewId::Settings: Views::RenderSettings(*this); break;
        case ViewId::About:    Views::RenderAbout(*this);    break;
    }
}
