#ifndef HIDEOUS_GUI_H
#define HIDEOUS_GUI_H

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include "../common/settings.h"

#define IDM_COPY_CELL 1001
#define IDM_RESTORE 1002
#define IDM_EXIT 1003
#define WM_TRAYICON (WM_USER + 1)

struct LastKeypress
{
	DWORD timestamp; // GetTickCount() value
	std::string deviceHash;
	USHORT vkCode;
};

struct KeyboardDevice
{
	std::wstring fullName;
	std::string hash;
	std::string userLabel; // From settings.ini
};

extern LastKeypress g_lastKeypress;

// GUI function declarations
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
HWND CreateDeviceTable(HWND parent);
void UpdateDeviceTable(HWND hList);
std::wstring GetListViewCellText(HWND hList, int row, int col);
int FindDeviceListItem(HWND hList, HANDLE hDevice);

#endif // HIDEOUS_GUI_H
