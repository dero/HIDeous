#include "../common/settings.h"
#include "../common/logging.h"
#include "../common/constants.h"
#include "hideous_hook.h"
#include <windows.h>
#include <fstream>
#include <sstream>

HHOOK g_keyboardHook = nullptr;
HWND g_mainWindow = nullptr;

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    wchar_t className[256];

    // Get the class name of the window
    GetClassName(hwnd, className, sizeof(className));

    // Check if the class name matches the one used by your application
    if (wcscmp(className, L"HIDeous") == 0)
    {
        // Found the main window handle
        g_mainWindow = hwnd;
        return FALSE; // Stop enumeration
    }

    return TRUE; // Continue enumeration
}

extern "C" LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code < 0)
    {
        return CallNextHookEx(g_keyboardHook, code, wParam, lParam);
    }

    if (getSettings().global.Debug)
    {
        // Log every hook call
        std::ostringstream ss;
        ss << "WH_KEYBOARD - code: " << code
           << ", wparam: " << static_cast<DWORD>(wParam)
           << ", lparam: " << static_cast<DWORD>(lParam);

        DebugLog(ss.str());
    }

    // Only process keydown events
    if (!(lParam & 0x80000000))
    {
        // Log target window info
        DWORD targetProcessId = 0;
        GetWindowThreadProcessId(g_mainWindow, &targetProcessId);

        if (getSettings().global.Debug)
        {
            std::ostringstream windowss;
            windowss << "Attempting PostMessage to window 0x"
                     << std::hex << (DWORD)(UINT_PTR)g_mainWindow
                     << " in process " << std::dec << targetProcessId;
            DebugLog(windowss.str());
        }

        // Try to verify if window still exists
        if (!IsWindow(g_mainWindow))
        {
            DebugLog("Target window is no longer valid!");
            return CallNextHookEx(g_keyboardHook, code, wParam, lParam);
        }

        DWORD_PTR decision = KEY_DECISION_LET_THROUGH;

        LRESULT sendResult = SendMessageTimeout(
            g_mainWindow,
            WM_HIDEOUS_KEYBOARD_EVENT,
            wParam,
            lParam,
            SMTO_ABORTIFHUNG | SMTO_NORMAL,
            getSettings().global.KeyWaitTime,
            &decision);

        if (!sendResult)
        {
            if (getSettings().global.Debug)
            {
                DWORD error = GetLastError();
                std::ostringstream errss;
                errss << "SendMessageTimeout failed with error: " << error;
                DebugLog(errss.str());
            }
        }
        else
        {
            DebugLog("PostMessage succeeded");
        }

        if (decision == KEY_DECISION_BLOCK)
        {
            return 1;
        }
    }

    return CallNextHookEx(g_keyboardHook, code, wParam, lParam);
}

HIDEOUS_API BOOL InstallHook()
{
    EnumWindows(EnumWindowsProc, NULL);

    if (g_mainWindow == nullptr)
    {
        DebugLog("Main window not found, aborting hook installation");
        return FALSE;
    }

    // Log initial hook installation
    if (getSettings().global.Debug)
    {
        // Log information about the target window
        DWORD targetProcessId = 0;
        DWORD targetThreadId = GetWindowThreadProcessId(g_mainWindow, &targetProcessId);

        std::ostringstream ss;
        ss << "Installing hook with target window 0x" << std::hex << (DWORD)(UINT_PTR)g_mainWindow
           << " in process " << std::dec << targetProcessId
           << " thread " << targetThreadId;
        DebugLog(ss.str());
    }

    g_keyboardHook = SetWindowsHookEx(
        WH_KEYBOARD,
        KeyboardProc,
        GetModuleHandle(TEXT("hideous_hook.dll")),
        0 // Hook all threads
    );

    if (!g_keyboardHook)
    {
        if (getSettings().global.Debug)
        {
            DWORD error = GetLastError();
            std::ostringstream errss;
            errss << "SetWindowsHookEx failed with error: " << error;
            DebugLog(errss.str());
        }
        return FALSE;
    }

    DebugLog("Hook installed successfully");
    return TRUE;
}

HIDEOUS_API BOOL UninstallHook()
{
    if (!g_keyboardHook)
    {
        return FALSE;
    }

    BOOL result = UnhookWindowsHookEx(g_keyboardHook);
    if (result)
    {
        DebugLog("Hook uninstalled successfully");
        g_keyboardHook = nullptr;
    }
    else
    {
        if (getSettings().global.Debug)
        {
            DWORD error = GetLastError();
            std::ostringstream ss;
            ss << "UnhookWindowsHookEx failed with error: " << error;
            DebugLog(ss.str());
        }
    }

    return result;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
    {
        DisableThreadLibraryCalls(hModule);

        // Find the main window handle every time the DLL is loaded
        EnumWindows(EnumWindowsProc, NULL);

        WCHAR processPath[MAX_PATH];
        GetModuleFileNameW(NULL, processPath, MAX_PATH);

        DebugLog("DLL loaded into process: " + WideToNarrow(processPath));
        DebugLog("Main window handle: 0x" + std::to_string((DWORD)(UINT_PTR)g_mainWindow));
        DebugLog("Debug mode: " + std::to_string(getSettings().global.Debug));

        break;
    }
    case DLL_PROCESS_DETACH:
        DebugLog("DLL unloaded");
        if (g_keyboardHook)
        {
            UninstallHook();
        }
        break;
    }
    return TRUE;
}
