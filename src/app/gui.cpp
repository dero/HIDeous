#include "gui.h"
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
#include <shellapi.h>
#include "resource.h"
#include "../common/settings.h"
#include "intercept.h"
#include "../common/crypto.h"
#include "../common/logging.h"
#include "../dll/hideous_hook.h"

// Link with Shlwapi.lib and Comctl32.lib
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Comctl32.lib")

// Tray icon
NOTIFYICONDATA g_trayIcon = {0};

void CreateTrayIcon(HWND hwnd)
{
	g_trayIcon.cbSize = sizeof(NOTIFYICONDATA);
	g_trayIcon.hWnd = hwnd;
	g_trayIcon.uID = 1;
	Shell_NotifyIcon(NIM_DELETE, &g_trayIcon);

	g_trayIcon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	g_trayIcon.uCallbackMessage = WM_TRAYICON;
	g_trayIcon.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APPICON));

	wcscpy_s(g_trayIcon.szTip, L"HIDeous");
	Shell_NotifyIcon(NIM_ADD, &g_trayIcon);
}

void CreateStartupCheckbox(HWND hwnd)
{
	// Read the registry key to see if the app is set to run on startup
	HKEY hKey;
	bool runsOnStartup = false;

	if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
					 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		WCHAR path[MAX_PATH];
		DWORD size = MAX_PATH;
		if (RegQueryValueEx(hKey, L"HIDeous", NULL, NULL, (LPBYTE)path, &size) == ERROR_SUCCESS)
		{
			runsOnStartup = true;
		}
		RegCloseKey(hKey);
	}

	RECT clientRect;
	GetClientRect(hwnd, &clientRect);

	int padding = 10;
	int checkboxHeight = 30;
	int checkboxWidth = 150;

	int checkboxX = padding;
	int checkboxY = clientRect.bottom - checkboxHeight;

	CreateWindowEx(0,
				   L"STATIC", nullptr,
				   WS_VISIBLE | WS_CHILD | SS_SUNKEN,
				   0, checkboxY - 1,
				   clientRect.right - clientRect.left, checkboxHeight + 1,
				   hwnd, nullptr, GetModuleHandle(NULL), NULL);

	CreateWindowEx(0, L"BUTTON", L"Run on startup",
				   WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
				   checkboxX, checkboxY, checkboxWidth, checkboxHeight,
				   hwnd, (HMENU)IDC_RUN_ON_STARTUP, GetModuleHandle(NULL), NULL);

	SendMessage(GetDlgItem(hwnd, IDC_RUN_ON_STARTUP), BM_SETCHECK, runsOnStartup, 0);
}

void RemoveTrayIcon()
{
	Shell_NotifyIcon(NIM_DELETE, &g_trayIcon);
}

std::wstring GetListViewCellText(HWND hList, int row, int col)
{
	WCHAR buffer[512] = {0};
	ListView_GetItemText(hList, row, col, buffer, 512);
	return std::wstring(buffer);
}

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

void StartupCallback(BOOL isChecked)
{
	HKEY hKey;

	if (isChecked)
	{
		// Add the registry key to run on startup
		if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
						 0, KEY_WRITE, &hKey) == ERROR_SUCCESS)
		{
			WCHAR path[MAX_PATH];
			GetModuleFileName(NULL, path, MAX_PATH);
			RegSetValueEx(hKey, L"HIDeous", 0, REG_SZ, (BYTE *)path, (DWORD)(wcslen(path) + 1) * sizeof(WCHAR));
			RegCloseKey(hKey);

			DebugLog(L"🚀 Run on startup enabled");
		}
	}
	else
	{
		// Remove the registry key
		RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
					 0, KEY_WRITE, &hKey);
		RegDeleteValue(hKey, L"HIDeous");
		RegCloseKey(hKey);

		DebugLog(L"🚀 Run on startup disabled");
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
					// Find the device in the list
					HWND hList = GetDlgItem(hwnd, IDC_MAIN_LIST);
					int itemIndex = FindDeviceListItem(hList, raw->header.hDevice);

					if (itemIndex >= 0)
					{
						// Get device hash
						std::wstring deviceHash = GetListViewCellText(hList, itemIndex, 1);

						if (!(raw->data.keyboard.Flags & RI_KEY_BREAK))
						{
							// Log the key press, only on key down
							std::wostringstream ss;
							ss << "1️⃣ WM_INPUT - Device: 0x" << std::hex << (UINT64)raw->header.hDevice
							   << " Key: 0x" << raw->data.keyboard.VKey
							   << " Flags: 0x" << raw->data.keyboard.Flags << std::dec;
							DebugLog(ss.str());

							// Update last keypress info
							g_lastKeypress.timestamp = GetTickCount();
							g_lastKeypress.deviceHash = deviceHash;
							g_lastKeypress.vkCode = raw->data.keyboard.VKey;

							// Key pressed - show VK code
							std::wstring keyName = virtualKeyCodeToString(raw->data.keyboard.VKey);

							// L"0x%02X (%hs)"
							WCHAR keyText[256];
							swprintf_s(keyText, L"0x%02X (%s)", raw->data.keyboard.VKey, keyName.c_str());

							ListView_SetItemText(hList, itemIndex, 3, keyText);
						}
						else
						{
							// Key released - clear the text
							ListView_SetItemText(hList, itemIndex, 3, L"");
						}
					}
				}
			}
		}
		break;
	}

	case WM_HIDEOUS_KEYBOARD_EVENT:
	{
		std::wostringstream ss;
		ss << "3️⃣ WM_HIDEOUS_KEYBOARD_EVENT - Key: 0x" << std::hex << wParam
		   << " lParam: 0x" << lParam << std::dec;
		DebugLog(ss.str());

		// `wParam` is the virtual key code
		return DecideOnKey(wParam);

		break;
	}

	case WM_CONTEXTMENU:
	{
		HWND hList = GetDlgItem(hwnd, IDC_MAIN_LIST);
		if ((HWND)wParam == hList)
		{
			POINT pt = {LOWORD(lParam), HIWORD(lParam)};

			HMENU hPopup = CreatePopupMenu();
			AppendMenu(hPopup, MF_STRING, IDM_COPY_CELL, L"Copy Device Hash");

			TrackPopupMenu(hPopup, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
			DestroyMenu(hPopup);
		}
		break;
	}

	case WM_CREATE:
		CreateTrayIcon(hwnd);

		return 0;

	case WM_CLOSE:
		RemoveTrayIcon();
		DestroyWindow(hwnd);
		return 0;

	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED)
		{
			ShowWindow(hwnd, SW_HIDE);
			return 0;
		}
		break;

	case WM_QUERYENDSESSION:
		RemoveTrayIcon();
		return TRUE;

	case WM_ENDSESSION:
		if (wParam == TRUE)
		{
			RemoveTrayIcon();
		}
		return 0;

	case WM_TRAYICON:
		if (LOWORD(lParam) == WM_RBUTTONUP)
		{
			POINT pt;
			GetCursorPos(&pt);
			HMENU hMenu = CreatePopupMenu();
			AppendMenu(hMenu, MF_STRING, IDM_RESTORE, L"Restore");
			AppendMenu(hMenu, MF_STRING, IDM_EXIT, L"Exit");
			SetForegroundWindow(hwnd);
			TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
			DestroyMenu(hMenu);
		}
		else if (LOWORD(lParam) == WM_LBUTTONDBLCLK)
		{
			ShowWindow(hwnd, SW_RESTORE);
			SetForegroundWindow(hwnd);
		}
		return 0;

	case WM_COMMAND:
	{
		HWND hList = GetDlgItem(hwnd, IDC_MAIN_LIST);
		switch (LOWORD(wParam))
		{
		case IDM_RESTORE:
		{
			ShowWindow(hwnd, SW_RESTORE);
			SetForegroundWindow(hwnd);
			break;
		}

		case IDM_EXIT:
		{
			RemoveTrayIcon();
			DestroyWindow(hwnd);
			break;
		}

		case IDM_COPY_CELL:
		{
			int selRow = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
			if (selRow >= 0)
			{
				std::wstring cellText = GetListViewCellText(hList, selRow, 1);
				if (OpenClipboard(hwnd))
				{
					EmptyClipboard();
					size_t size = (cellText.length() + 1) * sizeof(wchar_t);
					HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, size);
					if (hGlobal)
					{
						memcpy(GlobalLock(hGlobal), cellText.c_str(), size);
						GlobalUnlock(hGlobal);
						SetClipboardData(CF_UNICODETEXT, hGlobal);
					}
					CloseClipboard();
				}
			}
			break;
		}

		case IDC_RUN_ON_STARTUP:
		{
			if (HIWORD(wParam) == BN_CLICKED)
			{
				BOOL isChecked = SendMessage((HWND)lParam, BM_GETCHECK, 0, 0);
				StartupCallback(isChecked);
			}
			break;
		}
		}
		break;
	}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

HWND CreateDeviceTable(HWND parent)
{
	HWND hList = CreateWindowEx(0, WC_LISTVIEW, NULL,
								WS_CHILD | WS_VISIBLE | LVS_REPORT | LBS_HASSTRINGS | LVS_SHOWSELALWAYS,
								10, 10, 700, 300, parent,
								(HMENU)IDC_MAIN_LIST, GetModuleHandle(NULL), NULL);

	// Add styles for better selection and copying
	ListView_SetExtendedListViewStyle(hList,
									  LVS_EX_FULLROWSELECT |
										  LVS_EX_GRIDLINES |
										  LVS_EX_HEADERDRAGDROP |
										  LVS_EX_LABELTIP);

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
	lvc.cx = 150;
	ListView_InsertColumn(hList, 3, &lvc);

	return hList;
}

std::vector<KeyboardDevice> GetKeyboardDevices()
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
					dev.userLabel = getSettings().hashToDevice.find(dev.hash) != getSettings().hashToDevice.end()
										? getSettings().hashToDevice.at(dev.hash)
										: L"Unknown";

					devices.push_back(dev);
				}
			}
		}
	}

	delete[] pRawInputDeviceList;
	return devices;
}

void UpdateDeviceTable(HWND hList)
{
	ListView_DeleteAllItems(hList);

	auto devices = GetKeyboardDevices();

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
