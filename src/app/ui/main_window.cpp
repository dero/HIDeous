#include "main_window.h"
#include "taskbar_icon.h"
#include "components/device_list.h"
#include "components/profile_selector.h"
#include "../processing/raw_input_handler.h"
#include "../processing/macro_executor.h"
#include "../resource.h"
#include "../../common/utils/logging.h"
#include "../../common/input/key_mapping.h"
#include "../../common/config/settings_manager.h"
#include "../../hook_dll/hideous_hook.h"

// WM_HIDEOUS_KEYBOARD_EVENT is in hook header
#ifndef WM_HIDEOUS_KEYBOARD_EVENT
#define WM_HIDEOUS_KEYBOARD_EVENT (WM_APP + 1)
#endif

#include <commctrl.h>
#include <shellapi.h>
#include <sstream>
#include <iomanip>

static HWND g_mainWindow = NULL;

HWND GetMainWindow()
{
	return g_mainWindow;
}

void CreateHelpButton(HWND hwnd)
{
	RECT clientRect;
	GetClientRect(hwnd, &clientRect);

	int padding = 3;
	int buttonHeight = 24;
	int buttonWidth = 150;

	int buttonX = clientRect.right - buttonWidth - padding;
	int buttonY = clientRect.bottom - buttonHeight - padding;

	CreateWindowEx(0, L"BUTTON", L"Help",
				   WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_FLAT,
				   buttonX, buttonY, buttonWidth, buttonHeight,
				   hwnd, (HMENU)IDC_HELP_BUTTON, GetModuleHandle(NULL), NULL);
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
        RawKeyData data;
        if (GetRawKeyData((HRAWINPUT)lParam, data)) {
            // Find the device in the list
            HWND hList = GetDlgItem(hwnd, IDC_MAIN_LIST);
            int itemIndex = FindDeviceListItem(hList, data.hDevice);

            if (itemIndex >= 0)
            {
                // Get device hash
                std::wstring deviceHash = GetListViewCellText(hList, itemIndex, 1);

                if (!(data.flags & RI_KEY_BREAK))
                {
                    // Log the key press, only on key down
                    std::wostringstream ss;
                    ss << "1️⃣ WM_INPUT - Device: 0x" << std::hex << (UINT64)data.hDevice
                        << " Key: 0x" << data.vkCode
                        << " Flags: 0x" << data.flags << std::dec;
                    DebugLog(ss.str());

                    // Update last keypress info
                    g_lastKeypress.timestamp = GetTickCount();
                    g_lastKeypress.deviceHash = deviceHash;
                    g_lastKeypress.vkCode = data.vkCode;

                    // Key pressed - show VK code
                    std::wstring keyName = virtualKeyCodeToString(data.vkCode);

                    // L"0x%02X (%hs)"
                    WCHAR keyText[256];
                    swprintf_s(keyText, L"0x%02X (%s)", data.vkCode, keyName.c_str());

                    ListView_SetItemText(hList, itemIndex, 3, keyText);
                }
                else
                {
                    // Key released - clear the text
                    ListView_SetItemText(hList, itemIndex, 3, L"");
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
		return DecideOnKey((USHORT)wParam);

		break;
	}

	case WM_SETTINGS_CHANGED:
	{
		DebugLog(L"Settings changed on disk, refreshing...");

		// Refresh hook DLL
		RefreshInternalState();

		// Refresh UI
		HWND hList = GetDlgItem(hwnd, IDC_MAIN_LIST);
		UpdateDeviceTable(hList);

		// Refresh profile selector
		HWND hCombo = GetDlgItem(hwnd, IDC_PROFILE_SELECTOR);
		SendMessage(hCombo, CB_RESETCONTENT, 0, 0);
		SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"Profile: Default");
		auto profiles = SettingsManager::getInstance().getAvailableProfiles();
		for (const auto &profile : profiles)
		{
			const std::wstring profileName = L"Profile: " + profile;
			SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)profileName.c_str());
		}

		// Re-select current profile
		std::wstring currentProfile = SettingsManager::getInstance().currentProfile();
		std::wstring searchText = L"Profile: " + currentProfile;
		if (currentProfile.empty())
			searchText = L"Profile: Default";

		int itemCount = SendMessage(hCombo, CB_GETCOUNT, 0, 0);
		for (int i = 0; i < itemCount; i++)
		{
			WCHAR buffer[256];
			SendMessage(hCombo, CB_GETLBTEXT, i, (LPARAM)buffer);
			if (searchText == buffer)
			{
				SendMessage(hCombo, CB_SETCURSEL, i, 0);
				break;
			}
		}

		UpdateProfileSettingsLink(hwnd);
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
		g_mainWindow = hwnd;
		CreateTrayIcon(hwnd);
		CreateProfileSelector(hwnd);
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
			AppendMenu(hMenu, MF_STRING, IDM_RESTORE, L"Show window");
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

		case IDC_HELP_BUTTON:
		{
			ShellExecute(NULL, L"open", L"https://github.com/dero/HIDeous", NULL, NULL, SW_SHOWNORMAL);
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

		case IDC_PROFILE_SELECTOR:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
                OnProfileSelectorSelChange((HWND)lParam);
			}
			break;

		case IDC_SETTINGS_LINK:
			ShellExecute(NULL, L"open", L"settings.ini", NULL, NULL, SW_SHOWNORMAL);
			break;

		case IDC_PROFILE_SETTINGS_LINK:
		{
            OnProfileSettingsLinkClick(GetDlgItem(hwnd, IDC_PROFILE_SELECTOR));
			break;
		}
		}
		break;
	}

	case WM_CTLCOLORSTATIC:
	{
		if ((HWND)lParam == GetDlgItem(hwnd, IDC_SETTINGS_LINK) ||
			(HWND)lParam == GetDlgItem(hwnd, IDC_PROFILE_SETTINGS_LINK))
		{
			HDC hdcStatic = (HDC)wParam;
			SetTextColor(hdcStatic, RGB(0, 0, 255));
			SetBkMode(hdcStatic, TRANSPARENT);
			return (LRESULT)GetStockObject(NULL_BRUSH);
		}
		break;
	}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
