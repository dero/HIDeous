#ifndef HIDEOUS_APP_H
#define HIDEOUS_APP_H

#include <windows.h>
#include <string>

// Function prototypes
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

#endif // HIDEOUS_APP_H
