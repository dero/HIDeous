#include "device_list.h"
#include <commctrl.h>
#include <iostream>
#include <hidusage.h>
#include "../../resource.h"
#include "../../../common/config/settings_manager.h"
#include "../../../common/utils/crypto.h"

// Ensure we link with Comctl32.lib if not already linked (it is in CMake)

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

HWND CreateDeviceTable(HWND parent)
{
	HWND hList = CreateWindowEx(0, WC_LISTVIEW, NULL,
								WS_CHILD | WS_VISIBLE | LVS_REPORT | LBS_HASSTRINGS | LVS_SHOWSELALWAYS,
								10, 65, 700, 260, parent, // Moved down from 45 to 65
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
		return devices;
	}

	pRawInputDeviceList = new RAWINPUTDEVICELIST[nDevices];
	if (GetRawInputDeviceList(pRawInputDeviceList, &nDevices, sizeof(RAWINPUTDEVICELIST)) == -1)
	{
		delete[] pRawInputDeviceList;
		return devices;
	}

	const Settings &settings = SettingsManager::getInstance().getSettings();

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
					dev.userLabel = settings.hashToDevice.find(dev.hash) != settings.hashToDevice.end()
										? settings.hashToDevice.at(dev.hash)
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
