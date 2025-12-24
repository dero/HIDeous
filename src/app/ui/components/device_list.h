#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include "../gui_types.h"

HWND CreateDeviceTable(HWND parent);
void UpdateDeviceTable(HWND hList);
std::wstring GetListViewCellText(HWND hList, int row, int col);
int FindDeviceListItem(HWND hList, HANDLE hDevice);
std::vector<KeyboardDevice> GetKeyboardDevices();
