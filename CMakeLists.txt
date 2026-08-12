cmake_minimum_required(VERSION 3.20)
project(ADBConnectLDPlayerTool LANGUAGES CXX)

if (NOT WIN32)
    message(FATAL_ERROR "This project targets Windows (Win32 + Direct3D 11) only.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(imgui CONFIG REQUIRED)

file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.h"
)

# WIN32 subsystem => real GUI exe, no console window, no interpreter of any kind.
add_executable(ADBConnectLDPlayerTool WIN32 ${APP_SOURCES})

target_include_directories(ADBConnectLDPlayerTool PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
)

target_link_libraries(ADBConnectLDPlayerTool PRIVATE
    imgui::imgui
    d3d11.lib
    d3dcompiler.lib
    dwmapi.lib
    dwrite.lib
    Shcore.lib
    comdlg32.lib
)

target_compile_definitions(ADBConnectLDPlayerTool PRIVATE
    UNICODE
    _UNICODE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
)

set_target_properties(ADBConnectLDPlayerTool PROPERTIES
    OUTPUT_NAME "ADBConnectLDPlayerTool"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if (MSVC)
    target_compile_options(ADBConnectLDPlayerTool PRIVATE /W4 /utf-8)
endif()
