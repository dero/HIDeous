#include <windows.h>
#include <shlwapi.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <sstream>
#include <hidusage.h>
#include "../common/shared_data.h"
#include "../common/log_util.h"
#include "../dll/hideous_hook.h"

// Link with Shlwapi.lib
#pragma comment(lib, "Shlwapi.lib")

// Function pointers for DLL functions
typedef BOOL (*InstallHookFn)(HWND);
typedef BOOL (*UninstallHookFn)();

namespace
{
    HMODULE g_hookDll = nullptr;
    InstallHookFn g_installHook = nullptr;
    UninstallHookFn g_uninstallHook = nullptr;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_INPUT:
    {
        UINT dataSize = 0;
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dataSize, sizeof(RAWINPUTHEADER));

        if (dataSize > 0)
        {
            std::vector<BYTE> rawData(dataSize);
            if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, rawData.data(), &dataSize,
                                sizeof(RAWINPUTHEADER)) == dataSize)
            {

                RAWINPUT *raw = (RAWINPUT *)rawData.data();
                if (raw->header.dwType == RIM_TYPEKEYBOARD)
                {
                    std::ostringstream ss;
                    ss << "WM_INPUT - Device: 0x" << std::hex << (UINT64)raw->header.hDevice
                       << " Key: 0x" << raw->data.keyboard.VKey
                       << " Flags: 0x" << raw->data.keyboard.Flags << std::dec;
                    DebugLog(ss.str());
                }
            }
        }
        break;
    }

    case WM_HIDEOUS_KEYBOARD_EVENT:
    {
        std::ostringstream ss;
        ss << " WM_HIDEOUS_KEYBOARD_EVENT - Key: 0x" << std::hex << wParam
           << " lParam: 0x" << lParam << std::dec;
        DebugLog(ss.str());

        break;
    }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
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

    // Register window class
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = TEXT("HIDeous_Test");

    if (!RegisterClassEx(&wc))
    {
        DebugLog("Failed to register window class");
        return 1;
    }

    // Create window
    HWND hwnd = CreateWindow(
        TEXT("HIDeous_Test"), TEXT("HIDeous Test"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
        nullptr, nullptr, hInstance, nullptr);

    if (!hwnd)
    {
        DebugLog("Failed to create window");
        return 1;
    }

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
