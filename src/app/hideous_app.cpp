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
#include "settings.h"
#include "../common/shared.h"
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

namespace
{
    HMODULE g_hookDll = nullptr;
    InstallHookFn g_installHook = nullptr;
    UninstallHookFn g_uninstallHook = nullptr;
}

bool InstallGlobalHook(HWND hwnd, bool isDebugMode)
{
    WCHAR logPath[MAX_PATH] = {0};

    if (isDebugMode)
    {
        // Get the executable's directory
        GetModuleFileName(NULL, logPath, MAX_PATH);
        PathRemoveFileSpec(logPath);
        PathAppend(logPath, L"debug.log");
    }

    HMODULE hDll = LoadLibrary(TEXT("hideous_hook.dll"));
    if (!hDll)
    {
        return false;
    }

    auto fnInstallHook = (BOOL(*)(HWND, const WCHAR *, BOOL))GetProcAddress(hDll, "InstallHook");
    if (!fnInstallHook)
    {
        FreeLibrary(hDll);
        return false;
    }

    if (!fnInstallHook(hwnd, isDebugMode ? logPath : nullptr, isDebugMode))
    {
        FreeLibrary(hDll);
        return false;
    }

    return true;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Parse command line arguments
    std::vector<std::string> args;
    std::istringstream iss(lpCmdLine);
    std::string arg;

    while (iss >> arg)
    {
        args.push_back(arg);
    }

    bool isDebugMode = false;
    for (const auto &arg : args)
    {
        if (arg == "--debug")
        {
            isDebugMode = true;
            break;
        }
    }

    InitializeSharedMemory();

    // Icons
    HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));
    HICON hIconSm = (HICON)LoadImage(hInstance,
                                     MAKEINTRESOURCE(IDI_APPICON),
                                     IMAGE_ICON,
                                     GetSystemMetrics(SM_CXSMICON),
                                     GetSystemMetrics(SM_CYSMICON),
                                     LR_DEFAULTCOLOR);

    // Register window class
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = TEXT("HIDeous_Test");
    wc.hIcon = hIcon;
    wc.hIconSm = hIconSm;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassEx(&wc))
    {
        DebugLog("Failed to register window class");
        return 1;
    }

    // Create window with fixed size
    HWND hwnd = CreateWindow(
        TEXT("HIDeous_Test"), TEXT("HIDeous Test"),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 640, 300,
        nullptr, nullptr, hInstance, nullptr);

    if (!hwnd)
    {
        DebugLog("Failed to create window");
        return 1;
    }

    // Set window icons
    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSm);

    // Create device table
    HWND hList = CreateDeviceTable(hwnd);

    // Load settings
    Settings settings;
    settings.load();

    // Populate table
    UpdateDeviceTable(hList, settings);

    // Register for raw input
    RAWINPUTDEVICE rid;
    rid.usUsagePage = HID_USAGE_PAGE_GENERIC;
    rid.usUsage = HID_USAGE_GENERIC_KEYBOARD;
    rid.dwFlags = RIDEV_INPUTSINK;
    rid.hwndTarget = hwnd;

    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid)))
    {
        DebugLog("Failed to register for raw input");
        return 1;
    }

    // Load the hook DLL
    if (!InstallGlobalHook(hwnd, isDebugMode))
    {
        DebugLog("Failed to install global keyboard hook");
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

    CleanupSharedMemory();

    return 0;
}
