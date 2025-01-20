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
#include "resource.h"
#include "settings.h"
#include "../common/shared.h"
#include "../common/crypto.h"
#include "../common/logging.h"
#include "../dll/hideous_hook.h"

// Link with Shlwapi.lib and Comctl32.lib
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Comctl32.lib")

struct LastKeypress
{
	DWORD timestamp; // GetTickCount() value
	std::string deviceHash;
	USHORT vkCode;
	bool valid; // Indicates if we have any keypress recorded
};

static LastKeypress g_lastKeypress = {0, "", 0, false};

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
					HWND hList = GetDlgItem(hwnd, 0);
					int itemIndex = FindDeviceListItem(hList, raw->header.hDevice);

					if (itemIndex >= 0)
					{
						// Get device hash
						std::wstring deviceHashW = GetListViewCellText(hList, itemIndex, 1);
						std::string deviceHash = WideToNarrow(deviceHashW.c_str());

						if (!(raw->data.keyboard.Flags & RI_KEY_BREAK))
						{
							// Update last keypress info
							g_lastKeypress.timestamp = GetTickCount();
							g_lastKeypress.deviceHash = deviceHash;
							g_lastKeypress.vkCode = raw->data.keyboard.VKey;
							g_lastKeypress.valid = true;

							// Key pressed - show VK code
							std::string keyName = Settings::virtualKeyCodeToString(raw->data.keyboard.VKey);
							WCHAR keyText[32];
							swprintf_s(keyText, L"0x%02X (%hs)", raw->data.keyboard.VKey, keyName.c_str());

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
		std::ostringstream ss;
		ss << " WM_HIDEOUS_KEYBOARD_EVENT - Key: 0x" << std::hex << wParam
		   << " lParam: 0x" << lParam << std::dec;
		DebugLog(ss.str());

		break;
	}

	case WM_CONTEXTMENU:
	{
		HWND hList = GetDlgItem(hwnd, 0);
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

	case WM_COMMAND:
	{
		HWND hList = GetDlgItem(hwnd, 0);
		switch (LOWORD(wParam))
		{
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
								10, 10, 700, 300, parent, NULL, GetModuleHandle(NULL), NULL);

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
