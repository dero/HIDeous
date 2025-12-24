#pragma once
#include <windows.h>

struct RawKeyData {
    HANDLE hDevice;
    USHORT vkCode;
    USHORT flags;
};

bool GetRawKeyData(HRAWINPUT hRawInput, RawKeyData& outData);
