# Keypress Interception Flow

This document details how HIDeous intercepts, identifies, and decides whether to block or process keyboard events.

## Architecture Overview

The system relies on a "Hybrid Hooking" approach:
1.  **Raw Input (`WM_INPUT`)**: Used to **identify** the physical device source of a keypress.
2.  **Windows Hook (`WH_KEYBOARD`)**: Used to **intercept and block** the keypress globally.

These two systems run in parallel, and the application correlates them to apply device-specific macros.

## 1. Shared Memory (IPC)

The `hideous_hook.dll` uses a **Shared Data Section** to communicate between the main application and the hook instances injected into every running process.

- **Mechanism**: `#pragma data_seg(".shared")`
- **Shared Variables**:
    - `HHOOK g_keyboardHook`: The global hook handle.
    - `HWND g_mainWindow`: Handle to the main HIDeous window (for IPC messages).
    - `BYTE g_interestedKeys[256]`: Bloom filter for Virtual Keys (VK).
    - `BYTE g_interestedScanCodes[2048]`: Bloom filter for Scan Codes (SC).

This optimization ensures that if a key is *not* bound to any macro (bit is 0 in filters), the DLL passes it immediately without switching context to the main app, preserving system performance.

## 2. The Hook (`WH_KEYBOARD`)

The DLL installs a system-wide `WH_KEYBOARD` hook using `SetWindowsHookEx`.

- **Callback**: `KeyboardProc(int code, WPARAM wParam, LPARAM lParam)`
- **File**: `src/hook_dll/dll_main.cpp`
- **Logic**:
    1.  **Safety**: Ignores injected events marked with `HIDEOUS_IDENTIFIER` to prevent loops.
    2.  **Extended Keys**: Checks bit 24 of `lParam`. If set, adds `1000` to the scan code.
        - Example: `Left Ctrl` = SC 29, `Right Ctrl` = SC 1029.
    3.  **Filtering**: Checks if the normalized Scan Code or VK is likely to have a macro (using Shared Memory filters).
    4.  **IPC**: If "interested", sends a synchronous message to the main app:
        ```cpp
        SendMessageTimeout(g_mainWindow, WM_HIDEOUS_KEYBOARD_EVENT, wParam, lParam, ...);
        ```
    5.  **Blocking**: If the app returns `KEY_DECISION_BLOCK` (1), the hook returns `1` to suppress the key.

## 3. Raw Input (`WM_INPUT`)

The main application registers for Raw Input (RIDEV_INPUTSINK) to receive all keyboard events, even when not in focus.

- **Handler**: `src/app/processing/raw_input_handler.cpp`
- **Role**: purely for **Identification**. Windows Hooks do not tell you *which* keyboard sent a key. Raw Input does (`header.hDevice`).
- **Logic**:
    - Parses `RAWINPUT` data.
    - Normalizes Extended Keys: Checks `RI_KEY_E0` or `RI_KEY_E1` flags and adds `1000` to the scan code if present.
    - Updates a global state (`g_lastKeypress`) with the timestamp and Device Handle of the exact physical keyboard that just fired.

## 4. The Decision (`WM_HIDEOUS_KEYBOARD_EVENT`)

Back in the main application (`src/app/ui/main_window.cpp`):

1.  **Event**: Receives the custom message from the Hook.
2.  **Context**: At this point, `WM_INPUT` has usually already fired (or is processed immediately before), so `g_lastKeypress` contains the ID of the device pressing the key.
3.  **Logic**:
    - `DecideOnKey(vk, scanCode)` is called.
    - It checks if `g_lastKeypress` matches a device that has a macro for this key.
    - If matched -> Execute Command -> Return `KEY_DECISION_BLOCK`.
    - If not matched -> Return `KEY_DECISION_LET_THROUGH`.

This hybrid approach allows HIDeous to have the power of global blocking (Hooks) with the precision of hardware differentiation (Raw Input).


## 5. Conceptual: Why a Hybrid Approach?

**Why not `WH_KEYBOARD_LL`?**

A Low Level Hook runs in the main application's context but blocks the *entire* system's input processing while waiting for a return. It has a strict timeout (OS enforced). If we used this, we would have to "guess" (race condition) if a matching `WM_INPUT` message has arrived in our queue before the hook callback fires. If the system is busy, we might miss it, leading to strict timeouts and Windows silently uninstalling our hook.

**Why `WH_KEYBOARD` (DLL)?**

A Standard Hook is injected into *each* application. The callback runs in the target app's thread. If that specific app hangs, it only delays input for that app window, not the whole system. We use IPC (`SendMessageTimeout`) to ask the main HIDeous app "Do you see a device match?". This is more robust because it avoids global system stalls and gives us a reliable synchronization point.