#pragma once
#include <string>
#include <vector>
#include <optional>
#include <mutex>
#include <chrono>
#include "device.h"
#include "process_runner.h"

namespace core {

class AdbManager {
public:
    AdbManager();

    // --- Discovery -------------------------------------------------------
    static std::optional<std::wstring> FindAdbExecutable();
    bool SetAdbPath(const std::wstring& path);   // validates the file exists
    bool IsAdbAvailable() const;
    std::wstring GetAdbPath() const { return m_adbPath; }

    // --- Server control ----------------------------------------------------
    ProcessResult StartServer();
    ProcessResult KillServer();
    ProcessResult RestartServer();

    // --- Devices -----------------------------------------------------------
    ProcessResult ListDevicesRaw();                          // `adb devices -l`
    std::vector<Device> ParseDevices(const std::string& raw) const;
    ProcessResult Connect(const std::string& address);        // `adb connect <addr>`
    ProcessResult Disconnect(const std::string& address = ""); // `adb disconnect [addr]`

    // --- Shell / info --------------------------------------------------------
    // Note: named RunShell / GetDeviceProp (not Shell / GetProp) to avoid
    // colliding with Win32's GetProp macro (winuser.h -> GetPropW under
    // UNICODE) and the COM "Shell" type declared in <shldisp.h> (pulled in
    // transitively via <shlobj.h>), both of which caused real MSVC build
    // failures when this file used the shorter names.
    ProcessResult RunShell(const std::string& serial, const std::vector<std::wstring>& shellArgs, unsigned long timeoutMs = 15000);
    std::string   GetDeviceProp(const std::string& serial, const std::string& prop);
    void          LoadExtendedInfo(Device& device);    // model, android ver, resolution, battery, cpu, ram...

    // --- Package management -------------------------------------------------
    ProcessResult InstallApk(const std::string& serial, const std::wstring& apkPath, bool reinstall);
    ProcessResult UninstallPackage(const std::string& serial, const std::string& package);
    ProcessResult ListPackages(const std::string& serial);

    // --- File transfer -------------------------------------------------------
    ProcessResult Push(const std::string& serial, const std::wstring& localPath, const std::string& remotePath);
    ProcessResult Pull(const std::string& serial, const std::string& remotePath, const std::wstring& localPath);

    // --- Screenshot ------------------------------------------------------------
    ProcessResult CaptureScreenshot(const std::string& serial, const std::wstring& localSavePath);

    // --- Device power / misc ------------------------------------------------
    ProcessResult Reboot(const std::string& serial);
    ProcessResult ShellSimple(const std::string& serial, const std::string& shellCommandLine);

    // --- Raw console command (arbitrary but argv-based, no shell) ------------
    ProcessResult RunConsoleCommand(const std::string& serial, const std::vector<std::wstring>& adbArgs);

private:
    ProcessResult Run(const std::vector<std::wstring>& args, unsigned long timeoutMs = 15000) const;
    std::vector<std::wstring> WithSerial(const std::string& serial, std::vector<std::wstring> args) const;

    std::wstring m_adbPath;
    mutable std::mutex m_mutex;
};

// UTF helpers shared across core/ui.
std::wstring Utf8ToWide(const std::string& s);
std::string  WideToUtf8Str(const std::wstring& w);
std::vector<std::wstring> SplitArgsW(const std::string& utf8Command);

} // namespace core
