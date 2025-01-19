// src/dll/hideous_hook.cpp
#include "../common/shared_data.h"
#include "hideous_hook.h"
#include "../common/log_util.h"
#include <windows.h>
#include <fstream>
#include <sstream>

namespace
{
    HHOOK g_keyboardHook = nullptr;
    DWORD g_hookThreadId = 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
    {
        DisableThreadLibraryCalls(hModule);
        WCHAR processPath[MAX_PATH];
        GetModuleFileNameW(NULL, processPath, MAX_PATH);

        DebugLog("DLL loaded into process: " + WideToNarrow(processPath));

        if (!InitializeSharedMemory())
        {
            DebugLog("Failed to initialize shared memory");
            return FALSE;
        }
        break;
    }
    case DLL_PROCESS_DETACH:
        DebugLog("DLL unloaded");
        CleanupSharedMemory();
        if (g_keyboardHook)
        {
            UninstallHook();
        }
        break;
    }
    return TRUE;
}

extern "C" LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code < 0 || !g_shared)
    {
        return CallNextHookEx(g_keyboardHook, code, wParam, lParam);
    }

    if (g_shared->isDebugMode)
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
        HWND targetWnd = g_shared->mainWindow;

        // Log target window info
        DWORD targetProcessId = 0;
        GetWindowThreadProcessId(targetWnd, &targetProcessId);

        if (g_shared->isDebugMode)
        {
            std::ostringstream windowss;
            windowss << "Attempting PostMessage to window 0x"
                     << std::hex << (DWORD)(UINT_PTR)targetWnd
                     << " in process " << std::dec << targetProcessId;
            DebugLog(windowss.str());
        }

        // Try to verify if window still exists
        if (!IsWindow(targetWnd))
        {
            DebugLog("Target window is no longer valid!");
            return CallNextHookEx(g_keyboardHook, code, wParam, lParam);
        }

        BOOL result = PostMessage(
            targetWnd,
            WM_HIDEOUS_KEYBOARD_EVENT,
            wParam,
            lParam);

        if (!result)
        {
            if (g_shared->isDebugMode)
            {
                DWORD error = GetLastError();
                std::ostringstream errss;
                errss << "PostMessage failed with error: " << error;
                DebugLog(errss.str());
            }
        }
        else
        {
            DebugLog("PostMessage succeeded");
        }
    }

    return CallNextHookEx(g_keyboardHook, code, wParam, lParam);
}

HIDEOUS_API BOOL InstallHook(HWND mainWindow, const WCHAR *logPath, BOOL isDebugMode)
{
    if (!mainWindow || g_keyboardHook || !g_shared)
    {
        return FALSE;
    }

    // Store the configuration in shared memory
    g_shared->mainWindow = mainWindow;
    g_shared->isDebugMode = isDebugMode;

    if (isDebugMode && logPath)
    {
        wcscpy_s(g_shared->logPath, MAX_PATH, logPath);
    }

    // Log initial hook installation
    if (isDebugMode)
    {
        // Log information about the target window
        DWORD targetProcessId = 0;
        DWORD targetThreadId = GetWindowThreadProcessId(mainWindow, &targetProcessId);

        std::ostringstream ss;
        ss << "Installing hook with target window 0x" << std::hex << (DWORD)(UINT_PTR)mainWindow
           << " in process " << std::dec << targetProcessId
           << " thread " << targetThreadId;
        DebugLog(ss.str());
    }

    g_hookThreadId = GetCurrentThreadId();

    g_keyboardHook = SetWindowsHookEx(
        WH_KEYBOARD,
        KeyboardProc,
        GetModuleHandle(TEXT("hideous_hook.dll")),
        0 // Hook all threads
    );

    if (!g_keyboardHook)
    {
        if (isDebugMode)
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
        g_shared->mainWindow = nullptr;
        g_hookThreadId = 0;
    }
    else
    {
        if (g_shared->isDebugMode)
        {
            DWORD error = GetLastError();
            std::ostringstream ss;
            ss << "UnhookWindowsHookEx failed with error: " << error;
            DebugLog(ss.str());
        }
    }

    return result;
}

HIDEOUS_API DWORD GetHookThreadId()
{
    return g_hookThreadId;
}
