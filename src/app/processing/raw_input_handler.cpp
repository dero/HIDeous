#include "raw_input_handler.h"
#include <vector>

bool GetRawKeyData(HRAWINPUT hRawInput, RawKeyData& outData)
{
    UINT dataSize = 0;
    GetRawInputData(hRawInput, RID_INPUT, NULL, &dataSize, sizeof(RAWINPUTHEADER));

    if (dataSize == 0) return false;

    std::vector<BYTE> rawData(dataSize);
    if (GetRawInputData(hRawInput, RID_INPUT, rawData.data(), &dataSize, sizeof(RAWINPUTHEADER)) != dataSize)
    {
        return false;
    }

    RAWINPUT *raw = (RAWINPUT *)rawData.data();
    if (raw->header.dwType == RIM_TYPEKEYBOARD)
    {
        outData.hDevice = raw->header.hDevice;
        outData.vkCode = raw->data.keyboard.VKey;
        outData.scanCode = raw->data.keyboard.MakeCode;
        outData.flags = raw->data.keyboard.Flags;
        return true;
    }

    return false;
}
