#include "hideous_hook.h"

// Global variables
namespace
{
    HHOOK g_keyboardHook = nullptr;
    HWND g_targetWindow = nullptr;
}

// DLL entry point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        break;

    case DLL_PROCESS_DETACH:
        if (g_keyboardHook)
        {
            UninstallHook();
        }
        break;
    }
    return TRUE;
}

// Install hook
HIDEOUS_API BOOL InstallHook(HWND targetWindow)
{
    if (!targetWindow || g_keyboardHook)
    {
        return FALSE;
    }

    g_targetWindow = targetWindow;

    // Install the keyboard hook
    g_keyboardHook = SetWindowsHookEx(
        WH_KEYBOARD,
        KeyboardProc,
        GetModuleHandle(TEXT("hideous_hook.dll")),
        0);

    return (g_keyboardHook != nullptr);
}

// Uninstall hook
HIDEOUS_API BOOL UninstallHook()
{
    if (!g_keyboardHook)
    {
        return FALSE;
    }

    BOOL result = UnhookWindowsHookEx(g_keyboardHook);
    if (result)
    {
        g_keyboardHook = nullptr;
        g_targetWindow = nullptr;
    }

    return result;
}

// Keyboard hook procedure
LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code < 0)
    {
        return CallNextHookEx(g_keyboardHook, code, wParam, lParam);
    }

    // Only process keydown events (bit 31 indicates key up)
    if (!(lParam & 0x80000000))
    {
        // Forward the keyboard event to the main application
        PostMessage(
            g_targetWindow,
            WM_HIDEOUS_KEYBOARD_EVENT,
            wParam, // Virtual key code
            lParam  // Key event information
        );
    }

    // Always call the next hook - the main application will decide
    // whether to block the keystroke based on WM_INPUT device matching
    return CallNextHookEx(g_keyboardHook, code, wParam, lParam);
}
