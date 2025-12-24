#include "string_utils.h"
#include <windows.h>
#include <fstream>
#include <vector>

std::wstring trim(const std::wstring &str)
{
	size_t first = str.find_first_not_of(L" \t\r\n");
	if (first == std::wstring::npos)
		return L"";

	size_t last = str.find_last_not_of(L" \t\r\n");
	return str.substr(first, last - first + 1);
}

std::wstring ReadUtf8File(const std::wstring &path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return L"";
    }

    std::string str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    if (str.empty()) {
        return L"";
    }

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    
    // Remove BOM if present
    if (!wstrTo.empty() && wstrTo[0] == 0xFEFF) {
        wstrTo.erase(0, 1);
    }

    return wstrTo;
}
