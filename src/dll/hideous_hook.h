#pragma once

#include <windows.h>
#include <cstdint>

#ifdef HIDEOUS_HOOK_EXPORTS
#define HIDEOUS_API __declspec(dllexport)
#else
#define HIDEOUS_API __declspec(dllimport)
#endif

// Custom messages for communication with the main application
#define WM_HIDEOUS_KEYBOARD_EVENT (WM_APP + 1)

extern "C"
{
    HIDEOUS_API BOOL InstallHook(HWND targetWindow);
    HIDEOUS_API BOOL UninstallHook();
}
