#pragma once

#include <Windows.h>
#include <map>
#include <tlhelp32.h>

#include "Types.h"

static inline std::wstring StringToWString(const std::string& str)
{
	std::wstring wstr;
	std::size_t size;
	wstr.resize(str.length());
	mbstowcs_s(&size, &wstr[0], wstr.size() + 1, str.c_str(), str.size());
	return wstr;
}

static inline std::string WStringToString(const std::wstring& wstr)
{
	std::string str;
	std::size_t size;
	str.resize(wstr.length());
	wcstombs_s(&size, &str[0], str.size() + 1, wstr.c_str(), wstr.size());
	return str;
}

static inline tStrings argvToList(int argc, char* argv[])
{
	tStrings args;
#if UNICODE || _UNICODE
	int32_t nArgs;
	LPWSTR* pWargv = CommandLineToArgvW(GetCommandLineW(), &nArgs);
	for (int32_t i = 0; i < nArgs; i++)
		args.push_back(pWargv[i]);

	LocalFree(pWargv);
#else
	for (int32_t i = 0; i < argc; i++)
		args.push_back(argv[i]);
#endif

	return args;
}

static inline std::string ByteToHexString(const uint8_t& byte)
{
	char hex[3];
	sprintf_s(hex, "%02X", byte);
	return std::string(hex);
}

struct ProcessInfo
{
	DWORD pid;
	DWORD parentPid;
	std::wstring name;
	std::wstring parentName;
};

// Function to get the parent process ID
static inline ProcessInfo GetProcessInfo(const DWORD& pid)
{
	PROCESSENTRY32 processEntry;
	processEntry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (hSnapshot == INVALID_HANDLE_VALUE)
		return ProcessInfo{};

	std::map<DWORD, ProcessInfo> processMap;

	if (Process32First(hSnapshot, &processEntry))
	{
		do
		{
			processMap[processEntry.th32ProcessID] = ProcessInfo{ processEntry.th32ProcessID, processEntry.th32ParentProcessID, processEntry.szExeFile, L"" };
		} while (Process32Next(hSnapshot, &processEntry));
	}

	CloseHandle(hSnapshot);

	for (auto& process : processMap)
	{
		if (process.second.parentPid != 0 && processMap.find(process.second.parentPid) != processMap.end())
			process.second.parentName = processMap[process.second.parentPid].name;
	}

	if (processMap.find(pid) != processMap.end())
		return processMap[pid];

	return ProcessInfo{};
}

static inline bool IsSubProcess()
{
	const DWORD pid               = GetCurrentProcessId(); // Get the PID of the current process
	const ProcessInfo processInfo = GetProcessInfo(pid);

	return (processInfo.name == processInfo.parentName);
}
