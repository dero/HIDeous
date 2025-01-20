#include "settings.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cctype>
#include <algorithm>
#include <windows.h>
#include "../common/logging.h"

/**
 * Map of key names to virtual key codes.
 *
 * Loosely based on the list of virtual key codes at:
 *
 * https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
 */
std::unordered_map<std::string, WORD> keyMap = {
	{"SHIFT", VK_SHIFT},
	{"ALT", VK_MENU},
	{"CTRL", VK_CONTROL},
	{"WIN", VK_LWIN},
	{"META", VK_LWIN},
	{"ENTER", VK_RETURN},
	{"RETURN", VK_RETURN},
	{"SPACE", VK_SPACE},
	{"BACKSPACE", VK_BACK},
	{"TAB", VK_TAB},
	{"ESCAPE", VK_ESCAPE},
	{"ESC", VK_ESCAPE},
	{"UP", VK_UP},
	{"DOWN", VK_DOWN},
	{"LEFT", VK_LEFT},
	{"RIGHT", VK_RIGHT},
	{"PAGEUP", VK_PRIOR},
	{"PGUP", VK_PRIOR},
	{"PAGEDOWN", VK_NEXT},
	{"PGDN", VK_NEXT},
	{"HOME", VK_HOME},
	{"END", VK_END},
	{"INS", VK_INSERT},
	{"INSERT", VK_INSERT},
	{"DEL", VK_DELETE},
	{"DELETE", VK_DELETE},
	{"F1", VK_F1},
	{"F2", VK_F2},
	{"F3", VK_F3},
	{"F4", VK_F4},
	{"F5", VK_F5},
	{"F6", VK_F6},
	{"F7", VK_F7},
	{"F8", VK_F8},
	{"F9", VK_F9},
	{"F10", VK_F10},
	{"F11", VK_F11},
	{"F12", VK_F12},
	{"F13", VK_F13},
	{"F14", VK_F14},
	{"F15", VK_F15},
	{"F16", VK_F16},
	{"F17", VK_F17},
	{"F18", VK_F18},
	{"F19", VK_F19},
	{"F20", VK_F20},
	{"F21", VK_F21},
	{"F22", VK_F22},
	{"F23", VK_F23},
	{"F24", VK_F24},
	{"CAPS", VK_CAPITAL},
	{"CAPSLOCK", VK_CAPITAL},
	{"NUM", VK_NUMLOCK},
	{"NUMLOCK", VK_NUMLOCK},
	{"SCROLL", VK_SCROLL},
	{"SCROLLLOCK", VK_SCROLL},
	{"PRNTSCRN", VK_SNAPSHOT},
	{"PRINTSCREEN", VK_SNAPSHOT},
	{"PAUSE", VK_PAUSE},
	{"BREAK", VK_CANCEL},
	{"APPS", VK_APPS},
	{"MULTIPLY", VK_MULTIPLY},
	{"ADD", VK_ADD},
	{"SEPARATOR", VK_SEPARATOR},
	{"SUBTRACT", VK_SUBTRACT},
	{"DECIMAL", VK_DECIMAL},
	{"DIVIDE", VK_DIVIDE},
	{"NUMPAD0", VK_NUMPAD0},
	{"NUMPAD1", VK_NUMPAD1},
	{"NUMPAD2", VK_NUMPAD2},
	{"NUMPAD3", VK_NUMPAD3},
	{"NUMPAD4", VK_NUMPAD4},
	{"NUMPAD5", VK_NUMPAD5},
	{"NUMPAD6", VK_NUMPAD6},
	{"NUMPAD7", VK_NUMPAD7},
	{"NUMPAD8", VK_NUMPAD8},
	{"NUMPAD9", VK_NUMPAD9},
	{"LSHIFT", VK_LSHIFT},
	{"RSHIFT", VK_RSHIFT},
	{"LCTRL", VK_LCONTROL},
	{"RCTRL", VK_RCONTROL},
	{"LALT", VK_LMENU},
	{"RALT", VK_RMENU},
	{"LWIN", VK_LWIN},
	{"RWIN", VK_RWIN},
	{"MENU", VK_APPS}, // Context menu key
	{"SLEEP", VK_SLEEP},
	{"BROWSER_BACK", VK_BROWSER_BACK},
	{"BROWSER_FORWARD", VK_BROWSER_FORWARD},
	{"BROWSER_REFRESH", VK_BROWSER_REFRESH},
	{"BROWSER_STOP", VK_BROWSER_STOP},
	{"BROWSER_SEARCH", VK_BROWSER_SEARCH},
	{"BROWSER_FAVORITES", VK_BROWSER_FAVORITES},
	{"BROWSER_HOME", VK_BROWSER_HOME},
	{"VOLUME_MUTE", VK_VOLUME_MUTE},
	{"VOLUME_DOWN", VK_VOLUME_DOWN},
	{"VOLUME_UP", VK_VOLUME_UP},
	{"MEDIA_NEXT", VK_MEDIA_NEXT_TRACK},
	{"MEDIA_PREV", VK_MEDIA_PREV_TRACK},
	{"MEDIA_STOP", VK_MEDIA_STOP},
	{"MEDIA_PLAY_PAUSE", VK_MEDIA_PLAY_PAUSE},
	{";", VK_OEM_1}, // For US standard keyboards
	{"=", VK_OEM_PLUS},
	{",", VK_OEM_COMMA},
	{"-", VK_OEM_MINUS},
	{".", VK_OEM_PERIOD},
	{"/", VK_OEM_2},  // For US standard keyboards
	{"`", VK_OEM_3},  // For US standard keyboards
	{"[", VK_OEM_4},  // For US standard keyboards
	{"\\", VK_OEM_5}, // For US standard keyboards
	{"]", VK_OEM_6},  // For US standard keyboards
	{"'", VK_OEM_7},  // For US standard keyboards
};

Settings::Settings()
{
	wchar_t exePath[MAX_PATH];
	GetModuleFileNameW(NULL, exePath, MAX_PATH);
	std::wstring::size_type pos = std::wstring(exePath).find_last_of(L"\\/");
	filePath = std::wstring(exePath).substr(0, pos) + L"\\settings.ini";
}

/**
 * Load settings from the settings file.
 *
 * The settings file is a simple INI-style file with sections and key-value pairs.
 * The Devices section contains key-value pairs of device names and device paths.
 * The other sections contain key-value pairs of key names and key mappings.
 */
bool Settings::load()
{
	std::ifstream file(filePath);
	if (!file.is_open())
	{
		DebugLog("Failed to open settings file");
		return false;
	}

	std::string line;
	std::string currentSection;
	while (std::getline(file, line))
	{
		line = trim(line);
		if (line.empty() || line[0] == ';')
		{
			continue;
		}

		if (line[0] == '[' && line.back() == ']')
		{
			currentSection = line.substr(1, line.size() - 2);
		}
		else
		{
			auto delimiterPos = line.find('=');
			if (delimiterPos == std::string::npos)
			{
				continue;
			}

			std::string key = trim(line.substr(0, delimiterPos));
			std::string value = trim(line.substr(delimiterPos + 1));

			if (currentSection == "Devices")
			{
				devices[key] = value;
			}
			else
			{
				mappings[currentSection][key] = value;
			}
		}
	}

	file.close();
	return true;
}

/**
 * Trim whitespace from the beginning and end of a string.
 *
 * @param str The string to trim.
 * @return The trimmed string.
 */
std::string Settings::trim(const std::string &str)
{
	size_t first = str.find_first_not_of(' ');
	if (first == std::string::npos)
		return "";

	size_t last = str.find_last_not_of(' ');
	return str.substr(first, last - first + 1);
}

/**
 * Convert a virtual key code to its string representation.
 * Returns the first matching key name from the keyMap.
 *
 * @param vk The virtual key code to convert.
 * @return The string representation of the key, or hexadecimal value in a string.
 */
std::string Settings::virtualKeyCodeToString(WORD vk)
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
		return std::string(1, static_cast<char>(vk));
	}

	// If no match found, return the hex value as a string
	std::ostringstream ss;
	ss << "0x" << std::hex << vk;
	return ss.str();
}

/**
 * Convert a string key name to a virtual key code.
 *
 * The key name can either be a single character,
 * a key name (e.g. "A", "F1", "CTRL", "SPACE"),
 * or a hex value (e.g. "0x41").
 *
 * @see https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
 *
 * @param str The string key name to convert.
 * @return The virtual key code.
 */
WORD Settings::stringToVirtualKeyCode(const std::string &str)
{
	// Create uppercase version of input string
	std::string upper = str;
	std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

	// First check the map using uppercase string
	auto it = keyMap.find(upper);
	if (it != keyMap.end())
	{
		return it->second;
	}

	// Check for hex format
	if (upper.size() > 2 && upper[0] == '0' && (upper[1] == 'x' || upper[1] == 'X'))
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
		SHORT vk = VkKeyScanA(str[0]);
		if (vk != -1)
		{
			return LOBYTE(vk);
		}
	}

	return 0;
}

/**
 * Convert a string of keys separated by '+' into a vector of INPUT structs.
 *
 * Example: "CTRL+ALT+DEL" -> {CTRL down, ALT down, DEL down, DEL up, ALT up, CTRL up}
 * Example: "CTRL+F4" -> {CTRL down, F4 down, F4 up, CTRL up}
 *
 * @param keyString The string of keys to convert.
 * @return A vector of INPUT structs.
 */
std::vector<INPUT> Settings::convertStringToInput(const std::string &keyString)
{
	std::vector<INPUT> inputs;
	std::istringstream stream(keyString);
	std::string token;
	std::vector<std::string> keys;

	while (std::getline(stream, token, '+'))
	{
		keys.push_back(trim(token));
	}

	// Press keys in sequence
	for (const auto &key : keys)
	{
		INPUT input = {0};
		input.type = INPUT_KEYBOARD;
		input.ki.wVk = stringToVirtualKeyCode(key);
		inputs.push_back(input); // Key down
	}

	// Release keys in reverse sequence
	for (auto it = keys.rbegin(); it != keys.rend(); ++it)
	{
		INPUT input = {0};
		input.type = INPUT_KEYBOARD;
		input.ki.wVk = stringToVirtualKeyCode(*it);
		input.ki.dwFlags = KEYEVENTF_KEYUP;
		inputs.push_back(input); // Key up
	}

	return inputs;
}

const std::unordered_map<std::string, std::string> &Settings::getDevices() const
{
	return devices;
}

const std::unordered_map<std::string, std::unordered_map<std::string, std::string>> &Settings::getMappings() const
{
	return mappings;
}
