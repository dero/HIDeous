# HIDeous - AI Agent Documentation

## Project Overview
HIDeous is a specialized Windows keyboard remapping and macro application. Its primary feature is the ability to distinguish between multiple physical keyboards (using Raw Input) and intercept keys globally (using a Windows Hook), allowing for device-specific macros.

## Directory Structure

### `src/app/` (Main Application)
- **Target**: `Hideous.exe`
- **Responsibilities**:
  - Creates the main GUI window and tray icon.
  - Registers for Raw Input (`WM_INPUT`) to identify which specific physical keyboard is being used.
  - Installs the global hook.
  - Receives `WM_HIDEOUS_KEYBOARD_EVENT` messages from the hook DLL.
  - makes the final decision on whether to block or pass a keypress.
- **Key Files**:
  - `hideous_app.cpp`: `WinMain`, message loop, and hook installation logic.
  - `gui.cpp`: Window procedure, UI rendering (pure Win32 API), and startup logic.

### `src/dll/` (Hook DLL)
- **Target**: `hideous_hook.dll`
- **Responsibilities**:
  - Installs a global `WH_KEYBOARD` hook.
  - Injected into every running process on the system to intercept keystrokes.
  - **IPC**: Uses a Shared Data Section (`#pragma data_seg(".shared")`) to reliably share the main window handle (`g_mainWindow`) across all process instances. This avoids reliable window discovery issues during startup.
- **Key Files**:
  - `hideous_hook.cpp`: Implements `KeyboardProc` and the shared memory section.

### `src/common/` (Shared functionality)
- **Responsibilities**: Helper functions used by both the App and the DLL.
- **Key Files**:
  - `logging.cpp`: Thread-safe file logging.
  - `settings.cpp`: INI file management.

## Architecture Notes
1. **Hooking Mechanism**: The app installs a global hook. The DLL is injected into all processes.
2. **Device Discrimination**: Windows Hooks do *not* provide device information. Raw Input *does* provide device info but cannot block keys. HIDeous correlates the two events (Hook + Raw Input) based on timing to match a keypress to a specific device.
3. **Optimization (Shared Memory)**: To avoid IPC overhead for every keypress, the DLL maintains a shared memory bitmask (`g_interestedKeys`) of all keys that are mapped to macros. If a key is not in this list, the DLL lets it through immediately without checking with the main app.
4. **Startup Behavior**: The app can register itself in the Registry Run key. 

## Build System
- **Tool**: CMake
- **Command**: `cmake -B build` -> `cmake --build build --config Release`
