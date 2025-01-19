#pragma once

#include <windows.h>

#ifndef shared_H
#define shared_H

struct SharedData
{
	HWND mainWindow;
	BOOL isDebugMode;
	WCHAR logPath[MAX_PATH];
};

extern SharedData *g_shared;

bool InitializeSharedMemory();
void CleanupSharedMemory();

#endif // shared_H
