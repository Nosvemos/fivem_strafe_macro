cmake_minimum_required(VERSION 3.20)
project(StrafeMacro LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Compiler-specific flags ──────────────────────────────────────────
if(MSVC)
    # Static CRT linkage → no external MSVC runtime DLLs needed
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_compile_options(
        $<$<CONFIG:Release>:/O2>    # Full optimization
        $<$<CONFIG:Release>:/GL>    # Whole-program optimization
        /W4                         # High warning level
    )
    add_link_options($<$<CONFIG:Release>:/LTCG>)
else()
    # MinGW – static linkage for a self-contained .exe
    add_link_options(-static -static-libgcc -static-libstdc++)
    add_compile_options(
        $<$<CONFIG:Release>:-O2>
        $<$<CONFIG:Release>:-s>     # Strip symbols
        -Wall -Wextra -Wno-cast-function-type
    )
endif()

# ── Sources ──────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/gui.cpp
    src/macro_engine.cpp
    src/input_sender.cpp
    src/humanizer.cpp
    src/hotkey_manager.cpp
    src/process_monitor.cpp
    src/resource.rc
)

set(HEADERS
    src/config.h
    src/gui.h
    src/macro_engine.h
    src/input_sender.h
    src/humanizer.h
    src/hotkey_manager.h
    src/process_monitor.h
)

# ── Executable (WIN32 = no console window) ───────────────────────────
add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})

# ── Link system libraries ────────────────────────────────────────────
target_link_libraries(${PROJECT_NAME} PRIVATE
    comctl32    # Common controls (TrackBar, etc.)
    shell32     # Shell_NotifyIcon (tray icon)
    dwmapi      # DwmSetWindowAttribute (dark title bar)
)

# ── Unicode build ────────────────────────────────────────────────────
target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE _UNICODE)
