#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <windows.h>

class Settings
{
public:
	Settings();
	bool load();
	const std::unordered_map<std::string, std::string> &getDevices() const;
	const std::unordered_map<std::string, std::unordered_map<std::string, std::string>> &getMappings() const;
	static std::vector<INPUT> convertStringToInput(const std::string &keyString);
	static WORD stringToVirtualKeyCode(const std::string &str);
	static std::string virtualKeyCodeToString(WORD vk);

private:
	static std::string trim(const std::string &str);

	std::wstring filePath;
	std::unordered_map<std::string, std::string> devices;
	std::unordered_map<std::string, std::unordered_map<std::string, std::string>> mappings;
};
