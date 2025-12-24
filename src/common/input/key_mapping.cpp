#include "key_mapping.h"
#include "../utils/string_utils.h"
#include <sstream>
#include <algorithm>
#include <iostream>

std::unordered_map<std::wstring, WORD> keyMap = {
	{L"SHIFT", VK_SHIFT},
	{L"ALT", VK_MENU},
	{L"CTRL", VK_CONTROL},
	{L"WIN", VK_LWIN},
	{L"META", VK_LWIN},
	{L"ENTER", VK_RETURN},
	{L"RETURN", VK_RETURN},
	{L"SPACE", VK_SPACE},
	{L"BACKSPACE", VK_BACK},
	{L"TAB", VK_TAB},
	{L"ESCAPE", VK_ESCAPE},
	{L"ESC", VK_ESCAPE},
	{L"UP", VK_UP},
	{L"DOWN", VK_DOWN},
	{L"LEFT", VK_LEFT},
	{L"RIGHT", VK_RIGHT},
	{L"PAGEUP", VK_PRIOR},
	{L"PGUP", VK_PRIOR},
	{L"PAGEDOWN", VK_NEXT},
	{L"PGDN", VK_NEXT},
	{L"HOME", VK_HOME},
	{L"END", VK_END},
	{L"INS", VK_INSERT},
	{L"INSERT", VK_INSERT},
	{L"DEL", VK_DELETE},
	{L"DELETE", VK_DELETE},
	{L"F1", VK_F1},
	{L"F2", VK_F2},
	{L"F3", VK_F3},
	{L"F4", VK_F4},
	{L"F5", VK_F5},
	{L"F6", VK_F6},
	{L"F7", VK_F7},
	{L"F8", VK_F8},
	{L"F9", VK_F9},
	{L"F10", VK_F10},
	{L"F11", VK_F11},
	{L"F12", VK_F12},
	{L"F13", VK_F13},
	{L"F14", VK_F14},
	{L"F15", VK_F15},
	{L"F16", VK_F16},
	{L"F17", VK_F17},
	{L"F18", VK_F18},
	{L"F19", VK_F19},
	{L"F20", VK_F20},
	{L"F21", VK_F21},
	{L"F22", VK_F22},
	{L"F23", VK_F23},
	{L"F24", VK_F24},
	{L"CAPS", VK_CAPITAL},
	{L"CAPSLOCK", VK_CAPITAL},
	{L"NUM", VK_NUMLOCK},
	{L"NUMLOCK", VK_NUMLOCK},
	{L"SCROLL", VK_SCROLL},
	{L"SCROLLLOCK", VK_SCROLL},
	{L"PRNTSCRN", VK_SNAPSHOT},
	{L"PRINTSCREEN", VK_SNAPSHOT},
	{L"PAUSE", VK_PAUSE},
	{L"BREAK", VK_CANCEL},
	{L"APPS", VK_APPS},
	{L"MULTIPLY", VK_MULTIPLY},
	{L"ADD", VK_ADD},
	{L"SEPARATOR", VK_SEPARATOR},
	{L"SUBTRACT", VK_SUBTRACT},
	{L"DECIMAL", VK_DECIMAL},
	{L"DIVIDE", VK_DIVIDE},
	{L"NUMPAD0", VK_NUMPAD0},
	{L"NUMPAD1", VK_NUMPAD1},
	{L"NUMPAD2", VK_NUMPAD2},
	{L"NUMPAD3", VK_NUMPAD3},
	{L"NUMPAD4", VK_NUMPAD4},
	{L"NUMPAD5", VK_NUMPAD5},
	{L"NUMPAD6", VK_NUMPAD6},
	{L"NUMPAD7", VK_NUMPAD7},
	{L"NUMPAD8", VK_NUMPAD8},
	{L"NUMPAD9", VK_NUMPAD9},
	{L"LSHIFT", VK_LSHIFT},
	{L"RSHIFT", VK_RSHIFT},
	{L"LCTRL", VK_LCONTROL},
	{L"RCTRL", VK_RCONTROL},
	{L"LALT", VK_LMENU},
	{L"RALT", VK_RMENU},
	{L"LWIN", VK_LWIN},
	{L"RWIN", VK_RWIN},
	{L"MENU", VK_APPS}, // Context menu key
	{L"SLEEP", VK_SLEEP},
	{L"BROWSER_BACK", VK_BROWSER_BACK},
	{L"BROWSER_FORWARD", VK_BROWSER_FORWARD},
	{L"BROWSER_REFRESH", VK_BROWSER_REFRESH},
	{L"BROWSER_STOP", VK_BROWSER_STOP},
	{L"BROWSER_SEARCH", VK_BROWSER_SEARCH},
	{L"BROWSER_FAVORITES", VK_BROWSER_FAVORITES},
	{L"BROWSER_HOME", VK_BROWSER_HOME},
	{L"VOLUME_MUTE", VK_VOLUME_MUTE},
	{L"VOLUME_DOWN", VK_VOLUME_DOWN},
	{L"VOLUME_UP", VK_VOLUME_UP},
	{L"MEDIA_NEXT", VK_MEDIA_NEXT_TRACK},
	{L"MEDIA_PREV", VK_MEDIA_PREV_TRACK},
	{L"MEDIA_STOP", VK_MEDIA_STOP},
	{L"MEDIA_PLAY_PAUSE", VK_MEDIA_PLAY_PAUSE},
	{L";", VK_OEM_1}, // For US standard keyboards
	{L"EQUALS", VK_OEM_PLUS}, // For US standard keyboards
	{L",", VK_OEM_COMMA},
	{L"-", VK_OEM_MINUS},
	{L".", VK_OEM_PERIOD},
	{L"/", VK_OEM_2},  // For US standard keyboards
	{L"`", VK_OEM_3},  // For US standard keyboards
	{L"[", VK_OEM_4},  // For US standard keyboards
	{L"\\", VK_OEM_5}, // For US standard keyboards
	{L"]", VK_OEM_6},  // For US standard keyboards
	{L"'", VK_OEM_7},  // For US standard keyboards
};

std::wstring virtualKeyCodeToString(WORD vk)
{
	for (const auto &pair : keyMap)
	{
		if (pair.second == vk)
		{
			return pair.first;
		}
	}

	// Letters and numbers
	if ((vk >= 0x41 && vk <= 0x5A) || (vk >= 0x30 && vk <= 0x39))
	{
		return std::wstring(1, static_cast<wchar_t>(vk));
	}

	// If no match found, return the hex value as a string
	std::wostringstream ss;
	ss << L"0x" << std::hex << vk;
	return ss.str();
}

WORD stringToVirtualKeyCode(const std::wstring &str)
{
	// Create uppercase version of input string
	std::wstring upper = str;
	std::transform(upper.begin(), upper.end(), upper.begin(), ::towupper);

	// First check the map using uppercase string
	auto it = keyMap.find(upper);
	if (it != keyMap.end())
	{
		return it->second;
	}

	// Check for hex format
	if (upper.size() > 2 && upper[0] == L'0' && (upper[1] == L'x' || upper[1] == L'X'))
	{
		try
		{
			return static_cast<WORD>(std::stoul(upper, nullptr, 16));
		}
		catch (const std::exception &)
		{
			return 0;
		}
	}

	// Check for single character
	if (str.size() == 1)
	{ // Use original string here, not upper
		SHORT vk = VkKeyScanW(str[0]);
		if (vk != -1)
		{
			return LOBYTE(vk);
		}
	}

	return 0;
}

bool isScanCodeString(const std::wstring &str, USHORT* outScanCode)
{
	if (str.length() > 2)
	{
		std::wstring prefix = str.substr(0, 2);
		std::transform(prefix.begin(), prefix.end(), prefix.begin(), ::towupper);
		
		if (prefix == L"SC")
		{
			try
			{
				unsigned long val = std::stoul(str.substr(2));
				if (outScanCode) *outScanCode = static_cast<USHORT>(val);
				return true;
			}
			catch (...) {}
		}
	}
	return false;
}

std::vector<INPUT> convertStringToInput(const std::wstring &keyString)
{
	std::vector<INPUT> inputs;
	std::wistringstream stream(keyString);
	std::wstring token;
	std::vector<std::wstring> keys;

	while (std::getline(stream, token, L'+'))
	{
		keys.push_back(trim(token));
	}

	// Store properties of pressed keys to release them later
	std::vector<INPUT> pressedKeys;

	// Press keys in sequence
	for (const auto &key : keys)
	{
		INPUT input = {0};
		input.type = INPUT_KEYBOARD;
		
		// Check for SC prefix (e.g. SC28)
		bool isScanCode = false;
		if (key.length() > 2)
		{
			std::wstring prefix = key.substr(0, 2);
			std::transform(prefix.begin(), prefix.end(), prefix.begin(), ::towupper);
			
			if (prefix == L"SC")
			{
				try
				{
					input.ki.wScan = static_cast<WORD>(std::stoul(key.substr(2)));
					input.ki.dwFlags = KEYEVENTF_SCANCODE;
					isScanCode = true;
				}
				catch (...) {}
			}
		}

		if (!isScanCode)
		{
			input.ki.wVk = stringToVirtualKeyCode(key);
		}
		
		inputs.push_back(input); // Key down
		pressedKeys.push_back(input);
	}

	// Release keys in reverse sequence
	for (auto it = pressedKeys.rbegin(); it != pressedKeys.rend(); ++it)
	{
		INPUT input = *it;
		input.ki.dwFlags |= KEYEVENTF_KEYUP;
		inputs.push_back(input); // Key up
	}

	return inputs;
}
