#include "profile_command.h"
#include "../../../common/utils/logging.h"
#include "../../../common/config/settings_manager.h"
#include "../../ui/main_window.h" // For GetMainWindow
#include "../../ui/gui_types.h"   // For WM_SETTINGS_CHANGED

void ExecuteProfile(std::wstring data) {
    DebugLog(L"Switching to profile: " + data);

    if (data == L"Default")
    {
        data = L""; // Default profile
    }

    // Direct call to SettingsManager
    if (SettingsManager::getInstance().switchToProfile(data))
    {
        // Notify UI and Hook to refresh
        HWND hwnd = GetMainWindow();
        if (hwnd)
        {
            PostMessage(hwnd, WM_SETTINGS_CHANGED, 0, 0);
        }
    }
}
