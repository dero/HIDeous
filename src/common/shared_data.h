#pragma once

#include <windows.h>

#ifndef SHARED_DATA_H
#define SHARED_DATA_H

struct SharedData
{
	HWND mainWindow;
	BOOL isDebugMode;
	WCHAR logPath[MAX_PATH];
};

extern SharedData *g_shared;

bool InitializeSharedMemory();
void CleanupSharedMemory();

#endif // SHARED_DATA_H
