#pragma once

#include <string>
#include <unordered_map>
#include <windows.h>

#define REG_KEY_PATH L"Software\\HIDeous"
#define REG_VALUE_NAME L"AppPath"
#define DEFAULT_KEY_WAIT_TIME 30
#define DEFAULT_DEBUG_MODE false
#define DEFAULT_DEBUG_FILE L"debug.log"

struct GlobalSettings
{
    bool Debug;
    std::wstring DebugFile;
    int KeyWaitTime;
};

struct Settings
{
    GlobalSettings global;
    std::unordered_map<std::wstring, std::wstring> deviceToHash;
    std::unordered_map<std::wstring, std::wstring> hashToDevice;
    std::unordered_map<std::wstring, std::unordered_map<std::wstring, std::wstring>> mappings;
    std::wstring currentProfile; // Name of current profile, empty for base settings
};

class SettingsManager
{
public:
    static SettingsManager &getInstance();

    const Settings &getSettings() const { return m_settings; }
    std::vector<std::wstring> getAvailableProfiles();
    bool switchToProfile(const std::wstring &profileName);
    bool persistSettingsPath(const std::wstring &path);
    std::wstring SettingsManager::currentProfile();
    std::wstring getAppPath();

private:
    SettingsManager();
    ~SettingsManager() = default;
    SettingsManager(const SettingsManager &) = delete;
    SettingsManager &operator=(const SettingsManager &) = delete;

    Settings loadSettings();
    Settings loadProfileSettings(const std::wstring &profilePath, Settings baseSettings);

    Settings m_settings;
};

// Utility functions
std::wstring trim(const std::wstring &str);
std::wstring virtualKeyCodeToString(WORD vk);
WORD stringToVirtualKeyCode(const std::wstring &str);
std::vector<INPUT> convertStringToInput(const std::wstring &keyString);

extern std::unordered_map<std::wstring, WORD> keyMap;
