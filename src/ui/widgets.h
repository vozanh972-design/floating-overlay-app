#pragma once
#include "imgui.h"
#include "icons.h"
#include <string>

namespace Widgets {

// Uppercase, muted section header used above each card group (e.g. "KẾT NỐI ADB").
void SectionHeader(const char* label);

// A rounded, bordered card surface. Use like:
//   Widgets::BeginCard("id", ImVec2(0, 220));
//   ... contents ...
//   Widgets::EndCard();
bool BeginCard(const char* id, ImVec2 size, const char* title = nullptr);
void EndCard();

enum class ButtonStyle { Primary, Outline, Danger, Ghost };

// A full-width-ish styled button with optional leading icon. Returns true when clicked.
bool StyledButton(const char* label, ImVec2 size, ButtonStyle style = ButtonStyle::Outline,
                   Icons::Type icon = Icons::Type::Link, bool showIcon = false);

// Colored status dot + text, e.g. green dot + "Đã kết nối".
void StatusDot(ImU32 color, const char* text, float dotRadius = 5.0f);

// Small rounded pill badge, e.g. "Đang dùng".
void Badge(const char* text, ImU32 bg, ImU32 fg);

// A label/value row used in info panels ("Model:      LDPlayer").
void InfoRow(const char* label, const std::string& value, float labelWidth = 130.0f);

// Radio-style selectable option block with title + description (used on the
// Connect screen's "Phương thức kết nối" card).
bool RadioOption(const char* id, bool selected, const char* title, const char* description);

// Square icon "quick action" tile: icon on top, label below, used in the
// "Thao tác nhanh" grid.
bool QuickActionTile(const char* id, Icons::Type icon, const char* label, ImVec2 size);

} // namespace Widgets
