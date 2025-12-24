#pragma once
#include <windows.h>
#include <string>

HWND CreateProfileSelector(HWND parent);
void UpdateProfileSettingsLink(HWND hwnd);
bool SwitchToProfile(const std::wstring &profileName);
void OnProfileSelectorSelChange(HWND hCombo);
void OnProfileSettingsLinkClick(HWND hCombo);
