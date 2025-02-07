#ifndef HIDEOUS_GUI_H
#define HIDEOUS_GUI_H

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include "../common/settings.h"

#define WM_TRAYICON (WM_USER + 1)

struct LastKeypress
{
	DWORD timestamp; // GetTickCount() value
	std::wstring deviceHash;
	USHORT vkCode;
};

struct KeyboardDevice
{
	std::wstring fullName;
	std::wstring hash;
	std::wstring userLabel;
};

extern LastKeypress g_lastKeypress;

// GUI function declarations
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
HWND CreateDeviceTable(HWND parent);
void UpdateDeviceTable(HWND hList);
std::wstring GetListViewCellText(HWND hList, int row, int col);
int FindDeviceListItem(HWND hList, HANDLE hDevice);
void StartupCallback(BOOL isChecked);
void CreateStartupCheckbox(HWND hwnd);
void CreateHelpButton(HWND hwnd);
HWND CreateProfileSelector(HWND parent);
void UpdateProfileSettingsLink(HWND hwnd);
bool SwitchToProfile(const std::wstring &profileName);

#endif // HIDEOUS_GUI_H
