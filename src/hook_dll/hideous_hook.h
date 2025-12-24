// src/dll/hideous_hook.h
#pragma once

#include <windows.h>
#include <cstdint>

#ifdef HIDEOUS_HOOK_EXPORTS
#define HIDEOUS_API __declspec(dllexport)
#else
#define HIDEOUS_API __declspec(dllimport)
#endif

#define WM_HIDEOUS_KEYBOARD_EVENT (WM_APP + 1)

extern "C"
{
    HIDEOUS_API LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam);
    HIDEOUS_API BOOL InstallHook(HWND hwnd);
    HIDEOUS_API BOOL UninstallHook();
    HIDEOUS_API void UpdateInterestedKeys(BYTE *keys, BYTE *scanCodes);
}
