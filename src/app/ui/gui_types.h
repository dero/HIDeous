#pragma once
#include <windows.h>
#include <string>

#define WM_SETTINGS_CHANGED (WM_APP + 2)

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

// Global last keypress - defined in entry_point.cpp
extern LastKeypress g_lastKeypress;
