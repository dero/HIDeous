#include "intercept.h"
#include "../common/settings.h"
#include "gui.h"
#include "../common/constants.h"
#include "../common/logging.h"
#include <sstream>
#include <windows.h>

int DecideOnKey(USHORT vkCode)
{
    // Only consider keypresses within the last 30ms
    DWORD currentTime = GetTickCount();

    if (currentTime - g_lastKeypress.timestamp > getSettings().global.KeyWaitTime)
    {
        return KEY_DECISION_LET_THROUGH; // Keypress too old
    }

    if (g_lastKeypress.vkCode != vkCode)
    {
        return KEY_DECISION_LET_THROUGH; // Different key
    }

    const auto &hashToDevice = getSettings().hashToDevice;
    const auto deviceIt = hashToDevice.find(g_lastKeypress.deviceHash);
    if (deviceIt == hashToDevice.end())
    {
        return KEY_DECISION_LET_THROUGH; // No mappings for this device
    }

    // Get macro mappings for this device
    const auto &allMappings = getSettings().mappings;
    const auto mappingsIt = allMappings.find(deviceIt->second);
    if (mappingsIt == allMappings.end())
    {
        return KEY_DECISION_LET_THROUGH; // No macros for this device
    }

    // Convert vkCode to string
    std::wstring keyName = virtualKeyCodeToString(g_lastKeypress.vkCode);

    // Check if key has a macro mapping
    const auto &macros = mappingsIt->second;
    const auto macroIt = macros.find(keyName);

    if (macroIt == macros.end())
    {
        // Check for hex format as a fallback
        std::wostringstream ss;
        ss << L"0x" << std::hex << g_lastKeypress.vkCode;

        const auto macroKeyIt = macros.find(ss.str());
        if (macroKeyIt == macros.end())
            return KEY_DECISION_LET_THROUGH; // No macro for this key
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

    if (command == L"keys")
    {
        // Convert key sequence to INPUT events
        std::vector<INPUT> inputs = convertStringToInput(data);

        // Keydowns
        std::vector<INPUT> firstHalfOfInputs(inputs.begin(), inputs.begin() + inputs.size() / 2);

        // Keyups
        std::vector<INPUT> secondHalfOfInputs(inputs.begin() + inputs.size() / 2, inputs.end());

        DebugLog(L"Sending keys: " + data);

        if (!inputs.empty())
        {
            SendInput(firstHalfOfInputs.size(), firstHalfOfInputs.data(), sizeof(INPUT));
            SendInput(secondHalfOfInputs.size(), secondHalfOfInputs.data(), sizeof(INPUT));

            return KEY_DECISION_BLOCK; // Macro handled
        }
    }
    else if (command == L"text")
    {
        // Convert text to Unicode and send
        std::wstring wtext(data.begin(), data.end());

        DebugLog(L"Sending text: " + wtext);

        for (wchar_t c : wtext)
        {
            INPUT input = {0};
            input.type = INPUT_KEYBOARD;
            input.ki.wVk = 0;
            input.ki.wScan = c;
            input.ki.dwFlags = KEYEVENTF_UNICODE;
            input.ki.dwExtraInfo = HIDEOUS_IDENTIFIER;
            SendInput(1, &input, sizeof(INPUT));

            input.ki.dwFlags |= KEYEVENTF_KEYUP;
            SendInput(1, &input, sizeof(INPUT));
        }
        return KEY_DECISION_BLOCK; // Macro handled
    }
    else
    {
        DebugLog(L"Unknown macro command: " + command);
    }

    return KEY_DECISION_LET_THROUGH; // No valid command
}
