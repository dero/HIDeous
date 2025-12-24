#pragma once
#include <string>

// Trim whitespace from the beginning and end of a string.
std::wstring trim(const std::wstring &str);

// Helper to read a UTF-8 file and convert it to std::wstring using Windows API.
std::wstring ReadUtf8File(const std::wstring &path);
