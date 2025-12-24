#pragma once
#include <windows.h>

HWND GetMainWindow();
void CreateStartupCheckbox(HWND hwnd);
void CreateHelpButton(HWND hwnd);
void StartupCallback(BOOL isChecked);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
