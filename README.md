# 🖱️ MouseDebouncer

[![Build & Release](https://github.com/luckyleprechauns/MouseDebouncer/actions/workflows/build.yml/badge.svg)](https://github.com/luckyleprechauns/MouseDebouncer/actions/workflows/build.yml)
[![Latest Release](https://img.shields.io/github/v/release/luckyleprechauns/MouseDebouncer?label=download)](https://github.com/luckyleprechauns/MouseDebouncer/releases/latest)

A lightweight Windows utility that filters out unwanted double-clicks caused by aging or defective mouse switches. Configure independent debounce intervals for the left, right, and middle buttons — no driver hacks or hardware mods required.

## Download

**[⬇️ Download Latest Release](https://github.com/luckyleprechauns/MouseDebouncer/releases/latest)**

| File | Description |
|---|---|
| `MouseDebouncer-*-setup.exe` | **Windows installer** (recommended) — installs to Program Files, adds Start Menu entry, optional auto-start on login, and proper uninstall via "Apps & Features" |
| `MouseDebouncer-portable-win64.zip` | **Portable version** — extract anywhere and run, no installation needed |

## The Problem

Over time, mouse micro-switches wear out and begin registering phantom double-clicks when you only click once. This is a well-known hardware defect that affects mice from every manufacturer. Replacing the mouse (or the switch) is the permanent fix, but **MouseDebouncer** gives you a free, instant software workaround.

## Features

- **Per-button debounce** — Set separate debounce times (in milliseconds) for left, right, and middle buttons
- **System tray integration** — Runs quietly in the notification area; right-click the icon for settings or exit
- **Persistent settings** — Debounce values are saved to the Windows registry and restored on launch
- **Singleton instance** — Only one copy runs at a time; launching again brings the existing window to the foreground
- **Show on Startup option** — Choose whether the settings dialog appears automatically when the app starts
- **Zero external dependencies** — Pure Win32 API; no frameworks, no runtimes to install
- **Minimal resource usage** — Uses a low-level mouse hook with negligible CPU overhead

## How It Works

MouseDebouncer installs a global low-level mouse hook (`WH_MOUSE_LL`). When a button-down event arrives within the configured debounce window of the previous click on the same button, the event (and its corresponding button-up) is silently swallowed. Legitimate clicks pass through untouched.

| Setting | What it controls |
|---|---|
| **Left Debounce (ms)** | Minimum interval between two left clicks |
| **Right Debounce (ms)** | Minimum interval between two right clicks |
| **Middle Debounce (ms)** | Minimum interval between two middle clicks |

> **Tip:** Start with a value around **40–80 ms**. If you still get phantom double-clicks, increase gradually. Values above **150 ms** may interfere with intentional double-clicking.

## Building from Source

### Prerequisites

- **CMake** ≥ 3.25
- **MSVC** — one of the following, with the Windows SDK:
  - Visual Studio 2022 (any edition) or Build Tools for Visual Studio 2022
  - Visual Studio 2019 (any edition) or Build Tools for Visual Studio 2019
- **Windows 10/11**

### Build Steps

```cmd
:: Clone the repository
git clone https://github.com/luckyleprechauns/MouseDebouncer.git
cd MouseDebouncer

:: Configure (pick the generator that matches your installed Visual Studio version)
cmake -B build -G "Visual Studio 17 2022"
:: cmake -B build -G "Visual Studio 16 2019"   &:: VS 2019

:: Build
cmake --build build --config Release
```

The compiled executable will be at `build/bin/MouseDebouncer.exe`.

#### Alternative: Build Tools only (no full Visual Studio IDE)

If you only have **Build Tools for Visual Studio** installed (without the full IDE), use the Ninja generator from a **Developer Command Prompt**:

```cmd
:: Open "Developer Command Prompt for VS 2022" first, then:
cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The executable will be at `build/bin/MouseDebouncer.exe`.

## Installation

### Quick Install

After building, install to `C:\Program Files\MouseDebouncer`:

```cmd
:: Run from an elevated (Administrator) command prompt
cmake --install build --prefix "C:\Program Files\MouseDebouncer"
```

Then run `install-shortcuts.bat` from the install directory to optionally:
- Create a **Start Menu** shortcut
- Add to **Startup** (launches on login with a 10-second delay)
- Or both

### Manual Install

The built `MouseDebouncer.exe` is fully standalone — just copy it anywhere and run it. Settings are stored in the registry, not alongside the executable.

## Usage

1. **Run** `MouseDebouncer.exe` — the app starts in the system tray.
2. **Right-click** the tray icon → **Settings…** (or double-click the icon) to open the settings dialog.
3. Enter debounce times in milliseconds for each button.
4. Click **Apply** to save and close, or **Cancel** to discard changes.
5. Click **Help** for version and attribution info.
6. Click **Exit** (in the dialog or tray menu) to quit.

Settings are stored in the registry under `HKEY_CURRENT_USER\Software\MouseDebouncer` and persist across restarts.

## Project Structure

```
MouseDebouncer/
├── .github/
│   └── workflows/
│       └── build.yml              # CI/CD: build, package, and release
├── src/
│   └── main.cpp                   # Application entry point, hook logic, and UI
├── resources/
│   ├── MouseDebouncer.rc          # Win32 resource script (dialogs, icons)
│   ├── resource.h                 # Dialog and control ID definitions
│   ├── mouse.ico                  # System tray icon
│   ├── mouse_high_res_trpt.ico    # High-resolution icon for the settings dialog
│   └── mouse_high_res_md_24b.bmp  # Bitmap asset
├── installer/
│   └── MouseDebouncer.iss         # Inno Setup installer script
├── scripts/
│   └── install-shortcuts.bat      # Creates Start Menu / Startup shortcuts (portable)
├── CMakeLists.txt                 # Build and install configuration
├── README.md                      # This file
└── .gitignore
```

## Credits

- Original debounce logic by [ItzOwo](https://github.com/ItzOwo)
- GUI, system tray interface, and packaging by [luckyleprechauns](https://github.com/luckyleprechauns)

## License

This project is provided as-is for personal use. See the repository for any license details.
