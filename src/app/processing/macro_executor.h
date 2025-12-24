#pragma once
#include <windows.h>
#include <string>

#define KEY_DECISION_LET_TH// Returns 1 if key should be blocked, 0 if it should be passed through
#define KEY_DECISION_BLOCK 1

int DecideOnKey(USHORT vkCode, USHORT scanCode);

void RefreshInternalState();
