#include "run_command.h"
#include "../../../common/utils/logging.h"
#include <windows.h>
#include <vector>

void ExecuteRun(std::wstring data) {
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
        static_cast<int>(data.size() + 1),
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
}
