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
#include "settings.h"
#include "../common/shared.h"
#include "../common/crypto.h"
#include "../common/logging.h"
#include "../dll/hideous_hook.h"
// Link with Shlwapi.lib and Comctl32.lib
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Comctl32.lib")
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

// Add this helper function at global/namespace scope to find list item by device handle
int FindDeviceListItem(HWND hList, HANDLE hDevice)
{
    WCHAR deviceName[256];
    UINT nameSize = sizeof(deviceName);

    if (GetRawInputDeviceInfo(hDevice, RIDI_DEVICENAME, deviceName, &nameSize) == (UINT)-1)
    {
        return -1;
    }

    int itemCount = ListView_GetItemCount(hList);
    for (int i = 0; i < itemCount; i++)
    {
        WCHAR itemText[256];
        ListView_GetItemText(hList, i, 0, itemText, 256);
        if (wcscmp(deviceName, itemText) == 0)
        {
            return i;
        }
    }
    return -1;
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
                    // Debug logging
                    std::ostringstream ss;
                    ss << "WM_INPUT - Device: 0x" << std::hex << (UINT64)raw->header.hDevice
                       << " Key: 0x" << raw->data.keyboard.VKey
                       << " Flags: 0x" << raw->data.keyboard.Flags << std::dec;
                    DebugLog(ss.str());

                    // Find the device in the list
                    HWND hList = GetDlgItem(hwnd, 0); // Assuming the list is the first child window
                    int itemIndex = FindDeviceListItem(hList, raw->header.hDevice);

                    if (itemIndex >= 0)
                    {
                        // Key pressed or released
                        if (raw->data.keyboard.Flags & RI_KEY_BREAK)
                        {
                            // Key released - clear the text
                            ListView_SetItemText(hList, itemIndex, 3, L"");
                        }
                        else
                        {
                            // Key pressed - show VK code
                            WCHAR keyText[16];
                            swprintf_s(keyText, L"0x%02X", raw->data.keyboard.VKey);
                            ListView_SetItemText(hList, itemIndex, 3, keyText);
                        }
                    }
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

// Add this struct to hold device info
struct KeyboardDevice
{
    std::wstring fullName;
    std::string hash;
    std::string userLabel; // From settings.ini
};

/**
 * Get a list of keyboard devices.
 *
 * @param settings The settings object.
 * @return A vector of keyboard devices.
 */
std::vector<KeyboardDevice> GetKeyboardDevices(const Settings &settings)
{
    std::vector<KeyboardDevice> devices;
    UINT nDevices;
    PRAWINPUTDEVICELIST pRawInputDeviceList;

    if (GetRawInputDeviceList(NULL, &nDevices, sizeof(RAWINPUTDEVICELIST)) != 0)
    {
        std::cerr << "Error getting number of devices." << std::endl;
        return devices;
    }

    pRawInputDeviceList = new RAWINPUTDEVICELIST[nDevices];
    if (GetRawInputDeviceList(pRawInputDeviceList, &nDevices, sizeof(RAWINPUTDEVICELIST)) == -1)
    {
        std::cerr << "Error getting device list." << std::endl;
        delete[] pRawInputDeviceList;
        return devices;
    }

    // Get device mappings to find user labels
    const auto &deviceMappings = settings.getDevices();

    for (UINT i = 0; i < nDevices; i++)
    {
        if (pRawInputDeviceList[i].dwType == RIM_TYPEKEYBOARD)
        {
            UINT cbSize = 0;
            GetRawInputDeviceInfo(pRawInputDeviceList[i].hDevice, RIDI_DEVICENAME, NULL, &cbSize);
            if (cbSize > 0)
            {
                std::wstring deviceName(cbSize * 2, L'\0');
                if (GetRawInputDeviceInfo(pRawInputDeviceList[i].hDevice, RIDI_DEVICENAME, &deviceName[0], &cbSize) > 0)
                {
                    KeyboardDevice dev;
                    dev.fullName = deviceName;
                    dev.hash = GetShortHash(deviceName);

                    // Find user label if it exists
                    dev.userLabel = "Unknown";
                    for (const auto &mapping : deviceMappings)
                    {
                        if (mapping.second == dev.hash)
                        {
                            dev.userLabel = mapping.first;
                            break;
                        }
                    }

                    devices.push_back(dev);
                }
            }
        }
    }

    delete[] pRawInputDeviceList;
    return devices;
}

HWND CreateDeviceTable(HWND parent)
{
    // Create list view control with additional styles
    HWND hList = CreateWindowEx(0, WC_LISTVIEW, NULL,
                                WS_CHILD | WS_VISIBLE | LVS_REPORT | LBS_HASSTRINGS | LVS_SHOWSELALWAYS,
                                10, 10, 500, 300, parent, NULL, GetModuleHandle(NULL), NULL);

    // Set full row select and grid lines
    ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    // Add columns
    LVCOLUMN lvc;
    lvc.mask = LVCF_TEXT | LVCF_WIDTH;

    lvc.pszText = L"Device Name";
    lvc.cx = 200;
    ListView_InsertColumn(hList, 0, &lvc);

    lvc.pszText = L"Device Hash";
    lvc.cx = 100;
    ListView_InsertColumn(hList, 1, &lvc);

    lvc.pszText = L"User Label";
    lvc.cx = 150;
    ListView_InsertColumn(hList, 2, &lvc);

    lvc.pszText = L"Key";
    lvc.cx = 50;
    ListView_InsertColumn(hList, 3, &lvc);

    return hList;
}

// Add this to window procedure
void UpdateDeviceTable(HWND hList, const Settings &settings)
{
    ListView_DeleteAllItems(hList);

    auto devices = GetKeyboardDevices(settings);

    for (const auto &dev : devices)
    {
        LVITEM lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = ListView_GetItemCount(hList);

        // Convert wstring to LPWSTR for display
        lvi.pszText = const_cast<LPWSTR>(dev.fullName.c_str());
        int pos = ListView_InsertItem(hList, &lvi);

        std::wstring hashW(dev.hash.begin(), dev.hash.end());
        ListView_SetItemText(hList, pos, 1, const_cast<LPWSTR>(hashW.c_str()));

        std::wstring userLabelW(dev.userLabel.begin(), dev.userLabel.end());
        ListView_SetItemText(hList, pos, 2, const_cast<LPWSTR>(userLabelW.c_str()));
    }
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

    // Create window with fixed size
    HWND hwnd = CreateWindow(
        TEXT("HIDeous_Test"), TEXT("HIDeous Test"),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 540, 300,
        nullptr, nullptr, hInstance, nullptr);

    if (!hwnd)
    {
        DebugLog("Failed to create window");
        return 1;
    }

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
