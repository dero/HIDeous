#include "text_command.h"
#include "../../../common/utils/logging.h"
#include "../../../common/utils/constants.h"
#include "../../../common/input/key_mapping.h"
#include <sstream>
#include <iomanip>
#include <thread>

void ExecuteText(std::wstring data) {
    DebugLog(L"Sending text: " + data);

    std::wstringstream hexLog;
    hexLog << L"Raw text data: ";
    for (wchar_t c : data) {
        hexLog << L"0x" << std::uppercase << std::hex << (unsigned int)(unsigned short)c << L" ";
    }
    DebugLog(hexLog.str());

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
                        SendInput(static_cast<UINT>(firstHalfOfInputs.size()), firstHalfOfInputs.data(), sizeof(INPUT));
                        Sleep(10); 
                        SendInput(static_cast<UINT>(secondHalfOfInputs.size()), secondHalfOfInputs.data(), sizeof(INPUT));
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
            // Regular character or Surrogate Pair
            bool handled = false;
            if (i + 1 < data.length()) {
                WORD high = static_cast<WORD>(data[i]);
                WORD low = static_cast<WORD>(data[i + 1]);

                // Check for High Surrogate (0xD800–0xDBFF) followed by Low Surrogate (0xDC00–0xDFFF)
                if ((high >= 0xD800 && high <= 0xDBFF) &&
                    (low >= 0xDC00 && low <= 0xDFFF)) {
                    
                    std::wstringstream ss;
                    ss << L"Sending surrogate pair: 0x" << std::uppercase << std::hex << high 
                       << L" 0x" << low;
                    DebugLog(ss.str());

                    INPUT inputs[4] = {};
                    
                    // High Down
                    inputs[0].type = INPUT_KEYBOARD;
                    inputs[0].ki.wScan = high;
                    inputs[0].ki.dwFlags = KEYEVENTF_UNICODE;
                    inputs[0].ki.dwExtraInfo = HIDEOUS_IDENTIFIER;

                    // High Up
                    inputs[1] = inputs[0];
                    inputs[1].ki.dwFlags |= KEYEVENTF_KEYUP;

                    // Low Down
                    inputs[2].type = INPUT_KEYBOARD;
                    inputs[2].ki.wScan = data[i + 1];
                    inputs[2].ki.dwFlags = KEYEVENTF_UNICODE;
                    inputs[2].ki.dwExtraInfo = HIDEOUS_IDENTIFIER;

                    // Low Up
                    inputs[3] = inputs[2];
                    inputs[3].ki.dwFlags |= KEYEVENTF_KEYUP;
                    
                    SendInput(4, inputs, sizeof(INPUT));
                    
                    i += 2;
                    handled = true;
                }
            }

            if (!handled) {
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
    } 
}
