#pragma once
#include "imgui.h"

// Color palette sampled to match the reference mockup: near-black navy
// background, slightly lighter card surfaces, indigo/purple accent,
// green for "connected" / online states, red for destructive actions.
namespace Theme {

inline constexpr ImU32 Col(int r, int g, int b, int a = 255) {
    return IM_COL32(r, g, b, a);
}

// Backgrounds
inline const ImU32 BgApp        = Col(10, 12, 18);      // outer window background
inline const ImU32 BgSidebar    = Col(13, 15, 22);       // sidebar background
inline const ImU32 BgTitlebar   = Col(10, 12, 18);
inline const ImU32 BgCard       = Col(19, 22, 31);       // card / panel background
inline const ImU32 BgCardAlt    = Col(23, 27, 38);       // slightly lighter nested surface
inline const ImU32 BgInput      = Col(15, 18, 26);
inline const ImU32 BgTableHead  = Col(16, 19, 27);
inline const ImU32 BgHover      = Col(28, 32, 44);

// Borders
inline const ImU32 Border       = Col(34, 39, 51);
inline const ImU32 BorderSoft   = Col(26, 30, 41);

// Text
inline const ImU32 TextPrimary  = Col(232, 235, 240);
inline const ImU32 TextSecondary= Col(148, 156, 173);
inline const ImU32 TextMuted    = Col(96, 103, 120);
inline const ImU32 TextHeading  = Col(140, 148, 165); // uppercase section headers

// Accent (indigo / purple gradient look)
inline const ImU32 Accent       = Col(91, 110, 255);
inline const ImU32 AccentHover  = Col(110, 128, 255);
inline const ImU32 AccentActive = Col(75, 92, 230);
inline const ImU32 AccentSoft   = Col(91, 110, 255, 40);  // active-nav background wash
inline const ImU32 AccentSoft2  = Col(91, 110, 255, 90);  // badge background

// Status
inline const ImU32 Success      = Col(34, 197, 94);
inline const ImU32 SuccessSoft  = Col(34, 197, 94, 35);
inline const ImU32 Danger       = Col(239, 68, 68);
inline const ImU32 DangerSoft   = Col(239, 68, 68, 35);
inline const ImU32 Warning      = Col(245, 158, 11);

inline constexpr float Rounding      = 10.0f;
inline constexpr float RoundingSmall = 6.0f;
inline constexpr float RoundingPill  = 999.0f;
inline constexpr float TitlebarHeight = 44.0f;
inline constexpr float SidebarWidth   = 208.0f;

void ApplyStyle();          // push global ImGuiStyle values (spacing, rounding, colors)
ImFont* LoadUIFont(float sizePx);      // regular UI font (Vietnamese glyph range)
ImFont* LoadMonoFont(float sizePx);    // monospace font for shell/console text
ImFont* LoadBoldFont(float sizePx);    // semi-bold for headings

} // namespace Theme
