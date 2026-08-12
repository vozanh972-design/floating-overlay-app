#pragma once
#include <string>

namespace core {

enum class DeviceStatus {
    Device,         // "device" — fully connected and authorized
    Offline,
    Unauthorized,
    NoPermissions,
    Unknown,
};

inline const char* DeviceStatusLabel(DeviceStatus s) {
    switch (s) {
        case DeviceStatus::Device:        return "Đã kết nối";
        case DeviceStatus::Offline:       return "Ngoại tuyến";
        case DeviceStatus::Unauthorized:  return "Chưa cấp quyền";
        case DeviceStatus::NoPermissions: return "Không có quyền";
        default:                          return "Không xác định";
    }
}

struct Device {
    std::string  serial;              // e.g. "127.0.0.1:5555"
    DeviceStatus status = DeviceStatus::Unknown;

    // Extended info (populated lazily via `adb shell getprop` / dumpsys).
    bool         infoLoaded   = false;
    std::string  displayName  = "LDPlayer";   // friendly name shown in UI
    std::string  model;
    std::string  androidVersion;
    std::string  resolution;
    std::string  dpi;
    std::string  manufacturer;
    std::string  cpu;
    std::string  ramTotal;
    std::string  storageTotal;
    std::string  battery;
    bool         isRooted = false;
    std::string  abi;
};

} // namespace core
