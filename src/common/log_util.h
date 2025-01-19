#pragma once

#include <string>

std::string WideToNarrow(const wchar_t *wide);
void DebugLog(const std::string &message);
void CleanupSharedMemory();
bool InitializeSharedMemory();
