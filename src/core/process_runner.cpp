#include "process_runner.h"
#include <windows.h>
#include <sstream>

namespace core {

// Quote a single argument for the Win32 command line following the same
// rules CommandLineToArgvW expects, so paths with spaces / special chars
// round-trip correctly.
static std::wstring QuoteArg(const std::wstring& arg) {
    if (!arg.empty() && arg.find_first_of(L" \t\n\v\"") == std::wstring::npos) {
        return arg;
    }
    std::wstring result = L"\"";
    for (auto it = arg.begin(); ; ++it) {
        size_t backslashes = 0;
        while (it != arg.end() && *it == L'\\') { ++backslashes; ++it; }
        if (it == arg.end()) {
            result.append(backslashes * 2, L'\\');
            break;
        } else if (*it == L'"') {
            result.append(backslashes * 2 + 1, L'\\');
            result.push_back(L'"');
        } else {
            result.append(backslashes, L'\\');
            result.push_back(*it);
        }
    }
    result.push_back(L'"');
    return result;
}

static std::wstring BuildCommandLine(const std::wstring& exePath, const std::vector<std::wstring>& args) {
    std::wstring cmd = QuoteArg(exePath);
    for (const auto& a : args) {
        cmd.push_back(L' ');
        cmd += QuoteArg(a);
    }
    return cmd;
}

static std::string WideToUtf8(const std::wstring& w) {
    if (w.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string out(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), out.data(), len, nullptr, nullptr);
    return out;
}

ProcessResult RunProcess(const std::wstring& exePath, const std::vector<std::wstring>& args, unsigned long timeoutMs) {
    ProcessResult result;

    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE hOutRead = nullptr, hOutWrite = nullptr;
    HANDLE hErrRead = nullptr, hErrWrite = nullptr;

    if (!CreatePipe(&hOutRead, &hOutWrite, &sa, 0) ||
        !SetHandleInformation(hOutRead, HANDLE_FLAG_INHERIT, 0)) {
        result.launchError = "Không thể tạo pipe stdout";
        return result;
    }
    if (!CreatePipe(&hErrRead, &hErrWrite, &sa, 0) ||
        !SetHandleInformation(hErrRead, HANDLE_FLAG_INHERIT, 0)) {
        CloseHandle(hOutRead); CloseHandle(hOutWrite);
        result.launchError = "Không thể tạo pipe stderr";
        return result;
    }

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hOutWrite;
    si.hStdError  = hErrWrite;
    si.hStdInput  = nullptr;

    PROCESS_INFORMATION pi{};
    std::wstring cmdLine = BuildCommandLine(exePath, args);

    // CreateProcessW may write into the buffer, so it must be mutable.
    std::vector<wchar_t> mutableCmd(cmdLine.begin(), cmdLine.end());
    mutableCmd.push_back(L'\0');

    BOOL ok = CreateProcessW(
        nullptr, mutableCmd.data(), nullptr, nullptr, TRUE,
        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);

    CloseHandle(hOutWrite);
    CloseHandle(hErrWrite);

    if (!ok) {
        CloseHandle(hOutRead);
        CloseHandle(hErrRead);
        result.launchError = "Không thể chạy: " + WideToUtf8(exePath);
        return result;
    }
    result.launched = true;

    // Drain both pipes so the child never blocks on a full pipe buffer.
    auto drain = [](HANDLE h, std::string& out) {
        char buf[4096];
        DWORD read = 0;
        while (ReadFile(h, buf, sizeof(buf), &read, nullptr) && read > 0) {
            out.append(buf, read);
        }
    };

    std::thread outThread([&] { drain(hOutRead, result.stdOut); });
    std::thread errThread([&] { drain(hErrRead, result.stdErr); });

    DWORD waitResult = WaitForSingleObject(pi.hProcess, timeoutMs == 0 ? INFINITE : timeoutMs);
    if (waitResult == WAIT_TIMEOUT) {
        result.timedOut = true;
        TerminateProcess(pi.hProcess, 1);
    }

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    result.exitCode = static_cast<int>(exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    outThread.join();
    errThread.join();
    CloseHandle(hOutRead);
    CloseHandle(hErrRead);

    return result;
}

std::thread RunProcessAsync(const std::wstring& exePath, const std::vector<std::wstring>& args,
                             std::function<void(ProcessResult)> callback, unsigned long timeoutMs) {
    return std::thread([exePath, args, callback, timeoutMs]() {
        ProcessResult r = RunProcess(exePath, args, timeoutMs);
        if (callback) callback(std::move(r));
    });
}

// ------------------------------------------------------------------------ //
// ManagedProcess — used for `adb logcat` style long-running commands.
// ------------------------------------------------------------------------ //

ManagedProcess::~ManagedProcess() { Stop(); }

bool ManagedProcess::Start(const std::wstring& exePath, const std::vector<std::wstring>& args) {
    Stop();

    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE hRead = nullptr, hWrite = nullptr;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return false;
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hWrite;
    si.hStdError  = hWrite;

    PROCESS_INFORMATION pi{};
    std::wstring cmdLine = BuildCommandLine(exePath, args);
    std::vector<wchar_t> mutableCmd(cmdLine.begin(), cmdLine.end());
    mutableCmd.push_back(L'\0');

    BOOL ok = CreateProcessW(nullptr, mutableCmd.data(), nullptr, nullptr, TRUE,
                              CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    CloseHandle(hWrite);
    if (!ok) {
        CloseHandle(hRead);
        return false;
    }

    m_hProcess = pi.hProcess;
    m_hThread  = pi.hThread;
    m_hStdOutRead = hRead;
    m_running = true;
    return true;
}

void ManagedProcess::Stop() {
    if (m_hProcess) {
        TerminateProcess((HANDLE)m_hProcess, 0);
        CloseHandle((HANDLE)m_hProcess);
        m_hProcess = nullptr;
    }
    if (m_hThread) {
        CloseHandle((HANDLE)m_hThread);
        m_hThread = nullptr;
    }
    if (m_hStdOutRead) {
        CloseHandle((HANDLE)m_hStdOutRead);
        m_hStdOutRead = nullptr;
    }
    m_running = false;
}

bool ManagedProcess::IsRunning() const {
    if (!m_running || !m_hProcess) return false;
    DWORD code = 0;
    if (GetExitCodeProcess((HANDLE)m_hProcess, &code)) {
        return code == STILL_ACTIVE;
    }
    return false;
}

bool ManagedProcess::PollOutput(std::string& outAppend) {
    if (!m_hStdOutRead) return false;
    DWORD available = 0;
    if (!PeekNamedPipe((HANDLE)m_hStdOutRead, nullptr, 0, nullptr, &available, nullptr) || available == 0) {
        return false;
    }
    char buf[4096];
    DWORD read = 0;
    bool any = false;
    while (available > 0) {
        DWORD toRead = (available < (DWORD)sizeof(buf)) ? available : (DWORD)sizeof(buf);
        if (!ReadFile((HANDLE)m_hStdOutRead, buf, toRead, &read, nullptr) || read == 0) break;
        outAppend.append(buf, read);
        any = true;
        if (!PeekNamedPipe((HANDLE)m_hStdOutRead, nullptr, 0, nullptr, &available, nullptr)) break;
    }
    return any;
}

} // namespace core
