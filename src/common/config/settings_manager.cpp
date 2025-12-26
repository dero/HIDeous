#include "settings_manager.h"
#include "../utils/string_utils.h"
#include "../utils/logging.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <windows.h>

SettingsManager &SettingsManager::getInstance()
{
	static SettingsManager instance;
	return instance;
}

SettingsManager::SettingsManager()
{
	m_settings = loadSettings();
    m_stopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

SettingsManager::~SettingsManager()
{
    stopWatching();
    if (m_stopEvent) {
        CloseHandle(m_stopEvent);
    }
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
	baseSettings.mappings.clear(); // Fix: Clear mappings to avoid inheriting bindings from the default profile
	m_settings = loadProfileSettings(profilePath, baseSettings);
	m_settings.currentProfile = profileName;
	return true;
}

std::wstring SettingsManager::currentProfile()
{
	return m_settings.currentProfile;
}

void SettingsManager::startWatching(ChangeCallback callback)
{
    if (m_watching) return;
    
    m_callback = callback;
    m_watching = true;
    ResetEvent(m_stopEvent);
    m_watcherThread = std::thread(&SettingsManager::watchLoop, this);
}

void SettingsManager::stopWatching()
{
    if (!m_watching) return;

    m_watching = false;
    if (m_stopEvent) {
        SetEvent(m_stopEvent);
    }
    
    if (m_watcherThread.joinable()) {
        m_watcherThread.join();
    }
}

void SettingsManager::reload()
{
    std::wstring current = m_settings.currentProfile;
    if (current.empty()) {
        m_settings = loadSettings();
    } else {
        Settings base = loadSettings();
        m_settings = loadProfileSettings(getAppPath() + L"\\settings." + current + L".ini", base);
    }
    m_settings.currentProfile = current;

    if (m_callback) {
        m_callback();
    }
}

void SettingsManager::watchLoop()
{
    std::wstring appPath = getAppPath();
    HANDLE hDir = CreateFileW(
        appPath.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL
    );

    if (hDir == INVALID_HANDLE_VALUE) return;

    OVERLAPPED overlapped = {0};
    overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    BYTE buffer[1024];
    HANDLE waitHandles[2] = { overlapped.hEvent, m_stopEvent };

    while (m_watching) {
        DWORD bytesReturned = 0;
        BOOL result = ReadDirectoryChangesW(
            hDir,
            buffer,
            sizeof(buffer),
            FALSE, // Don't watch subtree (settings files are in root app path)
            FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME,
            &bytesReturned,
            &overlapped,
            NULL
        );

        if (!result && GetLastError() != ERROR_IO_PENDING) break;

        DWORD wait = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);

        if (wait == WAIT_OBJECT_0) {
            // File changed
            GetOverlappedResult(hDir, &overlapped, &bytesReturned, TRUE);
            
            bool shouldReload = false;
            FILE_NOTIFY_INFORMATION* fileInfo = (FILE_NOTIFY_INFORMATION*)buffer;
            while (fileInfo) {
                std::wstring fileName(fileInfo->FileName, fileInfo->FileNameLength / sizeof(wchar_t));
                if (fileName == L"settings.ini" || 
                   (fileName.substr(0, 9) == L"settings." && fileName.substr(fileName.length() - 4) == L".ini")) {
                    shouldReload = true;
                    break;
                }
                if (fileInfo->NextEntryOffset == 0) break;
                fileInfo = (FILE_NOTIFY_INFORMATION*)((BYTE*)fileInfo + fileInfo->NextEntryOffset);
            }

            if (shouldReload) {
                // Debounce - wait a bit to ensure file is closed by editor
                Sleep(200);
                reload();
            }

            ResetEvent(overlapped.hEvent);
        } else {
            // stopEvent or error
            CancelIo(hDir);
            break;
        }
    }

    CloseHandle(overlapped.hEvent);
    CloseHandle(hDir);
}
