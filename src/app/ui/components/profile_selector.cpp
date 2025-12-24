#include "profile_selector.h"
#include "../main_window.h"
#include "../../resource.h"
#include "../../../common/config/settings_manager.h"
#include "../../processing/macro_executor.h"
#include "../../../common/utils/logging.h"
#include <shellapi.h>

static bool g_isProfileSwitching = false;

void UpdateProfileSettingsLink(HWND hwnd)
{
	HWND hProfileLink = GetDlgItem(hwnd, IDC_PROFILE_SETTINGS_LINK);
	std::wstring profile = SettingsManager::getInstance().currentProfile();

	if (profile.empty())
	{
		// No profile selected (i.e. only "Default" settings)
		ShowWindow(hProfileLink, SW_HIDE);
	}
	else
	{
		// Show the link to the profile settings file
		std::wstring filename = L"Edit settings." + profile + L".ini";
		SetWindowText(hProfileLink, filename.c_str());
		ShowWindow(hProfileLink, SW_SHOW);
	}
}

HWND CreateProfileSelector(HWND parent)
{
	// Create taller combo box (increased height from 200 to 300)
	HWND hCombo = CreateWindowEx(0, L"COMBOBOX", NULL,
								 WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
								 10, 10, 200, 300, parent,
								 (HMENU)IDC_PROFILE_SELECTOR, GetModuleHandle(NULL), NULL);

	// Create the settings.ini link (moved down by 5 pixels for better spacing)
	HWND hSettingsLink = CreateWindowEx(0, L"STATIC", L"Edit settings.ini",
										WS_CHILD | WS_VISIBLE | SS_NOTIFY,
										10, 40, 80, 20, parent,
										(HMENU)IDC_SETTINGS_LINK, GetModuleHandle(NULL), NULL);

	// Create the profile settings link below the first link
	HWND hProfileLink = CreateWindowEx(0, L"STATIC", L"",
									   WS_CHILD | SS_NOTIFY,
									   100, 40, 150, 20, parent,
									   (HMENU)IDC_PROFILE_SETTINGS_LINK, GetModuleHandle(NULL), NULL);

	// Set the font to underlined
	HFONT hFont = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, TRUE, FALSE,
							 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
							 DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
	SendMessage(hSettingsLink, WM_SETFONT, (WPARAM)hFont, TRUE);
	SendMessage(hProfileLink, WM_SETFONT, (WPARAM)hFont, TRUE);

	// Add "Default" profile first
	SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"Profile: Default");

	// Get all available profiles
	auto profiles = SettingsManager::getInstance().getAvailableProfiles();
	for (const auto &profile : profiles)
	{
		const std::wstring profileName = L"Profile: " + profile;
		SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)profileName.c_str());
	}

	// Select default profile
	SendMessage(hCombo, CB_SETCURSEL, 0, 0);

	UpdateProfileSettingsLink(parent);

	return hCombo;
}

bool SwitchToProfile(const std::wstring &profileName)
{
	const std::wstring currentProfile = SettingsManager::getInstance().currentProfile();

	// Skip if we're already on the same profile
	if (currentProfile == profileName)
	{
		return true;
	}

	if (SettingsManager::getInstance().switchToProfile(profileName))
	{
		// Set flag to prevent recursion
		g_isProfileSwitching = true;

		// Main window handle
		HWND hwnd = GetMainWindow();

		// Find and select the matching profile in the combo box
		HWND hCombo = GetDlgItem(hwnd, IDC_PROFILE_SELECTOR);

		std::wstring searchText = L"Profile: " + profileName;
		if (profileName.empty())
		{
			searchText = L"Profile: Default";
		}

		int itemCount = SendMessage(hCombo, CB_GETCOUNT, 0, 0);
		for (int i = 0; i < itemCount; i++)
		{
			WCHAR buffer[256];
			SendMessage(hCombo, CB_GETLBTEXT, i, (LPARAM)buffer);
			if (searchText == buffer)
			{
				SendMessage(hCombo, CB_SETCURSEL, i, 0);
				break;
			}
		}

		UpdateProfileSettingsLink(hwnd);

		// Clear flag after we're done
		g_isProfileSwitching = false;
        RefreshInternalState();
		return true;
	}
	return false;
}

void OnProfileSelectorSelChange(HWND hCombo)
{
    // Skip if we're already in the process of switching
    if (g_isProfileSwitching)
        return;

    int index = SendMessage(hCombo, CB_GETCURSEL, 0, 0);
    if (index != CB_ERR)
    {
        WCHAR buffer[256];
        SendMessage(hCombo, CB_GETLBTEXT, index, (LPARAM)buffer);
        std::wstring profile = buffer;
        std::wstring profileName = profile.substr(9); // Remove "Profile: "

        if (index == 0)
        {
            profileName = L""; // Default profile
        }

        if (!SwitchToProfile(profileName))
        {
            MessageBox(GetMainWindow(), L"Failed to switch to profile", L"Error", MB_OK | MB_ICONERROR);
        }
    }
}

void OnProfileSettingsLinkClick(HWND hCombo)
{
    int index = SendMessage(hCombo, CB_GETCURSEL, 0, 0);
    if (index > 0) // Not "Default"
    {
        WCHAR buffer[256];
        SendMessage(hCombo, CB_GETLBTEXT, index, (LPARAM)buffer);
        std::wstring profile = buffer;
        std::wstring profileName = profile.substr(9); // Remove "Profile: "
        std::wstring filename = L"settings." + profileName + L".ini";
        ShellExecute(NULL, L"open", filename.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }
}
