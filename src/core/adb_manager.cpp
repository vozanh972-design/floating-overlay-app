#include "adb_manager.h"
#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>   // CommandLineToArgvW
#include <sstream>
#include <regex>
#include <algorithm>

namespace core {

std::wstring Utf8ToWide(const std::string& s) {
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring out(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), out.data(), len);
    return out;
}

std::string WideToUtf8Str(const std::wstring& w) {
    if (w.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string out(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), out.data(), len, nullptr, nullptr);
    return out;
}

std::vector<std::wstring> SplitArgsW(const std::string& utf8Command) {
    std::wstring wide = Utf8ToWide(utf8Command);
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(wide.c_str(), &argc);
    std::vector<std::wstring> result;
    if (argv) {
        for (int i = 0; i < argc; ++i) result.emplace_back(argv[i]);
        LocalFree(argv);
    }
    return result;
}

// ---------------------------------------------------------------------- //

AdbManager::AdbManager() {
    auto found = FindAdbExecutable();
    if (found) m_adbPath = *found;
}

std::optional<std::wstring> AdbManager::FindAdbExecutable() {
    // 1. Already on PATH.
    wchar_t buf[MAX_PATH];
    DWORD n = SearchPathW(nullptr, L"adb.exe", nullptr, MAX_PATH, buf, nullptr);
    if (n > 0 && n < MAX_PATH) {
        return std::wstring(buf);
    }

    // 2. Common LDPlayer / Android SDK install locations.
    std::vector<std::wstring> envVars = { L"ProgramFiles", L"ProgramFiles(x86)", L"ProgramW6432", L"LOCALAPPDATA" };
    std::vector<std::wstring> candidates;
    for (auto& var : envVars) {
        wchar_t val[MAX_PATH];
        DWORD len = GetEnvironmentVariableW(var.c_str(), val, MAX_PATH);
        if (len == 0 || len >= MAX_PATH) continue;
        std::wstring base(val);
        candidates.push_back(base + L"\\LDPlayer\\LDPlayer9\\adb.exe");
        candidates.push_back(base + L"\\LDPlayer\\LDPlayer4.0\\adb.exe");
        candidates.push_back(base + L"\\LDPlayer9\\adb.exe");
        candidates.push_back(base + L"\\Android\\android-sdk\\platform-tools\\adb.exe");
        candidates.push_back(base + L"\\Android\\Sdk\\platform-tools\\adb.exe");
    }
    wchar_t userProfile[MAX_PATH];
    if (GetEnvironmentVariableW(L"USERPROFILE", userProfile, MAX_PATH) > 0) {
        candidates.push_back(std::wstring(userProfile) + L"\\AppData\\Local\\Android\\Sdk\\platform-tools\\adb.exe");
    }

    for (auto& c : candidates) {
        DWORD attrs = GetFileAttributesW(c.c_str());
        if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
            return c;
        }
    }
    return std::nullopt;
}

bool AdbManager::SetAdbPath(const std::wstring& path) {
    DWORD attrs = GetFileAttributesW(path.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES || (attrs & FILE_ATTRIBUTE_DIRECTORY)) return false;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_adbPath = path;
    return true;
}

bool AdbManager::IsAdbAvailable() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_adbPath.empty()) return false;
    DWORD attrs = GetFileAttributesW(m_adbPath.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

ProcessResult AdbManager::Run(const std::vector<std::wstring>& args, unsigned long timeoutMs) const {
    std::wstring path;
    { std::lock_guard<std::mutex> lock(m_mutex); path = m_adbPath; }
    if (path.empty()) {
        ProcessResult r;
        r.launched = false;
        r.launchError = "Không tìm thấy adb.exe. Vào Cài đặt để chọn đường dẫn thủ công.";
        return r;
    }
    return RunProcess(path, args, timeoutMs);
}

std::vector<std::wstring> AdbManager::WithSerial(const std::string& serial, std::vector<std::wstring> args) const {
    std::vector<std::wstring> full;
    if (!serial.empty()) {
        full.push_back(L"-s");
        full.push_back(Utf8ToWide(serial));
    }
    for (auto& a : args) full.push_back(std::move(a));
    return full;
}

ProcessResult AdbManager::StartServer()   { return Run({ L"start-server" }); }
ProcessResult AdbManager::KillServer()    { return Run({ L"kill-server" }); }
ProcessResult AdbManager::RestartServer() { KillServer(); return StartServer(); }

ProcessResult AdbManager::ListDevicesRaw() { return Run({ L"devices", L"-l" }); }

std::vector<Device> AdbManager::ParseDevices(const std::string& raw) const {
    std::vector<Device> devices;
    std::istringstream stream(raw);
    std::string line;
    bool first = true;
    while (std::getline(stream, line)) {
        if (first) { first = false; continue; } // skip "List of devices attached"
        // trim
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        line = line.substr(start);
        if (line.empty()) continue;

        std::istringstream ls(line);
        std::string serial, statusToken;
        ls >> serial >> statusToken;
        if (serial.empty()) continue;

        Device d;
        d.serial = serial;
        if (statusToken == "device") d.status = DeviceStatus::Device;
        else if (statusToken == "offline") d.status = DeviceStatus::Offline;
        else if (statusToken == "unauthorized") d.status = DeviceStatus::Unauthorized;
        else if (statusToken == "no") d.status = DeviceStatus::NoPermissions;
        else d.status = DeviceStatus::Unknown;

        devices.push_back(std::move(d));
    }
    return devices;
}

ProcessResult AdbManager::Connect(const std::string& address) {
    return Run({ L"connect", Utf8ToWide(address) });
}

ProcessResult AdbManager::Disconnect(const std::string& address) {
    if (address.empty()) return Run({ L"disconnect" });
    return Run({ L"disconnect", Utf8ToWide(address) });
}

ProcessResult AdbManager::RunShell(const std::string& serial, const std::vector<std::wstring>& shellArgs, unsigned long timeoutMs) {
    std::vector<std::wstring> args = WithSerial(serial, { L"shell" });
    for (auto& a : shellArgs) args.push_back(a);
    return Run(args, timeoutMs);
}

std::string AdbManager::GetDeviceProp(const std::string& serial, const std::string& prop) {
    auto r = RunShell(serial, { L"getprop", Utf8ToWide(prop) });
    if (!r.Success()) return {};
    std::string out = r.stdOut;
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) out.pop_back();
    return out;
}

static std::string ExtractRegex(const std::string& text, const std::regex& re) {
    std::smatch m;
    if (std::regex_search(text, m, re) && m.size() > 1) return m[1].str();
    return {};
}

void AdbManager::LoadExtendedInfo(Device& device) {
    device.manufacturer   = GetDeviceProp(device.serial, "ro.product.manufacturer");
    device.model           = GetDeviceProp(device.serial, "ro.product.model");
    device.androidVersion  = GetDeviceProp(device.serial, "ro.build.version.release");
    device.abi              = GetDeviceProp(device.serial, "ro.product.cpu.abi");

    auto wmSize = RunShell(device.serial, { L"wm", L"size" });
    if (wmSize.Success()) {
        device.resolution = ExtractRegex(wmSize.stdOut, std::regex(R"((\d+x\d+))"));
    }
    auto wmDensity = RunShell(device.serial, { L"wm", L"density" });
    if (wmDensity.Success()) {
        device.dpi = ExtractRegex(wmDensity.stdOut, std::regex(R"((\d+))"));
    }

    auto battery = RunShell(device.serial, { L"dumpsys", L"battery" });
    if (battery.Success()) {
        std::string level = ExtractRegex(battery.stdOut, std::regex(R"(level:\s*(\d+))"));
        std::string charging = battery.stdOut.find("status: 2") != std::string::npos ? "Đang sạc" : "";
        if (!level.empty()) {
            device.battery = (charging.empty() ? "" : charging + " (") + level + "%" + (charging.empty() ? "" : ")");
        }
    }

    auto meminfo = RunShell(device.serial, { L"cat", L"/proc/meminfo" });
    if (meminfo.Success()) {
        std::string kb = ExtractRegex(meminfo.stdOut, std::regex(R"(MemTotal:\s*(\d+))"));
        if (!kb.empty()) {
            double gb = std::stod(kb) / 1024.0 / 1024.0;
            std::ostringstream oss; oss.precision(1);
            oss << std::fixed << gb << " GB";
            device.ramTotal = oss.str();
        }
    }

    auto df = RunShell(device.serial, { L"df", L"/data" });
    if (df.Success()) {
        std::istringstream stream(df.stdOut);
        std::string headerLine, dataLine;
        std::getline(stream, headerLine);
        std::getline(stream, dataLine);
        std::istringstream ls(dataLine);
        std::string fs; long long blocks = 0, used = 0, avail = 0;
        ls >> fs >> blocks >> used >> avail;
        if (blocks > 0) {
            double totalGb = blocks / 1024.0 / 1024.0;
            std::ostringstream oss; oss.precision(1);
            oss << std::fixed << totalGb << " GB";
            device.storageTotal = oss.str();
        }
    }

    auto suCheck = RunShell(device.serial, { L"which", L"su" });
    device.isRooted = suCheck.Success() && !suCheck.stdOut.empty();

    device.infoLoaded = true;
}

ProcessResult AdbManager::InstallApk(const std::string& serial, const std::wstring& apkPath, bool reinstall) {
    std::vector<std::wstring> args = WithSerial(serial, { L"install" });
    if (reinstall) args.push_back(L"-r");
    args.push_back(apkPath);
    return Run(args, 120000);
}

ProcessResult AdbManager::UninstallPackage(const std::string& serial, const std::string& package) {
    return Run(WithSerial(serial, { L"uninstall", Utf8ToWide(package) }), 60000);
}

ProcessResult AdbManager::ListPackages(const std::string& serial) {
    return RunShell(serial, { L"pm", L"list", L"packages" }, 20000);
}

ProcessResult AdbManager::Push(const std::string& serial, const std::wstring& localPath, const std::string& remotePath) {
    return Run(WithSerial(serial, { L"push", localPath, Utf8ToWide(remotePath) }), 120000);
}

ProcessResult AdbManager::Pull(const std::string& serial, const std::string& remotePath, const std::wstring& localPath) {
    return Run(WithSerial(serial, { L"pull", Utf8ToWide(remotePath), localPath }), 120000);
}

ProcessResult AdbManager::CaptureScreenshot(const std::string& serial, const std::wstring& localSavePath) {
    const std::wstring remoteTmp = L"/sdcard/_adbconnect_screenshot.png";
    auto cap = RunShell(serial, { L"screencap", L"-p", remoteTmp }, 20000);
    if (!cap.Success()) return cap;
    auto pull = Pull(serial, WideToUtf8Str(remoteTmp), localSavePath);
    RunShell(serial, { L"rm", L"-f", remoteTmp }, 10000);
    return pull;
}

ProcessResult AdbManager::Reboot(const std::string& serial) {
    return Run(WithSerial(serial, { L"reboot" }), 20000);
}

ProcessResult AdbManager::ShellSimple(const std::string& serial, const std::string& shellCommandLine) {
    auto tokens = SplitArgsW(shellCommandLine);
    return RunShell(serial, tokens);
}

ProcessResult AdbManager::RunConsoleCommand(const std::string& serial, const std::vector<std::wstring>& adbArgs) {
    return Run(WithSerial(serial, adbArgs), 30000);
}

} // namespace core
