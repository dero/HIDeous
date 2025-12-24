#pragma once
#include <string>
#include <unordered_map>
#include <windows.h>
#include <vector>

extern std::unordered_map<std::wstring, WORD> keyMap;

std::wstring virtualKeyCodeToString(WORD vk);
WORD stringToVirtualKeyCode(const std::wstring &str);
bool isScanCodeString(const std::wstring &str, USHORT* outScanCode);
std::vector<INPUT> convertStringToInput(const std::wstring &keyString);
