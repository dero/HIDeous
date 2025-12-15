#include "settings.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cctype>
#include <algorithm>
#include <windows.h>
#include <locale>
#include <codecvt>
#include <iterator>
#include "../common/logging.h"

/**
 * Map of key names to virtual key codes.
 *
 * Loosely based on the list of virtual key codes at:
 *
 * https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
 */
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
	{L"=", VK_OEM_PLUS},
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

SettingsManager &SettingsManager::getInstance()
{
	static SettingsManager instance;
	return instance;
}

SettingsManager::SettingsManager()
{
	m_settings = loadSettings();
}

bool persistSettingsPath(const std::wstring &path)
{
	HKEY hKey;
	LONG result = RegCreateKeyExW(
		HKEY_CURRENT_USER,
		REG_KEY_PATH,
		0,
		NULL,
		0,
		KEY_WRITE,
		NULL,
		&hKey,
		NULL);

	if (result != ERROR_SUCCESS)
	{
		return false;
	}

	result = RegSetValueExW(
		hKey,
		REG_VALUE_NAME,
		0,
		REG_SZ,
		(BYTE *)path.c_str(),
		(DWORD)((path.length() + 1) * sizeof(wchar_t)));

	RegCloseKey(hKey);
	return (result == ERROR_SUCCESS);
}

std::wstring SettingsManager::getAppPath()
{
	HKEY hKey;
	LONG result = RegOpenKeyExW(
		HKEY_CURRENT_USER,
		REG_KEY_PATH,
		0,
		KEY_READ,
		&hKey);

	if (result != ERROR_SUCCESS)
	{
		return L"";
	}

	wchar_t path[MAX_PATH];

	DWORD size = MAX_PATH;
	result = RegQueryValueExW(
		hKey,
		REG_VALUE_NAME,
		NULL,
		NULL,
		(BYTE *)path,
		&size);

	RegCloseKey(hKey);

	if (result != ERROR_SUCCESS)
	{
		return L"";
	}

	return path;
}

/**
 * Helper to read a UTF-8 file and convert it to std::wstring using Windows API.
 * This avoids the issues with std::codecvt on Windows where it might truncate 4-byte UTF-8 sequences.
 */
std::wstring ReadUtf8File(const std::wstring& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return L"";
    }

    std::string str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    if (str.empty()) {
        return L"";
    }

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    
    // Remove BOM if present
    if (!wstrTo.empty() && wstrTo[0] == 0xFEFF) {
        wstrTo.erase(0, 1);
    }

    return wstrTo;
}

Settings SettingsManager::loadSettings()
{
    GlobalSettings globalSettings = {DEFAULT_DEBUG_MODE, DEFAULT_DEBUG_FILE, DEFAULT_KEY_WAIT_TIME};
    Settings settings = {globalSettings, {}, {}, {}, L""};

    std::wstring settingsPath = getAppPath() + L"\\settings.ini";
    
    // Use the UTF-8 safe reader
    std::wstring fileContent = ReadUtf8File(settingsPath);
    if (fileContent.empty())
    {
        DebugLog(L"Failed to open settings file or file is empty");
        // Try standard open just to be sure if it exists but empty? 
        // Or just return default.
        // If file doesn't exist ReadUtf8File returns empty, which is fine.
        return settings;
    }

    std::wstringstream file(fileContent);
    std::wstring line;
    std::wstring currentSection;

    while (std::getline(file, line))
    {
        line = trim(line);
        if (line.empty() || line[0] == L';')
        {
            continue;
        }

        if (line[0] == L'[' && line.back() == L']')
        {
            currentSection = line.substr(1, line.size() - 2);
        }
        else
        {
            auto delimiterPos = line.find(L'=');
            if (delimiterPos == std::wstring::npos)
            {
                continue;
            }

            std::wstring key = trim(line.substr(0, delimiterPos));
            std::wstring value = trim(line.substr(delimiterPos + 1));

            if (currentSection == L"Devices")
            {
                settings.deviceToHash[key] = value;
                settings.hashToDevice[value] = key;
            }
            else if (currentSection == L"Global")
            {
                if (key == L"KeyWaitTime")
                {
                    settings.global.KeyWaitTime = std::stoi(value);
                }
                else if (key == L"Debug")
                {
                    settings.global.Debug = (value == L"true" || value == L"1");
                }
                else if (key == L"DebugFile")
                {
                    settings.global.DebugFile = value;
                }
            }
            else
            {
                // Convert key to uppercase in order to make the mapping case-insensitive
                // and match the key names in the keyMap
                std::wstring uCaseKey = key;
                std::transform(uCaseKey.begin(), uCaseKey.end(), uCaseKey.begin(), ::towupper);
                settings.mappings[currentSection][uCaseKey] = value;
            }
        }
    }

    return settings;
}

Settings SettingsManager::loadProfileSettings(const std::wstring &profilePath, Settings baseSettings)
{
    Settings settings = baseSettings;
    
    std::wstring fileContent = ReadUtf8File(profilePath);
    if (fileContent.empty())
    {
        DebugLog(L"Failed to open profile file: " + profilePath);
        return settings;
    }

    std::wstringstream file(fileContent);

    std::wstring line;
    std::wstring currentSection;

    while (std::getline(file, line))
    {
        line = trim(line);
        if (line.empty() || line[0] == L';')
            continue;

        if (line[0] == L'[' && line.back() == L']')
        {
            currentSection = line.substr(1, line.size() - 2);
        }
        else
        {
            // Skip lines outside of a section
            if (currentSection.empty())
                continue;

            auto delimiterPos = line.find(L'=');
            if (delimiterPos == std::wstring::npos)
                continue;

            // Skip Device and Global sections in profile
            if (currentSection == L"Devices" || currentSection == L"Global")
                continue;

            std::wstring key = trim(line.substr(0, delimiterPos));
            std::wstring value = trim(line.substr(delimiterPos + 1));

            // Convert key to uppercase for case-insensitive mapping
            std::wstring uCaseKey = key;
            std::transform(uCaseKey.begin(), uCaseKey.end(), uCaseKey.begin(), ::towupper);
            settings.mappings[currentSection][uCaseKey] = value;
        }
    }

    return settings;
}

std::vector<std::wstring> SettingsManager::getAvailableProfiles()
{
	std::vector<std::wstring> profiles;
	std::wstring basePath = getAppPath();
	WIN32_FIND_DATAW findData;
	HANDLE hFind = FindFirstFileW((basePath + L"\\settings.*.ini").c_str(), &findData);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			std::wstring filename = findData.cFileName;
			if (filename.substr(0, 9) == L"settings." && filename.substr(filename.length() - 4) == L".ini")
			{
				profiles.push_back(filename.substr(9, filename.length() - 13));
			}
		} while (FindNextFileW(hFind, &findData));
		FindClose(hFind);
	}

	return profiles;
}

bool SettingsManager::switchToProfile(const std::wstring &profileName)
{
	if (profileName.empty())
	{
		// Switch to base settings
		m_settings = loadSettings();
		m_settings.currentProfile = L"";
		return true;
	}

	std::wstring profilePath = getAppPath() + L"\\settings." + profileName + L".ini";
	if (GetFileAttributesW(profilePath.c_str()) == INVALID_FILE_ATTRIBUTES)
	{
		std::wstring message = L"Profile not found: " + profilePath;
		DebugLog(message);
		MessageBox(NULL, message.c_str(), L"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	Settings baseSettings = loadSettings();
	m_settings = loadProfileSettings(profilePath, baseSettings);
	m_settings.currentProfile = profileName;
	return true;
}

/**
 * Returns the name of the currently active profile.
 *
 * Returns an empty string if only the base settings are active.
 *
 * @return The name of the currently active profile.
 */
std::wstring SettingsManager::currentProfile()
{
	return m_settings.currentProfile;
}

/**
 * Trim whitespace from the beginning and end of a string.
 *
 * @param str The string to trim.
 * @return The trimmed string.
 */
std::wstring trim(const std::wstring &str)
{
	size_t first = str.find_first_not_of(L" \t\r\n");
	if (first == std::wstring::npos)
		return L"";

	size_t last = str.find_last_not_of(L" \t\r\n");
	return str.substr(first, last - first + 1);
}

/**
 * Convert a virtual key code to its string representation.
 * Returns the first matching key name from the keyMap.
 *
 * @param vk The virtual key code to convert.
 * @return The string representation of the key, or hexadecimal value in a string.
 */
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

/**
 * Convert a string of keys separated by '+' into a vector of INPUT structs.
 *
 * Example: "CTRL+ALT+DEL" -> {CTRL down, ALT down, DEL down, DEL up, ALT up, CTRL up}
 * Example: "CTRL+F4" -> {CTRL down, F4 down, F4 up, CTRL up}
 *
 * @param keyString The string of keys to convert.
 * @return A vector of INPUT structs.
 */
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
