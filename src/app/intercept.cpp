#include "intercept.h"
#include "../common/settings.h"
#include "gui.h"
#include "../common/constants.h"
#include "../common/logging.h"
#include <sstream>
#include <windows.h>
#include <future>
#include <cstdlib>

int DecideOnKey(USHORT vkCode)
{
    const Settings &settings = SettingsManager::getInstance().getSettings();
    DWORD currentTime = GetTickCount();

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
        if (macroIt == macros.end())
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
        auto future = std::async(std::launch::async, [&data]()
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
                Sleep(10); // Delay between keydown and keyup to prevent sticky keys
                SendInput(secondHalfOfInputs.size(), secondHalfOfInputs.data(), sizeof(INPUT));
            } });

        return KEY_DECISION_BLOCK; // Macro handled
    }
    else if (command == L"text")
    {
        auto future = std::async(std::launch::async, [&data]()
                                 {
                DebugLog(L"Sending text: " + data);

                for (wchar_t c : data)
                {
                    INPUT input = {0};
                    input.type = INPUT_KEYBOARD;
                    input.ki.wVk = 0;
                    input.ki.wScan = c;
                    input.ki.dwFlags = KEYEVENTF_UNICODE;
                    input.ki.dwExtraInfo = HIDEOUS_IDENTIFIER;
                    SendInput(1, &input, sizeof(INPUT));

                    Sleep(1); // Simulate ultra-fast typing

                    input.ki.dwFlags |= KEYEVENTF_KEYUP;
                    SendInput(1, &input, sizeof(INPUT));
                } });
        return KEY_DECISION_BLOCK; // Macro handled
    }
    else if (command == L"run")
    {
        auto future = std::async(std::launch::async, [&data]()
                                 {
                                    char *narrowCommand = new char[data.size() + 1];
                                    WideCharToMultiByte(
                                        // The default code page
                                        CP_ACP,
                                        // Flags indicating invalid characters
                                        0,
                                        // The wide-character string
                                        data.c_str(),
                                        // The number of wide-character characters in the string, -1 if null-terminated
                                        -1,
                                        // The buffer to receive the converted string
                                        narrowCommand,
                                        // The size of the buffer
                                        data.size() + 1,
                                        // A pointer to a default character
                                        NULL,
                                        // A pointer to a flag that indicates if a default character was used
                                        NULL);

                                    if (narrowCommand == nullptr)
                                    {
                                        DebugLog(L"Failed to convert command to ASCII");
                                    } else {
                                        DebugLog(L"Sending system command: " + data);
                                        system(narrowCommand);
                                    }

                                    delete[] narrowCommand; });

        return KEY_DECISION_BLOCK; // Macro handled
    }
    else if (command == L"profile")
    {
        DebugLog(L"Switching to profile: " + data);

        if (data == L"Default")
        {
            data = L""; // Default profile
        }

        SwitchToProfile(data);

        return KEY_DECISION_BLOCK; // Macro handled
    }
    else
    {
        DebugLog(L"Unknown macro command: " + command);
    }

    return KEY_DECISION_LET_THROUGH; // No valid command
}
