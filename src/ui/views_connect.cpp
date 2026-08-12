#include "views.h"
#include "app.h"
#include "widgets.h"
#include "theme.h"
#include "icons.h"
#include <cstdio>
#include <chrono>
#include <thread>

namespace Views {

static std::string FormatElapsed(std::chrono::steady_clock::time_point since) {
    if (since.time_since_epoch().count() == 0) return "--:--:--";
    auto secs = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - since).count();
    long h = secs / 3600, m = (secs % 3600) / 60, s = secs % 60;
    char buf[16];
    snprintf(buf, sizeof(buf), "%02ld:%02ld:%02ld", h, m, s);
    return buf;
}

static std::string ValueOr(const std::string& v, const char* fallback = "--") {
    return v.empty() ? fallback : v;
}

// ------------------------------------------------------------------------ //
void RenderConnect(App& app) {
    ImGui::Dummy(ImVec2(0, 16));
    ImGui::Indent(24);
    ImGui::PushItemWidth(-1);

    float fullW = ImGui::GetContentRegionAvail().x - 24;

    Widgets::SectionHeader("KẾT NỐI ADB");

    core::Device* selected = app.GetSelectedDevice();
    bool isConnected = selected && selected->status == core::DeviceStatus::Device;

    // ---- Row 1: connection method card + status card ------------------- //
    float leftW  = fullW * 0.34f;
    float rightW = fullW - leftW - 16.0f;
    float rowH = 210.0f;

    if (Widgets::BeginCard("##method_card", ImVec2(leftW, rowH))) {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Theme::TextPrimary), "Phương thức kết nối");
        ImGui::Dummy(ImVec2(0, 10));

        if (Widgets::RadioOption("auto", app.connectMode == 0, "Kết nối tự động (Khuyến nghị)",
                                  "Tự động tìm và kết nối LDPlayer")) app.connectMode = 0;
        ImGui::Dummy(ImVec2(0, 6));
        if (Widgets::RadioOption("manual", app.connectMode == 1, "Kết nối thủ công",
                                  "Nhập địa chỉ IP và cổng ADB")) app.connectMode = 1;
        ImGui::Dummy(ImVec2(0, 6));
        if (Widgets::RadioOption("wifi", app.connectMode == 2, "Kết nối qua WiFi",
                                  "Kết nối không dây qua WiFi")) app.connectMode = 2;

        if (app.connectMode == 1) {
            ImGui::Dummy(ImVec2(0, 8));
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::ColorConvertU32ToFloat4(Theme::BgInput));
            ImGui::InputText("##addr", app.manualAddress, IM_ARRAYSIZE(app.manualAddress));
            ImGui::PopStyleColor();
        }
    }
    Widgets::EndCard();

    ImGui::SameLine();

    if (Widgets::BeginCard("##status_card", ImVec2(rightW, rowH))) {
        float infoW = ImGui::GetContentRegionAvail().x - 210.0f;

        ImGui::BeginChild("##status_info", ImVec2(infoW, 0), false);
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Theme::TextPrimary), "Trạng thái kết nối");
        ImGui::Dummy(ImVec2(0, 10));

        Widgets::StatusDot(isConnected ? Theme::Success : Theme::Danger,
                            isConnected ? "Đã kết nối" : "Chưa kết nối", 6.0f);
        ImGui::Dummy(ImVec2(0, 12));

        Widgets::InfoRow("Thiết bị:", selected ? selected->displayName : std::string("--"));
        Widgets::InfoRow("Địa chỉ:", selected ? selected->serial : std::string("--"));
        Widgets::InfoRow("Phiên bản Android:",
            selected ? (ValueOr(selected->androidVersion) + (selected->abi.find("64") != std::string::npos ? " (64-bit)" : "")) : std::string("--"));
        Widgets::InfoRow("Độ phân giải:",
            selected ? (ValueOr(selected->resolution) + (selected->dpi.empty() ? "" : " (dpi " + selected->dpi + ")")) : std::string("--"));
        Widgets::InfoRow("Thời gian hoạt động:", isConnected ? FormatElapsed(app.connectedSince) : std::string("--:--:--"));
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("##status_actions", ImVec2(0, 0), false);
        ImGui::Dummy(ImVec2(0, 30));
        if (Widgets::StyledButton(isConnected ? "  Ngắt kết nối" : "  Kết nối", ImVec2(200, 40),
                                   Widgets::ButtonStyle::Primary,
                                   isConnected ? Icons::Type::Unlink : Icons::Type::Link, true)) {
            if (isConnected && selected) {
                app.adb.Disconnect(selected->serial);
            } else if (app.connectMode == 1) {
                app.adb.Connect(app.manualAddress);
            } else {
                app.adb.Connect("127.0.0.1:5555");
            }
            app.RefreshDevicesAsync();
        }
        ImGui::Dummy(ImVec2(0, 8));
        if (Widgets::StyledButton("  Khởi động lại ADB", ImVec2(200, 38), Widgets::ButtonStyle::Outline,
                                   Icons::Type::Refresh, true)) {
            std::thread([&app]() { app.adb.RestartServer(); app.RefreshDevicesAsync(); }).detach();
        }
        ImGui::Dummy(ImVec2(0, 8));
        if (Widgets::StyledButton("  Tìm lại thiết bị", ImVec2(200, 38), Widgets::ButtonStyle::Outline,
                                   Icons::Type::Search, true)) {
            app.RefreshDevicesAsync();
        }
        ImGui::EndChild();
    }
    Widgets::EndCard();

    ImGui::Dummy(ImVec2(0, 20));

    // ---- Device list table ------------------------------------------------ //
    Widgets::SectionHeader("DANH SÁCH THIẾT BỊ");

    {
        std::lock_guard<std::mutex> lock(app.devicesMutex);

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(Theme::BgCard));
        ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertU32ToFloat4(Theme::Border));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, Theme::Rounding);
        float tableH = 40.0f + (float)(app.devices.empty() ? 1 : app.devices.size()) * 52.0f + 20.0f;
        ImGui::BeginChild("##devtable_card", ImVec2(fullW, tableH), true);

        if (ImGui::BeginTable("devtable", 6,
                ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("Tên thiết bị", ImGuiTableColumnFlags_WidthStretch, 2.2f);
            ImGui::TableSetupColumn("Địa chỉ", ImGuiTableColumnFlags_WidthStretch, 1.6f);
            ImGui::TableSetupColumn("Trạng thái", ImGuiTableColumnFlags_WidthStretch, 1.4f);
            ImGui::TableSetupColumn("Phiên bản Android", ImGuiTableColumnFlags_WidthStretch, 1.6f);
            ImGui::TableSetupColumn("Độ phân giải", ImGuiTableColumnFlags_WidthStretch, 1.6f);
            ImGui::TableSetupColumn("Thao tác", ImGuiTableColumnFlags_WidthStretch, 1.4f);
            ImGui::TableHeadersRow();

            if (app.devices.empty()) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Theme::TextMuted),
                    "Chưa có thiết bị nào. Nhấn \"Tìm lại thiết bị\" để quét LDPlayer.");
            }

            for (auto& d : app.devices) {
                ImGui::TableNextRow(ImGuiTableRowFlags_None, 48.0f);
                bool isSel = (d.serial == app.selectedSerial);
                bool devConnected = (d.status == core::DeviceStatus::Device);

                ImGui::TableSetColumnIndex(0);
                ImDrawList* dl = ImGui::GetWindowDrawList();
                ImVec2 rowP0 = ImGui::GetCursorScreenPos();
                Icons::Draw(dl, Icons::Type::Phone, ImVec2(rowP0.x + 10, rowP0.y + 14), 16, Theme::Accent, 1.5f);
                ImGui::Dummy(ImVec2(24, 0));
                ImGui::SameLine();
                ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Theme::TextPrimary), "%s", d.displayName.c_str());
                if (isSel) {
                    ImGui::SameLine();
                    Widgets::Badge("Đang dùng", Theme::AccentSoft2, IM_COL32(255,255,255,255));
                }

                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Theme::TextSecondary), "%s", d.serial.c_str());

                ImGui::TableSetColumnIndex(2);
                Widgets::StatusDot(devConnected ? Theme::Success : Theme::Danger, core::DeviceStatusLabel(d.status), 5.0f);

                ImGui::TableSetColumnIndex(3);
                ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Theme::TextSecondary), "%s",
                                    ValueOr(d.androidVersion, "--").c_str());

                ImGui::TableSetColumnIndex(4);
                ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Theme::TextSecondary), "%s",
                                    ValueOr(d.resolution, "--").c_str());

                ImGui::TableSetColumnIndex(5);
                ImGui::PushID(d.serial.c_str());
                if (Widgets::StyledButton("Chọn", ImVec2(72, 30),
                        isSel ? Widgets::ButtonStyle::Primary : Widgets::ButtonStyle::Outline)) {
                    app.SelectDevice(d.serial);
                }
                ImGui::SameLine();
                ImGui::InvisibleButton("##kebab", ImVec2(28, 30));
                ImVec2 kp = ImGui::GetItemRectMin();
                ImVec2 ksize = ImGui::GetItemRectSize();
                Icons::Draw(ImGui::GetWindowDrawList(), Icons::Type::Dots,
                            ImVec2(kp.x + ksize.x * 0.5f, kp.y + ksize.y * 0.5f), 12, Theme::TextSecondary, 1.4f);
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
    }

    ImGui::Dummy(ImVec2(0, 20));

    // ---- Quick actions + device info -------------------------------------- //
    float qaW = fullW * 0.42f;
    float infoCardW = fullW - qaW - 16.0f;
    float lowerH = 300.0f;

    if (Widgets::BeginCard("##quick_actions", ImVec2(qaW, lowerH), "THAO TÁC NHANH")) {
        struct QA { Icons::Type icon; const char* label; };
        static const QA actions[] = {
            { Icons::Type::Camera,   "Chụp màn hình" },
            { Icons::Type::VideoRec, "Quay màn hình" },
            { Icons::Type::ApkBox,   "Cài APK" },
            { Icons::Type::Trash,    "Gỡ APK" },
            { Icons::Type::Refresh,  "Khởi động lại" },
            { Icons::Type::Power,    "Tắt thiết bị" },
            { Icons::Type::Rotate,   "Xoay màn hình" },
            { Icons::Type::Volume,   "Âm lượng" },
        };
        float avail = ImGui::GetContentRegionAvail().x;
        float gap = 12.0f;
        float tileW = (avail - gap * 3) / 4.0f;
        float tileH = 78.0f;

        for (int i = 0; i < 8; ++i) {
            char id[16]; snprintf(id, sizeof(id), "qa%d", i);
            if (Widgets::QuickActionTile(id, actions[i].icon, actions[i].label, ImVec2(tileW, tileH))) {
                if (selected && i == 4) {
                    std::thread([&app, s = selected->serial]() { app.adb.Reboot(s); }).detach();
                }
            }
            if (i % 4 != 3) ImGui::SameLine(0, gap);
        }
    }
    Widgets::EndCard();

    ImGui::SameLine();

    if (Widgets::BeginCard("##device_info", ImVec2(infoCardW, lowerH), "THÔNG TIN THIẾT BỊ")) {
        float infoColW = ImGui::GetContentRegionAvail().x - 210.0f;
        ImGui::BeginChild("##dev_info_rows", ImVec2(infoColW, 0), false);
        Widgets::InfoRow("Nhà sản xuất:", selected ? ValueOr(selected->manufacturer) : "--", 120.0f);
        Widgets::InfoRow("Model:", selected ? ValueOr(selected->model) : "--", 120.0f);
        Widgets::InfoRow("CPU:", selected ? ValueOr(selected->cpu, "--") : "--", 120.0f);
        Widgets::InfoRow("RAM:", selected ? ValueOr(selected->ramTotal) : "--", 120.0f);
        Widgets::InfoRow("Bộ nhớ trong:", selected ? ValueOr(selected->storageTotal) : "--", 120.0f);
        Widgets::InfoRow("Pin:", selected ? ValueOr(selected->battery) : "--", 120.0f);
        Widgets::InfoRow("Root:", selected ? (selected->isRooted ? "Có" : "Không") : "--", 120.0f);
        Widgets::InfoRow("ABI:", selected ? ValueOr(selected->abi) : "--", 120.0f);
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("##dev_preview", ImVec2(0, 0), false);
        ImVec2 p0 = ImGui::GetCursorScreenPos();
        ImVec2 avail = ImGui::GetContentRegionAvail();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p1 = ImVec2(p0.x + avail.x, p0.y + avail.y);
        dl->AddRectFilled(p0, p1, Theme::BgInput, Theme::RoundingSmall);
        dl->AddRect(p0, p1, Theme::BorderSoft, Theme::RoundingSmall);
        const char* hint = "Xem trước\nmàn hình";
        ImVec2 hintSize = ImGui::CalcTextSize(hint);
        dl->AddText(ImVec2((p0.x + p1.x) * 0.5f - hintSize.x * 0.5f, (p0.y + p1.y) * 0.5f - hintSize.y * 0.5f),
                    Theme::TextMuted, hint);
        ImGui::EndChild();
    }
    Widgets::EndCard();

    ImGui::Dummy(ImVec2(0, 20));

    // ---- ADB shell quick bar ------------------------------------------------ //
    Widgets::SectionHeader("ADB SHELL NHANH");
    if (Widgets::BeginCard("##shell_bar", ImVec2(fullW, 56))) {
        ImGui::PushFont(app.fonts.mono);
        std::string prompt = (selected ? selected->displayName : std::string("adb")) + ":/$";
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Theme::Success), "%s", prompt.c_str());
        ImGui::PopFont();

        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 170 + ImGui::GetCursorPosX());
        if (Widgets::StyledButton("  Mở ADB Shell", ImVec2(170, 38), Widgets::ButtonStyle::Primary,
                                   Icons::Type::Terminal, true)) {
            app.currentView = ViewId::Shell;
        }
    }
    Widgets::EndCard();

    ImGui::Dummy(ImVec2(0, 20));
    ImGui::PopItemWidth();
    ImGui::Unindent(24);
}

} // namespace Views
