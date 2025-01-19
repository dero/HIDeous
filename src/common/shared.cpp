#include "shared.h"

// Define the global variable
SharedData *g_shared = nullptr;

HANDLE g_sharedMem = nullptr;

bool InitializeSharedMemory()
{
	g_sharedMem = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		nullptr,
		PAGE_READWRITE,
		0,
		sizeof(SharedData),
		TEXT("Local\\HideousHook_SharedMem"));

	if (!g_sharedMem)
	{
		return false;
	}

	g_shared = (SharedData *)MapViewOfFile(
		g_sharedMem,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		sizeof(SharedData));

	if (!g_shared)
	{
		CloseHandle(g_sharedMem);
		g_sharedMem = nullptr;
		return false;
	}

	return true;
}

void CleanupSharedMemory()
{
	if (g_shared)
	{
		UnmapViewOfFile(g_shared);
		g_shared = nullptr;
	}
	if (g_sharedMem)
	{
		CloseHandle(g_sharedMem);
		g_sharedMem = nullptr;
	}
}
