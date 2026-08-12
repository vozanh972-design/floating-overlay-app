#pragma once
#include <string>
#include <optional>

namespace core {

// Opens the classic Win32 "Open File" dialog. `filter` uses the
// GetOpenFileNameW double-null-terminated format, e.g.:
//   L"APK files\0*.apk\0All files\0*.*\0"
std::optional<std::wstring> OpenFileDialog(const wchar_t* filter, const wchar_t* title);

} // namespace core
