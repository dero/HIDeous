#pragma once
#include <windows.h>

#define WM_TRAYICON (WM_USER + 1)

void CreateTrayIcon(HWND hwnd);
void RemoveTrayIcon();
