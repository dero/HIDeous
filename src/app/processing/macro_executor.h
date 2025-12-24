#pragma once
#include <windows.h>
#include <string>

#define KEY_DECISION_LET_THROUGH 0
#define KEY_DECISION_BLOCK 1

int DecideOnKey(USHORT vkCode);
void RefreshInternalState();
