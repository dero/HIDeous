#include <windows.h>
#include <shlwapi.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <sstream>
#include <hidusage.h>
#include <wincrypt.h>
#include <commctrl.h>
#include "resource.h"
#include "../common/settings.h"
#include "intercept.h"
#include "../common/crypto.h"
#include "../common/logging.h"
#include "../dll/hideous_hook.h"
#include "gui.h"

// Link with Shlwapi.lib and Comctl32.lib
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Comctl32.lib")

// Function pointers for DLL functions
typedef BOOL (*InstallHookFn)(HWND);
typedef BOOL (*UninstallHookFn)();

/**
 * Structure to hold the last keypress information.
 *
 * - timestamp: GetTickCount() value when the key was pressed
 * - deviceHash: Hash of the device name
 * - vkCode: Virtual key code of the key pressed
 */
LastKeypress g_lastKeypress = {0, L"", 0};
HMODULE g_hookDll = nullptr;
InstallHookFn g_installHook = nullptr;
UninstallHookFn g_uninstallHook = nullptr;

bool InstallGlobalHook()
{

    HMODULE hDll = LoadLibrary(TEXT("hideous_hook.dll"));
    if (!hDll)
    {
        return false;
    }

    auto fnInstallHook = (BOOL(*)())GetProcAddress(hDll, "InstallHook");
    if (!fnInstallHook)
    {
        FreeLibrary(hDll);
        return false;
    }

    if (!fnInstallHook())
    {
        FreeLibrary(hDll);
        return false;
    }

    return true;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Make sure the settings path is set on every run.
    wchar_t processPath[MAX_PATH];
    GetModuleFileName(NULL, processPath, MAX_PATH);
    PathRemoveFileSpec(processPath);

    persistSettingsPath(processPath);

    // Icons
    HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));
    HICON hIconSm = (HICON)LoadImage(hInstance,
                                     MAKEINTRESOURCE(IDI_APPICON),
                                     IMAGE_ICON,
                                     GetSystemMetrics(SM_CXSMICON),
                                     GetSystemMetrics(SM_CYSMICON),
                                     LR_DEFAULTCOLOR);

    if (!hIcon)
    {
        DWORD error = GetLastError();
        WCHAR msg[256];
        swprintf_s(msg, L"Failed to load large icon. Error: %lu", error);
        MessageBox(nullptr, msg, TEXT("Error"), MB_OK | MB_ICONERROR);
        return 1;
    }

    // Register window class
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = TEXT("HIDeous");
    wc.hIcon = hIcon;
    wc.hIconSm = hIconSm;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassEx(&wc))
    {
        MessageBox(nullptr, TEXT("Failed to register window class"), TEXT("Error"), MB_OK | MB_ICONERROR);
        return 1;
    }

    // Create window with fixed size
    HWND hwnd = CreateWindow(
        TEXT("HIDeous"), TEXT("HIDeous"),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 740, 400,
        nullptr, nullptr, hInstance, nullptr);

    if (!hwnd)
    {
        MessageBox(nullptr, TEXT("Failed to create window"), TEXT("Error"), MB_OK | MB_ICONERROR);
        return 1;
    }

    DebugLog(L"HIDeous started");

    // Set window icons
    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSm);

    // Create device table
    HWND hList = CreateDeviceTable(hwnd);

    // Create the checkbox for running on startup
    CreateStartupCheckbox(hwnd);

    // Create the button to open help
    CreateHelpButton(hwnd);

    // Populate table
    UpdateDeviceTable(hList);

    // Register for raw input
    RAWINPUTDEVICE rid;
    rid.usUsagePage = HID_USAGE_PAGE_GENERIC;
    rid.usUsage = HID_USAGE_GENERIC_KEYBOARD;
    rid.dwFlags = RIDEV_INPUTSINK;
    rid.hwndTarget = hwnd;

    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid)))
    {
        DebugLog(L"Failed to register for raw input");
        return 1;
    }

    // Load the hook DLL
    if (!InstallGlobalHook())
    {
        DebugLog(L"Failed to install global keyboard hook");
        return 1;
    }

    // Show window
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Message loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
