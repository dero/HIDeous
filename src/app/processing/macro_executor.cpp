#include "macro_executor.h"
#include "commands/keys_command.h"
#include "commands/text_command.h"
#include "commands/run_command.h"
#include "commands/profile_command.h"
#include "../../common/config/settings_manager.h"
#include "../../common/utils/logging.h"
#include "../../common/input/key_mapping.h"
#include "../../common/utils/constants.h"
#include "../ui/gui_types.h" // For g_lastKeypress access

#include <thread>
#include <sstream>
#include <iomanip>

// Forward declaration of RevertToggleState helper
void RevertToggleState(WORD vk)
{
    // Toggle the key again to revert the state
    // We must ensure this input has the HIDEOUS_IDENTIFIER so our hook ignores it
    INPUT inputs[2] = {};
    
    // Get correct scan code
    WORD scanCode = static_cast<WORD>(MapVirtualKey(vk, MAPVK_VK_TO_VSC));

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = vk;
    inputs[0].ki.wScan = scanCode;
    inputs[0].ki.dwFlags = 0; // Keydown
    inputs[0].ki.dwExtraInfo = HIDEOUS_IDENTIFIER;

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = vk;
    inputs[1].ki.wScan = scanCode;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[1].ki.dwExtraInfo = HIDEOUS_IDENTIFIER;

    SendInput(2, inputs, sizeof(INPUT));
    DebugLog(L"Counter-input sent to revert toggle state for key: 0x" + std::to_wstring(vk));
}

int DecideOnKey(USHORT vkCode, USHORT scanCode)
{
    const Settings &settings = SettingsManager::getInstance().getSettings();
    DWORD currentTime = GetTickCount();

    // Access g_lastKeypress. Ideally we inject this dependency or move it.
    // For now, assuming it's available via gui.h or we move it to a SharedContext.
    // Let's rely on extern declaration from gui.h for now to minimize breakage during transition.
    
    // Only consider keypresses within the permitted time frame
    if (currentTime - g_lastKeypress.timestamp > settings.global.KeyWaitTime)
    {
        return KEY_DECISION_LET_THROUGH; // Keypress too old
    }

    if (g_lastKeypress.vkCode != vkCode)
    {
        return KEY_DECISION_LET_THROUGH; // Different key
    }

    const auto &hashToDevice = settings.hashToDevice;
    const auto deviceIt = hashToDevice.find(g_lastKeypress.deviceHash);
    if (deviceIt == hashToDevice.end())
    {
        return KEY_DECISION_LET_THROUGH; // No mappings for this device
    }

    // Get macro mappings for this device
    const auto &allMappings = settings.mappings;
    const auto mappingsIt = allMappings.find(deviceIt->second);
    if (mappingsIt == allMappings.end())
    {
        return KEY_DECISION_LET_THROUGH; // No macros for this device
    }

    // Convert vkCode to string
    std::wstring keyName = virtualKeyCodeToString(g_lastKeypress.vkCode);

    // Check if key has a macro mapping
    const auto &macros = mappingsIt->second;
    auto macroIt = macros.find(keyName);

    if (macroIt == macros.end())
    {
        // Check for hex format as a fallback
        std::wostringstream ss;
        ss << L"0X" << std::uppercase << std::hex << g_lastKeypress.vkCode;

        macroIt = macros.find(ss.str());
        macroIt = macros.find(ss.str());
        if (macroIt == macros.end())
        {
            // Fallback: Check for Scan Code trigger
            // e.g. SC43
            std::wostringstream scStream;
            scStream << L"SC" << scanCode;
            macroIt = macros.find(scStream.str());
            
            if (macroIt == macros.end())
                return KEY_DECISION_LET_THROUGH; // No macro for this key
        }
    }

    // Parse macro command
    size_t colonPos = macroIt->second.find(':');
    if (colonPos == std::string::npos)
    {
        DebugLog(L"Invalid macro format: " + macroIt->second);
        return KEY_DECISION_LET_THROUGH; // Invalid macro format
    }

    std::wstring command = macroIt->second.substr(0, colonPos);
    std::wstring data = macroIt->second.substr(colonPos + 1);

    bool handled = false;

    if (command == L"keys")
    {
        std::thread(ExecuteKeys, data).detach();
        handled = true;
    }
    else if (command == L"text")
    {
        std::thread(ExecuteText, data).detach();
        handled = true;
    }
    else if (command == L"run")
    {
        std::thread(ExecuteRun, data).detach();
        handled = true;
    }
    else if (command == L"profile")
    {
        ExecuteProfile(data); // Synchronous
        handled = true;
    }
    else
    {
        DebugLog(L"Unknown macro command: " + command);
    }

    if (handled)
    {
        if (vkCode == VK_CAPITAL || vkCode == VK_NUMLOCK || vkCode == VK_SCROLL)
        {
             std::thread(RevertToggleState, static_cast<WORD>(vkCode)).detach();
        }

        g_lastKeypress.vkCode = 0;
        g_lastKeypress.timestamp = 0;
        return KEY_DECISION_BLOCK;
    }

    return KEY_DECISION_LET_THROUGH; // No valid command
}

void RefreshInternalState()
{
    HMODULE hDll = GetModuleHandle(TEXT("hideous_hook.dll"));
    if (!hDll)
    {
        DebugLog(L"Could not get handle to hideous_hook.dll for optimization refresh");
        return;
    }

    typedef void (*UpdateInterestedKeysFn)(BYTE *, BYTE *);
    auto UpdateInterestedKeys = (UpdateInterestedKeysFn)GetProcAddress(hDll, "UpdateInterestedKeys");
    if (!UpdateInterestedKeys)
    {
        DebugLog(L"Could not find UpdateInterestedKeys in DLL");
        return;
    }

    BYTE interested[256] = {0};
    BYTE interestedScans[256] = {0};
    const Settings &settings = SettingsManager::getInstance().getSettings();

    // Iterate through all device mappings
    for (const auto &devicePair : settings.mappings)
    {
        const auto &macros = devicePair.second;
        for (const auto &macroPair : macros)
        {
            // macroPair.first is the Key String (e.g. "F1", "A", or "SC43")
            USHORT scanCode = 0;
            if (isScanCodeString(macroPair.first, &scanCode))
            {
                if (scanCode < 256)
                {
                    interestedScans[scanCode] = 1;
                }
            }
            else
            {
                WORD vk = stringToVirtualKeyCode(macroPair.first);
                if (vk != 0 && vk < 256)
                {
                    interested[vk] = 1;
                }
            }
        }
    }

    UpdateInterestedKeys(interested, interestedScans);
    DebugLog(L"Internal state refreshed (Shared Memory updated)");
}
