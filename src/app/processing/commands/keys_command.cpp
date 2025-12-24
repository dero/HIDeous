#include "keys_command.h"
#include "../../../common/utils/logging.h"
#include "../../../common/input/key_mapping.h"

void ExecuteKeys(std::wstring data) {
    // Convert key sequence to INPUT events
    std::vector<INPUT> inputs = convertStringToInput(data);

    // Keydowns
    std::vector<INPUT> firstHalfOfInputs(inputs.begin(), inputs.begin() + inputs.size() / 2);

    // Keyups
    std::vector<INPUT> secondHalfOfInputs(inputs.begin() + inputs.size() / 2, inputs.end());

    DebugLog(L"Sending keys: " + data);

    if (!inputs.empty())
    {
        SendInput(static_cast<UINT>(firstHalfOfInputs.size()), firstHalfOfInputs.data(), sizeof(INPUT));
        Sleep(10); // Delay between keydown and keyup to prevent sticky keys
        SendInput(static_cast<UINT>(secondHalfOfInputs.size()), secondHalfOfInputs.data(), sizeof(INPUT));
    }
}
