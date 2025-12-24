#include "../common/config/settings_manager.h"
#include "../common/utils/logging.h"
#include "../common/utils/constants.h"
#include "hideous_hook.h"
#include <windows.h>
#include <fstream>
#include <sstream>

// Shared data section to share variables across all instances of the DLL
#pragma data_seg(".shared")
HHOOK g_keyboardHook = nullptr;
HWND g_mainWindow = nullptr;
BYTE g_interestedKeys[256] = {0}; // 0 = not interested, 1 = interested
#pragma data_seg()
#pragma comment(linker, "/SECTION:.shared,RWS")

extern "C" LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code != 0)
    {
        return CallNextHookEx(g_keyboardHook, code, wParam, lParam);
    }

    // Check for our own injected events
    if ((ULONG_PTR)GetMessageExtraInfo() == HIDEOUS_IDENTIFIER)
    {
        return CallNextHookEx(g_keyboardHook, code, wParam, lParam);
    }

    const Settings &settings = SettingsManager::getInstance().getSettings();

    // Only process keydown events
    if (!(lParam & 0x80000000))
    {
        if (settings.global.Debug)
        {
            // Log every hook call
            std::wostringstream ss;
            ss << "2️⃣ WH_KEYBOARD - code: " << code
               << ", wparam: 0x" << std::hex << static_cast<DWORD>(wParam) << std::dec
               << ", lparam: " << static_cast<DWORD>(lParam);

            DebugLog(ss.str());
        }

        // Log target window info
        DWORD targetProcessId = 0;
        GetWindowThreadProcessId(g_mainWindow, &targetProcessId);

        // Try to verify if window still exists
        if (!IsWindow(g_mainWindow))
        {
            DebugLog(L"Target window is no longer valid!");
            return CallNextHookEx(g_keyboardHook, code, wParam, lParam);
        }

        // Check if we are interested in this key
        // wParam is the virtual key code
        if (wParam < 256 && g_interestedKeys[wParam] == 0)
        {
            return CallNextHookEx(g_keyboardHook, code, wParam, lParam);
        }

        DWORD_PTR decision = KEY_DECISION_LET_THROUGH;

        LRESULT sendResult = SendMessageTimeout(
            g_mainWindow,
            WM_HIDEOUS_KEYBOARD_EVENT,
            wParam,
            lParam,
            SMTO_ABORTIFHUNG | SMTO_NORMAL,
            settings.global.KeyWaitTime,
            &decision);

        if (!sendResult)
        {
            if (settings.global.Debug)
            {
                DWORD error = GetLastError();
                std::wostringstream errss;
                errss << "SendMessageTimeout failed with error: " << error;
                DebugLog(errss.str());
            }
        }
        else
        {
            DebugLog(L"SendMessageTimeout succeeded");
        }

        if (decision == KEY_DECISION_BLOCK)
        {
            return 1;
        }
    }

    return CallNextHookEx(g_keyboardHook, code, wParam, lParam);
}

HIDEOUS_API BOOL InstallHook(HWND hwnd)
{
    if (!IsWindow(hwnd))
    {
        DebugLog(L"Invalid window handle passed to InstallHook");
        return FALSE;
    }

    g_mainWindow = hwnd;

    const Settings &settings = SettingsManager::getInstance().getSettings();

    // Log initial hook installation
    if (settings.global.Debug)
    {
        // Log information about the target window
        DWORD targetProcessId = 0;
        DWORD targetThreadId = GetWindowThreadProcessId(g_mainWindow, &targetProcessId);

        std::wostringstream ss;
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
        if (settings.global.Debug)
        {
            DWORD error = GetLastError();
            std::wostringstream errss;
            errss << "SetWindowsHookEx failed with error: " << error;
            DebugLog(errss.str());
        }
        return FALSE;
    }

    DebugLog(L"Hook installed successfully");
    return TRUE;
}

HIDEOUS_API BOOL UninstallHook()
{
    if (!g_keyboardHook)
    {
        return FALSE;
    }

    const Settings &settings = SettingsManager::getInstance().getSettings();

    BOOL result = UnhookWindowsHookEx(g_keyboardHook);
    if (result)
    {
        DebugLog(L"Hook uninstalled successfully");
        g_keyboardHook = nullptr;
    }
    else
    {
        if (settings.global.Debug)
        {
            DWORD error = GetLastError();
            std::wostringstream ss;
            ss << "UnhookWindowsHookEx failed with error: " << error;
            DebugLog(ss.str());
        }
    }

    return result;
}

HIDEOUS_API void UpdateInterestedKeys(BYTE *keys)
{
    if (keys)
    {
        memcpy(g_interestedKeys, keys, 256);
        DebugLog(L"Interested keys updated");
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
    {
        const Settings &settings = SettingsManager::getInstance().getSettings();

        DisableThreadLibraryCalls(hModule);

        WCHAR processPath[MAX_PATH];
        GetModuleFileNameW(NULL, processPath, MAX_PATH);

        DebugLog(L"DLL loaded into process: " + std::wstring(processPath));
        DebugLog(L"Main window handle: 0x" + std::to_wstring((DWORD)(UINT_PTR)g_mainWindow));
        DebugLog(L"Debug mode: " + std::to_wstring(settings.global.Debug));

        break;
    }
    case DLL_PROCESS_DETACH:
        DebugLog(L"DLL unloaded");
        if (g_keyboardHook)
        {
            UninstallHook();
        }
        break;
    }
    return TRUE;
}
