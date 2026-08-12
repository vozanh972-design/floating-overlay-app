#include "views.h"
#include "app.h"
#include "widgets.h"
#include "theme.h"
#include "icons.h"
#include "core/file_dialog.h"
#include <thread>
#include <cstring>

namespace Views {

static void PlaceholderCard(const char* title, const char* description, Icons::Type icon) {
    ImGui::Dummy(ImVec2(0, 16));
    ImGui::Indent(24);
    float w = ImGui::GetContentRegionAvail().x - 24;

    if (Widgets::BeginCard("##placeholder", ImVec2(w, 260))) {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        ImVec2 center = ImVec2(ImGui::GetCursorScreenPos().x + avail.x * 0.5f,
                                ImGui::GetCursorScreenPos().y + avail.y * 0.35f);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddCircleFilled(center, 34.0f, Theme::BgCardAlt);
        Icons::Draw(dl, icon, center, 30.0f, Theme::Accent, 1.8f);

        ImGui::Dummy(ImVec2(0, 90));
        ImVec2 titleSize = ImGui::CalcTextSize(title);
        ImGui::SetCursorPosX((avail.x - titleSize.x) * 0.5f);
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Theme::TextPrimary), "%s", title);

        ImVec2 descSize = ImGui::CalcTextSize(description);
        ImGui::SetCursorPosX((avail.x - descSize.x) * 0.5f);
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Theme::TextMuted), "%s", description);
    }
    Widgets::EndCard();
    ImGui::Unindent(24);
}

// ------------------------------------------------------------------------ //
void RenderDevices(App& app) {
    ImGui::Dummy(ImVec2(0, 16));
    ImGui::Indent(24);
    float w = ImGui::GetContentRegionAvail().x - 24;
    Widgets::SectionHeader("QUẢN LÝ THIẾT BỊ");

    std::lock_guard<std::mutex> lock(app.devicesMutex);
    if (Widgets::BeginCard("##dev_manage", ImVec2(w, 0))) {
        for (auto& d : app.devices) {
            ImGui::PushID(d.serial.c_str());
            bool isSel = (d.serial == app.selectedSerial);
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Theme::TextPrimary), "%s", d.displayName.c_str());
            ImGui::SameLine();
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Theme::TextMuted), "(%s)", d.serial.c_str());
            ImGui::SameLine();
            Widgets::StatusDot(d.status == core::DeviceStatus::Device ? Theme::Success : Theme::Danger,
                                core::DeviceStatusLabel(d.status), 5.0f);

            if (Widgets::StyledButton(isSel ? "Đã chọn" : "Chọn", ImVec2(90, 30),
                    isSel ? Widgets::ButtonStyle::Primary : Widgets::ButtonStyle::Outline)) {
                app.SelectDevice(d.serial);
            }
            ImGui::SameLine();
            if (Widgets::StyledButton("Reboot", ImVec2(90, 30), Widgets::ButtonStyle::Outline)) {
                std::thread([&app, s = d.serial]() { app.adb.Reboot(s); }).detach();
            }
            ImGui::SameLine();
            if (Widgets::StyledButton("Ngắt kết nối", ImVec2(120, 30), Widgets::ButtonStyle::Danger)) {
                std::thread([&app, s = d.serial]() { app.adb.Disconnect(s); app.RefreshDevicesAsync(); }).detach();
            }
            ImGui::Dummy(ImVec2(0, 10));
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, 10));
            ImGui::PopID();
        }
        if (app.devices.empty()) {
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Theme::TextMuted),
                "Chưa có thiết bị nào. Vào tab Kết nối để quét LDPlayer.");
        }
    }
    Widgets::EndCard();
    ImGui::Unindent(24);
}

void RenderControl(App& app) {
    (void)app;
    PlaceholderCard("Điều khiển thiết bị", "Điều khiển màn hình trực tiếp (mirror + input) đang được phát triển.",
                     Icons::Type::Gamepad);
}

void RenderApps(App& app) {
    ImGui::Dummy(ImVec2(0, 16));
    ImGui::Indent(24);
    float w = ImGui::GetContentRegionAvail().x - 24;
    Widgets::SectionHeader("QUẢN LÝ ỨNG DỤNG (APK)");

    core::Device* selected = app.GetSelectedDevice();

    if (Widgets::BeginCard("##apk_manager", ImVec2(w, 220))) {
        std::string pathLabel = app.selectedApkPath.empty() ? "Chưa chọn file APK" : core::WideToUtf8Str(app.selectedApkPath);
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Theme::TextSecondary), "%s", pathLabel.c_str());
        ImGui::Dummy(ImVec2(0, 8));

        if (Widgets::StyledButton("Chọn APK...", ImVec2(140, 34), Widgets::ButtonStyle::Outline)) {
            auto path = core::OpenFileDialog(L"APK files\0*.apk\0All files\0*.*\0", L"Chọn file APK");
            if (path) app.selectedApkPath = *path;
        }
        ImGui::SameLine();
        if (Widgets::StyledButton("Cài đặt", ImVec2(120, 34), Widgets::ButtonStyle::Primary)) {
            if (selected && !app.selectedApkPath.empty()) {
                app.apkStatusMsg = "Đang cài đặt...";
                std::thread([&app, serial = selected->serial, path = app.selectedApkPath]() {
                    auto r = app.adb.InstallApk(serial, path, false);
                    app.apkStatusMsg = r.Success() ? "Cài đặt thành công." : (r.stdErr.empty() ? r.launchError : r.stdErr);
                }).detach();
            } else {
                app.apkStatusMsg = "Vui lòng chọn thiết bị và file APK trước.";
            }
        }
        ImGui::SameLine();
        if (Widgets::StyledButton("Cài lại (reinstall)", ImVec2(170, 34), Widgets::ButtonStyle::Outline)) {
            if (selected && !app.selectedApkPath.empty()) {
                app.apkStatusMsg = "Đang cài đặt lại...";
                std::thread([&app, serial = selected->serial, path = app.selectedApkPath]() {
                    auto r = app.adb.InstallApk(serial, path, true);
                    app.apkStatusMsg = r.Success() ? "Cài đặt lại thành công." : (r.stdErr.empty() ? r.launchError : r.stdErr);
                }).detach();
            }
        }

        ImGui::Dummy(ImVec2(0, 16));
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Theme::TextSecondary), "Gỡ cài đặt package:");
        ImGui::Dummy(ImVec2(0, 4));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::ColorConvertU32ToFloat4(Theme::BgInput));
        ImGui::SetNextItemWidth(320);
        ImGui::InputText("##pkg", app.uninstallPackageBuffer, IM_ARRAYSIZE(app.uninstallPackageBuffer));
        ImGui::PopStyleColor();
        ImGui::SameLine();
        if (Widgets::StyledButton("Gỡ cài đặt", ImVec2(120, 34), Widgets::ButtonStyle::Danger)) {
            if (selected) {
                app.apkStatusMsg = "Đang gỡ cài đặt...";
                std::thread([&app, serial = selected->serial, pkg = std::string(app.uninstallPackageBuffer)]() {
                    auto r = app.adb.UninstallPackage(serial, pkg);
                    app.apkStatusMsg = r.Success() ? "Gỡ cài đặt thành công." : (r.stdErr.empty() ? r.launchError : r.stdErr);
                }).detach();
            }
        }

        if (!app.apkStatusMsg.empty()) {
            ImGui::Dummy(ImVec2(0, 10));
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Theme::TextMuted), "%s", app.apkStatusMsg.c_str());
        }
    }
    Widgets::EndCard();
    ImGui::Unindent(24);
}

void RenderFiles(App& app) {
    (void)app;
    PlaceholderCard("Quản lý tập tin", "Trình duyệt file hai chiều (push/pull) đang được phát triển.",
                     Icons::Type::Folder);
}

void RenderShell(App& app) {
    ImGui::Dummy(ImVec2(0, 16));
    ImGui::Indent(24);
    float w = ImGui::GetContentRegionAvail().x - 24;
    Widgets::SectionHeader("ADB SHELL");

    core::Device* selected = app.GetSelectedDevice();

    if (Widgets::BeginCard("##shell_console", ImVec2(w, 460))) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(Theme::BgInput));
        ImGui::BeginChild("##shell_output", ImVec2(0, 360), true);
        ImGui::PushFont(app.fonts.mono);
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Theme::Success), "%s", app.shellOutput.c_str());
        ImGui::PopFont();
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 4.0f) ImGui::SetScrollHereY(1.0f);
        ImGui::EndChild();
        ImGui::PopStyleColor();

        ImGui::Dummy(ImVec2(0, 8));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::ColorConvertU32ToFloat4(Theme::BgInput));
        ImGui::SetNextItemWidth(w - 230);
        bool enter = ImGui::InputText("##shellin", app.shellInput, IM_ARRAYSIZE(app.shellInput),
                                       ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::PopStyleColor();
        ImGui::SameLine();
        bool execClicked = Widgets::StyledButton("Execute", ImVec2(100, 34), Widgets::ButtonStyle::Primary);
        ImGui::SameLine();
        bool clearClicked = Widgets::StyledButton("Clear", ImVec2(100, 34), Widgets::ButtonStyle::Outline);

        if (clearClicked) app.shellOutput.clear();

        if ((enter || execClicked) && app.shellInput[0] != '\0') {
            if (!selected) {
                app.shellOutput += "[error] Chưa chọn thiết bị.\n";
            } else {
                std::string cmd = app.shellInput;
                app.shellOutput += "\n$ " + cmd + "\n";
                app.shellInput[0] = '\0';
                std::thread([&app, serial = selected->serial, cmd]() {
                    auto r = app.adb.ShellSimple(serial, cmd);
                    if (!r.stdOut.empty()) app.shellOutput += r.stdOut;
                    if (!r.stdErr.empty()) app.shellOutput += r.stdErr;
                    if (!r.launched) app.shellOutput += "[error] " + r.launchError + "\n";
                }).detach();
            }
        }
    }
    Widgets::EndCard();
    ImGui::Unindent(24);
}

void RenderSettings(App& app) {
    ImGui::Dummy(ImVec2(0, 16));
    ImGui::Indent(24);
    float w = ImGui::GetContentRegionAvail().x - 24;
    Widgets::SectionHeader("CÀI ĐẶT");

    if (Widgets::BeginCard("##settings_card", ImVec2(w, 220))) {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Theme::TextPrimary), "Đường dẫn ADB (adb.exe)");
        ImGui::Dummy(ImVec2(0, 8));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::ColorConvertU32ToFloat4(Theme::BgInput));
        ImGui::SetNextItemWidth(w - 300);
        ImGui::InputText("##adbpath", app.adbPathBuffer, IM_ARRAYSIZE(app.adbPathBuffer));
        ImGui::PopStyleColor();
        ImGui::SameLine();
        if (Widgets::StyledButton("Duyệt...", ImVec2(100, 34), Widgets::ButtonStyle::Outline)) {
            auto path = core::OpenFileDialog(L"adb.exe\0adb.exe\0All files\0*.*\0", L"Chọn adb.exe");
            if (path) {
                std::string utf8 = core::WideToUtf8Str(*path);
                strncpy_s(app.adbPathBuffer, utf8.c_str(), sizeof(app.adbPathBuffer) - 1);
            }
        }
        ImGui::SameLine();
        if (Widgets::StyledButton("Tự động dò", ImVec2(110, 34), Widgets::ButtonStyle::Outline)) {
            auto found = core::AdbManager::FindAdbExecutable();
            if (found) {
                std::string utf8 = core::WideToUtf8Str(*found);
                strncpy_s(app.adbPathBuffer, utf8.c_str(), sizeof(app.adbPathBuffer) - 1);
                app.adbPathStatusMsg = "Đã tìm thấy: " + utf8;
            } else {
                app.adbPathStatusMsg = "Không tìm thấy adb.exe tự động.";
            }
        }

        ImGui::Dummy(ImVec2(0, 14));
        if (Widgets::StyledButton("Lưu cài đặt", ImVec2(140, 36), Widgets::ButtonStyle::Primary)) {
            auto wpath = core::Utf8ToWide(app.adbPathBuffer);
            if (app.adb.SetAdbPath(wpath)) {
                app.adbPathStatusMsg = "Đã lưu đường dẫn ADB.";
                app.RefreshDevicesAsync();
            } else {
                app.adbPathStatusMsg = "Đường dẫn không hợp lệ.";
            }
        }

        if (!app.adbPathStatusMsg.empty()) {
            ImGui::Dummy(ImVec2(0, 10));
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Theme::TextMuted), "%s", app.adbPathStatusMsg.c_str());
        }
    }
    Widgets::EndCard();
    ImGui::Unindent(24);
}

void RenderAbout(App& app) {
    (void)app;
    PlaceholderCard("ADB Connect - LDPlayer Tool", "Phiên bản 1.0.0 - Ứng dụng C++ native (Win32 + Direct3D11 + Dear ImGui).",
                     Icons::Type::Info);
}

} // namespace Views
