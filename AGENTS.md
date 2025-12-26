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
  - Makes the final decision on whether to block or pass a keypress.
- **Key Components**:
  - **ui/**:
    - `main_window.cpp`: Main message loop handler and window procedure.
    - `taskbar_icon.cpp`: System tray management.
    - `components/`: Reusable UI elements (`device_list`, `profile_selector`).
  - **processing/**:
    - `macro_executor.cpp`: Core logic for dispatching macros and deciding on key blocking.
    - `raw_input_handler.cpp`: Processes `WM_INPUT` messages to update tracking of the last physical device used.
    - `commands/`: Implementation of specific commands (`text:`, `keys:`, `run:`, `profile:`).
  - `entry_point.cpp`: Application entry point (`WinMain`), resource loading, and setup.

### `src/hook_dll/` (Hook Library)
- **Target**: `hideous_hook.dll`
- **Location**: `src/hook_dll`
- **Responsibilities**:
  - Installs a global `WH_KEYBOARD` hook.
  - Injected into every running process on the system to intercept keystrokes.
  - **IPC**: Uses a Shared Data Section (`#pragma data_seg(".shared")`) to reliably share the main window handle (`g_mainWindow`) and interested keys mask (`g_interestedKeys`) across all process instances.
- **Key Files**:
  - `dll_main.cpp`: Implements `KeyboardProc` and the shared memory section.

### `src/common/` (Shared Library)
- **Target**: `common.lib` (Static Library)
- **Responsibilities**: Helper functions and types used by both the App and the DLL.
- **Modules**:
  - **config/**: `settings_manager.cpp` (Singleton for loading/saving INI files, profile management, and file watching).
    - `key_mapping.cpp`: Virtual Key Code string conversion helpers and Scan Code parsing.
    - `utils/**`: 
    - `logging.cpp`: Thread-safe file logging.
    - `crypto.cpp`: Device name hashing.
    - `string_utils.cpp`: String manipulation helpers.

## Architecture Notes
1. **Hooking Mechanism**: The app installs a global hook. The DLL is injected into all processes.
2. **Device Discrimination**: Windows Hooks do *not* provide device information. Raw Input *does* provide device info but cannot block keys. HIDeous correlates the two events (Hook + Raw Input) based on timing to match a keypress to a specific device.
3. **Optimization (Shared Memory)**: The DLL maintains two shared memory bitmasks:
    - `g_interestedKeys` (256 bytes): Virtual Key Bloom filter.
    - `g_interestedScanCodes` (2048 bytes): Scan Code Bloom filter (supporting extended keys).
    If a key is not in either list, the DLL lets it through immediately.
4. **Command Pattern**: Macro execution is handled via a dispatcher in `macro_executor.cpp` which delegates to specific command handlers in `src/app/processing/commands/`.

## Keypress Flow

Detailed flow is documented in [KEYPRESS_FLOW.md](docs/KEYPRESS_FLOW.md).

## Extended Key Handling
The application handles "Extended Keys" (like Right Ctrl, Arrow Keys, Numpad Enter) by tracking the extended bit in the scan code.
- **Raw Input**: Checks for `RI_KEY_E0` or `RI_KEY_E1` flags. Adds `+1000` to the internal scan code representation.
- **Hook**: Checks bit 24 of `lParam`. Adds `+1000` to the scan code.
- **Effect**: allows users to bind `Left Ctrl` separately from `Right Ctrl`.

## Build System
- **Tool**: CMake
- **Command**: `cmake -B build` -> `cmake --build build --config Release`
