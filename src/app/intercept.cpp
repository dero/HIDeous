#include "intercept.h"
#include "../common/settings.h"
#include "gui.h"
#include "../common/constants.h"
#include "../common/logging.h"
#include <sstream>
#include <windows.h>
#include <future>
#include <cstdlib>
#include <thread>

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
        std::thread([data]() {
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
            }
        }).detach();

        return KEY_DECISION_BLOCK; // Macro handled
    }
    else if (command == L"text")
    {
        std::thread([data]() {
                DebugLog(L"Sending text: " + data);

                size_t i = 0;
                while (i < data.length()) {
                    if (data[i] == L'{') {
                        if (i + 1 < data.length() && data[i + 1] == L'{') {
                            // Escaped {{ -> send {
                            INPUT input = {0};
                            input.type = INPUT_KEYBOARD;
                            input.ki.wVk = 0;
                            input.ki.wScan = L'{';
                            input.ki.dwFlags = KEYEVENTF_UNICODE;
                            input.ki.dwExtraInfo = HIDEOUS_IDENTIFIER;
                            SendInput(1, &input, sizeof(INPUT));
                            input.ki.dwFlags |= KEYEVENTF_KEYUP;
                            SendInput(1, &input, sizeof(INPUT));
                            i += 2;
                        } else {
                            // Start of a control key
                            size_t closePos = data.find(L'}', i);
                            if (closePos != std::wstring::npos) {
                                std::wstring keyContent = data.substr(i + 1, closePos - i - 1);
                                std::vector<INPUT> inputs = convertStringToInput(keyContent);
                                
                                // Keydowns
                                std::vector<INPUT> firstHalfOfInputs(inputs.begin(), inputs.begin() + inputs.size() / 2);
                                // Keyups
                                std::vector<INPUT> secondHalfOfInputs(inputs.begin() + inputs.size() / 2, inputs.end());
                                
                                if (!inputs.empty()) {
                                    SendInput(firstHalfOfInputs.size(), firstHalfOfInputs.data(), sizeof(INPUT));
                                    Sleep(10); 
                                    SendInput(secondHalfOfInputs.size(), secondHalfOfInputs.data(), sizeof(INPUT));
                                }
                                i = closePos + 1;
                            } else {
                                // No checking brace, treat as literal {
                                INPUT input = {0};
                                input.type = INPUT_KEYBOARD;
                                input.ki.wVk = 0;
                                input.ki.wScan = data[i];
                                input.ki.dwFlags = KEYEVENTF_UNICODE;
                                input.ki.dwExtraInfo = HIDEOUS_IDENTIFIER;
                                SendInput(1, &input, sizeof(INPUT));
                                input.ki.dwFlags |= KEYEVENTF_KEYUP;
                                SendInput(1, &input, sizeof(INPUT));
                                i++;
                            }
                        }
                    } else if (data[i] == L'}') {
                        if (i + 1 < data.length() && data[i + 1] == L'}') {
                            // Escaped }} -> send }
                            INPUT input = {0};
                            input.type = INPUT_KEYBOARD;
                            input.ki.wVk = 0;
                            input.ki.wScan = L'}';
                            input.ki.dwFlags = KEYEVENTF_UNICODE;
                            input.ki.dwExtraInfo = HIDEOUS_IDENTIFIER;
                            SendInput(1, &input, sizeof(INPUT));
                            input.ki.dwFlags |= KEYEVENTF_KEYUP;
                            SendInput(1, &input, sizeof(INPUT));
                            i += 2;
                        } else {
                             // Treat as literal }
                            INPUT input = {0};
                            input.type = INPUT_KEYBOARD;
                            input.ki.wVk = 0;
                            input.ki.wScan = data[i];
                            input.ki.dwFlags = KEYEVENTF_UNICODE;
                            input.ki.dwExtraInfo = HIDEOUS_IDENTIFIER;
                            SendInput(1, &input, sizeof(INPUT));
                            input.ki.dwFlags |= KEYEVENTF_KEYUP;
                            SendInput(1, &input, sizeof(INPUT));
                            i++;
                        }
                    } else {
                        // Regular character
                        INPUT input = {0};
                        input.type = INPUT_KEYBOARD;
                        input.ki.wVk = 0;
                        input.ki.wScan = data[i];
                        input.ki.dwFlags = KEYEVENTF_UNICODE;
                        input.ki.dwExtraInfo = HIDEOUS_IDENTIFIER;
                        SendInput(1, &input, sizeof(INPUT));
                        Sleep(1); 
                        input.ki.dwFlags |= KEYEVENTF_KEYUP;
                        SendInput(1, &input, sizeof(INPUT));
                        i++;
                    }
                } 
        }).detach();
        return KEY_DECISION_BLOCK; // Macro handled
    }
    else if (command == L"run")
    {
        std::thread([data]() {
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

            delete[] narrowCommand;
        }).detach();

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

void RefreshInternalState()
{
    HMODULE hDll = GetModuleHandle(TEXT("hideous_hook.dll"));
    if (!hDll)
    {
        DebugLog(L"Could not get handle to hideous_hook.dll for optimization refresh");
        return;
    }

    typedef void (*UpdateInterestedKeysFn)(BYTE *);
    auto UpdateInterestedKeys = (UpdateInterestedKeysFn)GetProcAddress(hDll, "UpdateInterestedKeys");
    if (!UpdateInterestedKeys)
    {
        DebugLog(L"Could not find UpdateInterestedKeys in DLL");
        return;
    }

    BYTE interested[256] = {0};
    const Settings &settings = SettingsManager::getInstance().getSettings();

    // Iterate through all device mappings
    for (const auto &devicePair : settings.mappings)
    {
        const auto &macros = devicePair.second;
        for (const auto &macroPair : macros)
        {
            // macroPair.first is the Key String (e.g. "F1", "A")
            WORD vk = stringToVirtualKeyCode(macroPair.first);
            if (vk != 0 && vk < 256)
            {
                interested[vk] = 1;
            }
        }
    }

    UpdateInterestedKeys(interested);
    DebugLog(L"Internal state refreshed (Shared Memory updated)");
}
