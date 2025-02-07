#include "settings.h"
#include "logging.h"
#include <windows.h>
#include <shlwapi.h>
#include <fstream>
#include <sstream>

/**
 * Log a message to the debug log file.
 *
 * @param message The message to log.
 * @return void
 */
void DebugLog(const std::wstring &message)
{
	const std::wstring appPath = SettingsManager::getInstance().getAppPath();
	const Settings &settings = SettingsManager::getInstance().getSettings();

	if (!settings.global.Debug || settings.global.DebugFile.empty())
	{
		return;
	}

	DWORD processId = GetCurrentProcessId();
	DWORD threadId = GetCurrentThreadId();
	std::wostringstream ss;

	// Precise time
	SYSTEMTIME st;
	GetLocalTime(&st);

	ss << "[" << st.wHour << ":" << st.wMinute << ":" << st.wSecond << "." << st.wMilliseconds << "] ";
	ss << "[PID: " << processId << " TID: " << threadId << "] " << message << "\n";

	// Open log file for appending, write message, and close.
	std::wofstream outputLog;

	outputLog.open(
		appPath + L"\\" + settings.global.DebugFile,
		std::ios_base::app);
	outputLog << ss.str();
	outputLog.flush();
	outputLog.close();
}
