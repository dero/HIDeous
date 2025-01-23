#include "settings.h"
#include "logging.h"
#include <windows.h>
#include <shlwapi.h>
#include <fstream>
#include <sstream>

std::string WideToNarrow(const wchar_t *wide)
{
	if (!wide)
		return "";
	int size = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
	if (size <= 0)
		return "";
	std::string narrow(size - 1, 0);
	WideCharToMultiByte(CP_UTF8, 0, wide, -1, &narrow[0], size, nullptr, nullptr);
	return narrow;
}

/**
 * Log a message to the debug log file.
 *
 * @param message The message to log.
 * @return void
 */
void DebugLog(const std::wstring &message)
{
	if (!getSettings().global.Debug || getSettings().global.DebugFile.empty())
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
		getAppPath() + L"\\" + getSettings().global.DebugFile,
		std::ios_base::app);
	outputLog << ss.str();
	outputLog.flush();
	outputLog.close();
}
