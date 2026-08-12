#pragma once
#include <string>
#include <vector>
#include <functional>
#include <thread>

// Runs external processes (adb.exe) via the real Win32 CreateProcess API with
// anonymous pipes for stdout/stderr capture. No shell is ever invoked
// (CreateProcess is called directly with an argv-style command line that we
// build ourselves with proper quoting), which avoids shell-injection risk.
namespace core {

struct ProcessResult {
    bool        launched   = false;   // false if CreateProcess itself failed
    int         exitCode   = -1;
    std::string stdOut;
    std::string stdErr;
    bool        timedOut   = false;
    std::string launchError;          // human readable error if launched == false

    bool Success() const { return launched && !timedOut && exitCode == 0; }
};

// Runs `exePath args...` synchronously, waiting up to timeoutMs (0 = infinite).
ProcessResult RunProcess(const std::wstring& exePath,
                          const std::vector<std::wstring>& args,
                          unsigned long timeoutMs = 15000);

// Fire-and-forget: runs RunProcess on a background thread and invokes
// `callback` with the result once finished. Callback runs on the worker
// thread — callers must not touch ImGui/D3D state directly from it; instead
// stash the result behind a mutex/atomic and pick it up on the next frame.
std::thread RunProcessAsync(const std::wstring& exePath,
                             const std::vector<std::wstring>& args,
                             std::function<void(ProcessResult)> callback,
                             unsigned long timeoutMs = 15000);

// A long-running managed child process (e.g. `adb logcat`) whose stdout can
// be polled incrementally without blocking the caller.
class ManagedProcess {
public:
    ManagedProcess() = default;
    ~ManagedProcess();

    bool Start(const std::wstring& exePath, const std::vector<std::wstring>& args);
    void Stop();
    bool IsRunning() const;

    // Non-blocking: appends any newly available stdout text to `outAppend`.
    // Returns true if new data was read.
    bool PollOutput(std::string& outAppend);

private:
    void* m_hProcess = nullptr;
    void* m_hThread  = nullptr;
    void* m_hStdOutRead = nullptr;
    void* m_hStdOutWrite = nullptr;
    bool  m_running = false;
};

} // namespace core
