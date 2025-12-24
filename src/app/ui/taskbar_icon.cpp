#include "taskbar_icon.h"
#include <shellapi.h>
#include "../resource.h"
#include <cwchar>

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

void RemoveTrayIcon()
{
	Shell_NotifyIcon(NIM_DELETE, &g_trayIcon);
}
