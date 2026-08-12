#pragma once
#include "imgui.h"

// All UI icons are hand-drawn with ImDrawList primitives (lines, circles,
// rounded rects) rather than relying on emoji glyph coverage in whatever
// font happens to be installed. This guarantees the icons always render
// identically on every Windows machine.
namespace Icons {

enum class Type {
    Link,       // connect / plug
    Unlink,     // disconnect
    Phone,      // device
    Gamepad,    // control
    Grid,       // apps
    Folder,     // files
    Terminal,   // adb shell  ">_"
    Gear,       // settings
    Info,       // about "i"
    Camera,     // screenshot
    VideoRec,   // screen record
    ApkBox,     // install apk (box + down arrow)
    Trash,      // uninstall
    Refresh,    // restart / reload
    Power,      // power off
    Rotate,     // rotate screen
    Volume,     // audio
    ChevronDown,
    Dots,       // kebab menu
    Close,      // window close X
    Minimize,   // window minimize
    Maximize,   // window maximize (square outline)
    Search,
};

// Draws icon `type` centered at `center`, roughly `size` px across, in `color`.
void Draw(ImDrawList* dl, Type type, ImVec2 center, float size, ImU32 color, float thickness = 1.6f);

} // namespace Icons
