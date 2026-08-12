#include "widgets.h"
#include "theme.h"
#include <cstdio>

namespace Widgets {

void SectionHeader(const char* label) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(Theme::TextHeading));
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0, 6));
}

bool BeginCard(const char* id, ImVec2 size, const char* title) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(Theme::BgCard));
    ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertU32ToFloat4(Theme::Border));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, Theme::Rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(18, 16));
    bool open = ImGui::BeginChild(id, size, true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    if (title) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(Theme::TextPrimary));
        ImGui::TextUnformatted(title);
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0, 8));
    }
    return open;
}

void EndCard() {
    ImGui::EndChild();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}

bool StyledButton(const char* label, ImVec2 size, ButtonStyle style, Icons::Type icon, bool showIcon) {
    ImU32 bg, bgHover, bgActive, fg;
    switch (style) {
        case ButtonStyle::Primary:
            bg = Theme::Accent; bgHover = Theme::AccentHover; bgActive = Theme::AccentActive;
            fg = IM_COL32(255,255,255,255);
            break;
        case ButtonStyle::Danger:
            bg = Theme::Danger; bgHover = IM_COL32(248, 90, 90, 255); bgActive = IM_COL32(210, 55, 55, 255);
            fg = IM_COL32(255,255,255,255);
            break;
        case ButtonStyle::Ghost:
            bg = IM_COL32(0,0,0,0); bgHover = Theme::BgHover; bgActive = Theme::BgHover;
            fg = Theme::TextSecondary;
            break;
        case ButtonStyle::Outline:
        default:
            bg = Theme::BgCardAlt; bgHover = Theme::BgHover; bgActive = Theme::BgHover;
            fg = Theme::TextPrimary;
            break;
    }

    ImGui::PushID(label);
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    if (size.x <= 0) size.x = ImGui::CalcTextSize(label).x + 32.0f + (showIcon ? 22.0f : 0.0f);
    if (size.y <= 0) size.y = 36.0f;

    ImGui::InvisibleButton("##btn", size);
    bool hovered = ImGui::IsItemHovered();
    bool active  = ImGui::IsItemActive();
    bool clicked = ImGui::IsItemClicked();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 col = active ? bgActive : (hovered ? bgHover : bg);
    ImVec2 p0 = cursor, p1 = ImVec2(cursor.x + size.x, cursor.y + size.y);
    if (style == ButtonStyle::Outline) {
        dl->AddRectFilled(p0, p1, col, Theme::RoundingSmall);
        dl->AddRect(p0, p1, Theme::Border, Theme::RoundingSmall);
    } else if (col != 0) {
        dl->AddRectFilled(p0, p1, col, Theme::RoundingSmall);
    }

    float textStartX = p0.x + (size.x - ImGui::CalcTextSize(label).x) * 0.5f;
    if (showIcon) {
        ImVec2 iconCenter = ImVec2(p0.x + 20, (p0.y + p1.y) * 0.5f);
        Icons::Draw(dl, icon, iconCenter, 14.0f, fg, 1.6f);
        textStartX = p0.x + 34;
    }
    ImVec2 textSize = ImGui::CalcTextSize(label);
    dl->AddText(ImVec2(textStartX, (p0.y + p1.y) * 0.5f - textSize.y * 0.5f), fg, label);

    ImGui::PopID();
    return clicked;
}

void StatusDot(ImU32 color, const char* text, float dotRadius) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float textH = ImGui::GetTextLineHeight();
    dl->AddCircleFilled(ImVec2(pos.x + dotRadius, pos.y + textH * 0.5f), dotRadius, color);
    ImGui::Dummy(ImVec2(dotRadius * 2 + 8, 0));
    ImGui::SameLine(0, 0);
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(color), "%s", text);
}

void Badge(const char* text, ImU32 bg, ImU32 fg) {
    ImVec2 textSize = ImGui::CalcTextSize(text);
    ImVec2 pad = ImVec2(10, 4);
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 size = ImVec2(textSize.x + pad.x * 2, textSize.y + pad.y * 2);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), bg, Theme::RoundingPill);
    dl->AddText(ImVec2(pos.x + pad.x, pos.y + pad.y), fg, text);
    ImGui::Dummy(size);
}

void InfoRow(const char* label, const std::string& value, float labelWidth) {
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Theme::TextSecondary), "%s", label);
    ImGui::SameLine(labelWidth);
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Theme::TextPrimary), "%s", value.c_str());
}

bool RadioOption(const char* id, bool selected, const char* title, const char* description) {
    ImGui::PushID(id);
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x;
    float h = 54.0f;

    ImGui::InvisibleButton("##opt", ImVec2(w, h));
    bool clicked = ImGui::IsItemClicked();
    bool hovered = ImGui::IsItemHovered();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (hovered && !selected) {
        dl->AddRectFilled(cursor, ImVec2(cursor.x + w, cursor.y + h), Theme::BgHover, Theme::RoundingSmall);
    }

    // Radio circle
    ImVec2 radioCenter = ImVec2(cursor.x + 12, cursor.y + 14);
    dl->AddCircle(radioCenter, 8.0f, selected ? Theme::Accent : Theme::TextMuted, 0, 1.6f);
    if (selected) dl->AddCircleFilled(radioCenter, 4.0f, Theme::Accent);

    dl->AddText(ImVec2(cursor.x + 32, cursor.y + 2),
                selected ? Theme::TextPrimary : Theme::TextPrimary, title);

    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(Theme::TextMuted));
    dl->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 0.92f,
                ImVec2(cursor.x + 32, cursor.y + 24), Theme::TextMuted, description);
    ImGui::PopStyleColor();

    ImGui::PopID();
    return clicked;
}

bool QuickActionTile(const char* id, Icons::Type icon, const char* label, ImVec2 size) {
    ImGui::PushID(id);
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##tile", size);
    bool hovered = ImGui::IsItemHovered();
    bool clicked = ImGui::IsItemClicked();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p0 = cursor, p1 = ImVec2(cursor.x + size.x, cursor.y + size.y);
    dl->AddRectFilled(p0, p1, hovered ? Theme::BgHover : Theme::BgCardAlt, Theme::RoundingSmall);
    dl->AddRect(p0, p1, Theme::BorderSoft, Theme::RoundingSmall);

    ImVec2 iconCenter = ImVec2((p0.x + p1.x) * 0.5f, p0.y + size.y * 0.38f);
    Icons::Draw(dl, icon, iconCenter, 22.0f, hovered ? Theme::Accent : Theme::TextSecondary, 1.7f);

    ImVec2 textSize = ImGui::CalcTextSize(label);
    dl->AddText(ImVec2((p0.x + p1.x) * 0.5f - textSize.x * 0.5f, p0.y + size.y * 0.68f),
                Theme::TextSecondary, label);

    ImGui::PopID();
    return clicked;
}

} // namespace Widgets
