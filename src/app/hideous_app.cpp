#include <windows.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <format>
#include <hidusage.h>
#include "../../src/dll/hideous_hook.h"

// Function pointers for DLL functions
typedef BOOL (*InstallHookFn)(HWND);
typedef BOOL (*UninstallHookFn)();

namespace
{
    HMODULE g_hookDll = nullptr;
    InstallHookFn g_installHook = nullptr;
    UninstallHookFn g_uninstallHook = nullptr;

    // Get timestamp for message ordering
    std::string GetTimestamp()
    {
        SYSTEMTIME st;
        GetLocalTime(&st);
        return std::format("{:02d}:{:02d}:{:02d}.{:03d}",
                           st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    }
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
        UINT dataSize;
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &dataSize, sizeof(RAWINPUTHEADER));

        if (dataSize > 0)
        {
            std::vector<BYTE> rawData(dataSize);
            if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, rawData.data(), &dataSize,
                                sizeof(RAWINPUTHEADER)) == dataSize)
            {

                RAWINPUT *raw = (RAWINPUT *)rawData.data();
                if (raw->header.dwType == RIM_TYPEKEYBOARD)
                {
                    std::cout << GetTimestamp()
                              << " WM_INPUT - Device: " << std::hex << raw->header.hDevice
                              << " Key: 0x" << raw->data.keyboard.VKey
                              << " Flags: 0x" << raw->data.keyboard.Flags
                              << std::dec << std::endl;
                }
            }
        }
        break;
    }

    case WM_HIDEOUS_KEYBOARD_EVENT:
    {
        std::cout << GetTimestamp()
                  << " WM_HIDEOUS_KEYBOARD_EVENT - Key: 0x" << std::hex << wParam
                  << " lParam: 0x" << lParam << std::dec << std::endl;
        break;
    }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Create console for output
    AllocConsole();
    FILE *dummy;
    freopen_s(&dummy, "CONOUT$", "w", stdout);

    // Register window class
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = TEXT("HIDeous_Test");

    if (!RegisterClassEx(&wc))
    {
        MessageBox(nullptr, TEXT("Window Registration Failed"), TEXT("Error"), MB_ICONERROR);
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
        MessageBox(nullptr, TEXT("Window Creation Failed"), TEXT("Error"), MB_ICONERROR);
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
        MessageBox(nullptr, TEXT("Failed to register for raw input"), TEXT("Error"), MB_ICONERROR);
        return 1;
    }

    // Load the hook DLL
    g_hookDll = LoadLibrary(TEXT("hideous_hook.dll"));
    if (!g_hookDll)
    {
        MessageBox(nullptr, TEXT("Failed to load hook DLL"), TEXT("Error"), MB_ICONERROR);
        return 1;
    }

    // Get function pointers
    g_installHook = (InstallHookFn)GetProcAddress(g_hookDll, "InstallHook");
    g_uninstallHook = (UninstallHookFn)GetProcAddress(g_hookDll, "UninstallHook");

    if (!g_installHook || !g_uninstallHook)
    {
        MessageBox(nullptr, TEXT("Failed to get DLL function addresses"), TEXT("Error"), MB_ICONERROR);
        FreeLibrary(g_hookDll);
        return 1;
    }

    // Install the hook
    if (!g_installHook(hwnd))
    {
        MessageBox(nullptr, TEXT("Failed to install keyboard hook"), TEXT("Error"), MB_ICONERROR);
        FreeLibrary(g_hookDll);
        return 1;
    }

    std::cout << "Test application started. Press keys to see events.\n"
              << "Close the window to exit.\n"
              << "----------------------------------------\n";

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

    // Cleanup
    if (g_uninstallHook)
    {
        g_uninstallHook();
    }
    if (g_hookDll)
    {
        FreeLibrary(g_hookDll);
    }

    return 0;
}
