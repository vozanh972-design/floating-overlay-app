#include "theme.h"
#include <windows.h>
#include <shlobj.h>
#include <string>

namespace Theme {

static std::wstring FontsDir() {
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_FONTS, nullptr, 0, path))) {
        return std::wstring(path);
    }
    return L"C:\\Windows\\Fonts";
}

static ImFont* LoadFontFile(const wchar_t* fileName, float sizePx, const ImWchar* ranges) {
    std::wstring full = FontsDir() + L"\\" + fileName;

    // Convert wide path to UTF-8 for ImGui's AddFontFromFileTTF (ImGui expects
    // a narrow, UTF-8 path on this codepath).
    int len = WideCharToMultiByte(CP_UTF8, 0, full.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string utf8(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, full.c_str(), -1, utf8.data(), len, nullptr, nullptr);

    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig cfg;
    cfg.OversampleH = 2;
    cfg.OversampleV = 2;
    cfg.PixelSnapH = true;

    ImFont* font = io.Fonts->AddFontFromFileTTF(utf8.c_str(), sizePx, &cfg, ranges);
    return font; // may be null if the font file isn't present; caller falls back to default
}

ImFont* LoadUIFont(float sizePx) {
    ImGuiIO& io = ImGui::GetIO();
    static const ImWchar* ranges = io.Fonts->GetGlyphRangesVietnamese();
    // Segoe UI ships on every Windows install and covers Vietnamese diacritics.
    ImFont* f = LoadFontFile(L"segoeui.ttf", sizePx, ranges);
    if (!f) f = io.Fonts->AddFontDefault();
    return f;
}

ImFont* LoadBoldFont(float sizePx) {
    ImGuiIO& io = ImGui::GetIO();
    static const ImWchar* ranges = io.Fonts->GetGlyphRangesVietnamese();
    ImFont* f = LoadFontFile(L"segoeuib.ttf", sizePx, ranges);
    if (!f) f = LoadFontFile(L"segoeui.ttf", sizePx, ranges);
    if (!f) f = io.Fonts->AddFontDefault();
    return f;
}

ImFont* LoadMonoFont(float sizePx) {
    ImGuiIO& io = ImGui::GetIO();
    static const ImWchar* ranges = io.Fonts->GetGlyphRangesVietnamese();
    ImFont* f = LoadFontFile(L"consola.ttf", sizePx, ranges);
    if (!f) f = LoadFontFile(L"cascadiamono.ttf", sizePx, ranges);
    if (!f) f = io.Fonts->AddFontDefault();
    return f;
}

void ApplyStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style = ImGuiStyle(); // reset to defaults first

    style.WindowRounding    = 0.0f;
    style.ChildRounding     = Rounding;
    style.FrameRounding     = RoundingSmall;
    style.PopupRounding     = RoundingSmall;
    style.ScrollbarRounding = RoundingPill;
    style.GrabRounding      = RoundingPill;
    style.TabRounding       = RoundingSmall;

    style.WindowPadding = ImVec2(16, 16);
    style.FramePadding  = ImVec2(10, 8);
    style.ItemSpacing   = ImVec2(10, 10);
    style.ItemInnerSpacing = ImVec2(8, 6);
    style.IndentSpacing = 18.0f;
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize   = 10.0f;
    style.WindowBorderSize = 0.0f;
    style.ChildBorderSize  = 1.0f;
    style.FrameBorderSize  = 0.0f;
    style.PopupBorderSize  = 1.0f;

    ImVec4* c = style.Colors;
    auto U32ToVec4 = [](ImU32 col) { return ImGui::ColorConvertU32ToFloat4(col); };

    c[ImGuiCol_WindowBg]         = U32ToVec4(BgApp);
    c[ImGuiCol_ChildBg]          = U32ToVec4(BgCard);
    c[ImGuiCol_PopupBg]          = U32ToVec4(BgCardAlt);
    c[ImGuiCol_Border]           = U32ToVec4(Border);
    c[ImGuiCol_BorderShadow]     = ImVec4(0, 0, 0, 0);

    c[ImGuiCol_Text]             = U32ToVec4(TextPrimary);
    c[ImGuiCol_TextDisabled]     = U32ToVec4(TextMuted);

    c[ImGuiCol_FrameBg]          = U32ToVec4(BgInput);
    c[ImGuiCol_FrameBgHovered]   = U32ToVec4(BgHover);
    c[ImGuiCol_FrameBgActive]    = U32ToVec4(BgHover);

    c[ImGuiCol_Button]           = U32ToVec4(BgCardAlt);
    c[ImGuiCol_ButtonHovered]    = U32ToVec4(BgHover);
    c[ImGuiCol_ButtonActive]     = U32ToVec4(BgHover);

    c[ImGuiCol_Header]           = U32ToVec4(AccentSoft);
    c[ImGuiCol_HeaderHovered]    = U32ToVec4(AccentSoft2);
    c[ImGuiCol_HeaderActive]     = U32ToVec4(AccentSoft2);

    c[ImGuiCol_CheckMark]        = U32ToVec4(Accent);
    c[ImGuiCol_SliderGrab]       = U32ToVec4(Accent);
    c[ImGuiCol_SliderGrabActive] = U32ToVec4(AccentActive);

    c[ImGuiCol_ScrollbarBg]      = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_ScrollbarGrab]        = U32ToVec4(BgCardAlt);
    c[ImGuiCol_ScrollbarGrabHovered] = U32ToVec4(BgHover);
    c[ImGuiCol_ScrollbarGrabActive]  = U32ToVec4(BgHover);

    c[ImGuiCol_Separator]        = U32ToVec4(BorderSoft);
    c[ImGuiCol_SeparatorHovered] = U32ToVec4(Border);
    c[ImGuiCol_SeparatorActive]  = U32ToVec4(Accent);

    c[ImGuiCol_TableHeaderBg]    = U32ToVec4(BgTableHead);
    c[ImGuiCol_TableBorderStrong]= U32ToVec4(Border);
    c[ImGuiCol_TableBorderLight] = U32ToVec4(BorderSoft);
    c[ImGuiCol_TableRowBg]       = U32ToVec4(BgCard);
    c[ImGuiCol_TableRowBgAlt]    = U32ToVec4(BgCardAlt);
}

} // namespace Theme
