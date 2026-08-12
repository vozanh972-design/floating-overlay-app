#include "file_dialog.h"
#include <windows.h>
#include <commdlg.h>

namespace core {

std::optional<std::wstring> OpenFileDialog(const wchar_t* filter, const wchar_t* title) {
    wchar_t fileBuf[MAX_PATH] = L"";

    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = fileBuf;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameW(&ofn)) {
        return std::wstring(fileBuf);
    }
    return std::nullopt;
}

} // namespace core
