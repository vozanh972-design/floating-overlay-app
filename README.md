# ADB Connect - LDPlayer Tool (C++ native)

Ứng dụng desktop Windows viết **hoàn toàn bằng C++** — không Python, không
interpreter nào bị đóng gói bên trong. Stack thật:

- **Win32 API** — cửa sổ, message loop, xử lý input, tiến trình con (ADB).
- **Direct3D 11** — renderer cho toàn bộ giao diện.
- **Dear ImGui** — vẽ UI tức thời (immediate-mode) trên D3D11, tự vẽ 100% các
  card/nút/bảng/ icon theo đúng bố cục trong ảnh thiết kế (không dùng theme
  điều khiển có sẵn của Windows).
- **CreateProcess + pipe** — gọi `adb.exe` trực tiếp bằng Win32 API thật (argv
  list, không `shell=True`/cmd.exe), y hệt cách các công cụ ADB GUI chuyên
  nghiệp vẫn làm.

Build ra **1 file `.exe` PE32+ thật**, biên dịch bởi MSVC (`cl.exe`/`link.exe`),
chạy được trên máy Windows sạch, không cần cài Python/Java/.NET/Node.

## 1. Kiến trúc mã nguồn

```text
ldplayer_adb_tool_cpp/
├── CMakeLists.txt          # cấu hình build (add_executable WIN32 ...)
├── vcpkg.json              # khai báo phụ thuộc: Dear ImGui (win32+dx11 backend)
├── .github/workflows/build-exe.yml   # CI build .exe thật bằng MSVC trên GitHub
├── src/
│   ├── main.cpp             # WinMain, khởi tạo D3D11, message loop, WndProc
│   ├── app.h / app.cpp       # State toàn cục: AdbManager, danh sách device, view hiện tại
│   ├── core/
│   │   ├── process_runner.h/.cpp   # CreateProcess + pipe (chạy adb.exe an toàn)
│   │   ├── adb_manager.h/.cpp      # Tìm adb.exe, devices/connect/shell/install/pull/push...
│   │   ├── device.h                 # struct Device
│   │   └── file_dialog.h/.cpp       # Hộp thoại chọn file Win32 (GetOpenFileNameW)
│   └── ui/
│       ├── theme.h/.cpp      # Bảng màu + font (load segoeui.ttf từ Windows Fonts)
│       ├── icons.h/.cpp      # Icon vẽ vector bằng ImDrawList (không phụ thuộc emoji font)
│       ├── widgets.h/.cpp    # Card, Button, Badge, StatusDot, RadioOption, QuickActionTile...
│       ├── views.h
│       ├── views_connect.cpp # Màn hình "Kết nối" — bám sát ảnh thiết kế
│       └── views_other.cpp   # Thiết bị, Điều khiển, Ứng dụng, Tập tin, ADB Shell, Cài đặt, Giới thiệu
└── assets/
```

**Không có dòng Python nào trong toàn bộ project.** Toàn bộ UI được vẽ bằng
lệnh `ImDrawList` (hình chữ nhật bo góc, đường thẳng, vòng cung...) nên icon,
card, bảng, nút bấm hiển thị y hệt trên mọi máy Windows, không phụ thuộc theme
hệ thống hay font emoji.

## 2. Vì sao chọn stack này thay vì PyInstaller/Nuitka

| Stack cũ (Python) | Stack C++ này |
|---|---|
| `.exe` chỉ là Python bytecode + interpreter đóng gói lại | `.exe` là mã máy thật, biên dịch trực tiếp bởi MSVC |
| Cần Tkinter (theme hệ thống, giới hạn tùy biến) | Tự vẽ 100% UI bằng Direct3D11 + Dear ImGui, khớp pixel với thiết kế |
| Khởi động chậm hơn do nạp interpreter | Khởi động tức thì như mọi ứng dụng Win32 native |
| `subprocess` gọi qua lớp trừu tượng của Python | `CreateProcess` + pipe gọi thẳng Win32 API |

## 3. Build tự động trên GitHub (khuyến nghị)

```powershell
cd ldplayer_adb_tool_cpp
git init
git add .
git commit -m "Initial commit"
git branch -M main
git remote add origin https://github.com/<username>/<repo>.git
git push -u origin main
```

Ngay khi push, `.github/workflows/build-exe.yml` sẽ tự động:

1. Dựng máy ảo Windows (`windows-latest`, đã có sẵn MSVC + vcpkg).
2. `vcpkg` tải và build Dear ImGui (kèm backend Win32 + DirectX11).
3. `cmake --build` gọi thẳng **`cl.exe`/`link.exe`** biên dịch toàn bộ `src/*.cpp`
   thành `build/Release/ADBConnectLDPlayerTool.exe`.
4. Upload file exe làm **workflow artifact**.

Xem kết quả tại tab **Actions** trên GitHub repo → chọn lần chạy mới nhất →
mục **Artifacts** ở cuối trang → tải `ADBConnectLDPlayerTool-windows-x64`.

### Tạo Release chính thức

```powershell
git tag v1.0.0
git push origin v1.0.0
```

Workflow tự đính kèm file `.exe` vào GitHub Release `v1.0.0`.

## 4. Build local (nếu máy bạn có Visual Studio 2022 + Desktop C++ workload)

```powershell
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat

cd ldplayer_adb_tool_cpp
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 ^
      -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Release
```

File thực thi: `build\Release\ADBConnectLDPlayerTool.exe`.

## 5. Kết nối LDPlayer

1. Mở app → tab **Kết nối** hiện lên ngay (đúng như ảnh thiết kế: card
   "Phương thức kết nối", card "Trạng thái kết nối", bảng "Danh sách thiết
   bị", "Thao tác nhanh", "Thông tin thiết bị", "ADB Shell nhanh").
2. App tự dò `adb.exe` (kiểm tra PATH, thư mục cài LDPlayer phổ biến). Nếu
   không thấy, vào tab **Cài đặt** để chọn thủ công hoặc bấm "Tự động dò".
3. Chọn "Kết nối tự động" rồi bấm **Kết nối**, hoặc chọn "Kết nối thủ công"
   và nhập địa chỉ (VD `127.0.0.1:5555`).
4. Bảng "Danh sách thiết bị" sẽ liệt kê các LDPlayer instance đang chạy;
   bấm **Chọn** ở dòng tương ứng để thao tác.

## 6. Trạng thái các tab

| Tab | Trạng thái |
|---|---|
| Kết nối | Đầy đủ, khớp thiết kế ảnh gốc |
| Thiết bị | Hoạt động: chọn / reboot / ngắt kết nối theo từng device |
| Ứng dụng | Hoạt động: cài / cài lại / gỡ APK qua hộp thoại chọn file thật |
| ADB Shell | Hoạt động: console thật gọi `adb shell` trên device đã chọn |
| Cài đặt | Hoạt động: chọn/đổi đường dẫn `adb.exe`, tự động dò |
| Điều khiển, Tập tin | Khung sẵn (placeholder), sẽ hoàn thiện ở bản sau |
| Giới thiệu | Thông tin tĩnh |

## 7. Ghi chú kỹ thuật

- Cửa sổ **frameless custom** (title bar tự vẽ) dùng kỹ thuật chuẩn:
  `WM_NCCALCSIZE` để bỏ caption mặc định, `WM_NCHITTEST` để vẫn kéo-thả và
  resize được như cửa sổ bình thường (giữ Aero Snap của Windows).
- Mọi lệnh `adb` được gọi qua `CreateProcess` với **argument list**, không
  bao giờ dựng chuỗi lệnh cho `cmd.exe`/`shell=True` — tránh command injection.
- Font UI dùng `segoeui.ttf` có sẵn trong `C:\Windows\Fonts`, nạp cùng dải
  glyph tiếng Việt (`GetGlyphRangesVietnamese`) để hiển thị dấu tiếng Việt
  chuẩn không cần đóng gói font riêng.
