/*
 *  File: Utils.h
 *  Copyright (c) 2023 Sinflower
 *
 *  MIT License
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 */

#pragma once

#include <Windows.h>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <tlhelp32.h>
#include <vector>

#include "Types.h"

struct ProcessInfo
{
	DWORD pid;
	DWORD parentPid;
	std::wstring name;
	std::wstring parentName;
};

inline std::wstring StringToWString(const std::string& str)
{
	std::wstring wstr;
	std::size_t size;
	wstr.resize(str.length());
	mbstowcs_s(&size, &wstr[0], wstr.size() + 1, str.c_str(), str.size());
	return wstr;
}

inline std::string WStringToString(const std::wstring& wstr)
{
	std::string str;
	std::size_t size;
	str.resize(wstr.length());
	wcstombs_s(&size, &str[0], str.size() + 1, wstr.c_str(), wstr.size());
	return str;
}

inline tStrings argvToList(int argc, char* argv[])
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

inline std::string ByteToHexString(const uint8_t& byte)
{
	char hex[3];
	sprintf_s(hex, "%02X", byte);
	return std::string(hex);
}

// Function to get the parent process ID
inline ProcessInfo GetProcessInfo(const DWORD& pid)
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

inline bool IsSubProcess()
{
	const DWORD pid               = GetCurrentProcessId(); // Get the PID of the current process
	const ProcessInfo processInfo = GetProcessInfo(pid);

	return (processInfo.name == processInfo.parentName);
}

inline std::vector<uint8_t> file2Buffer(const std::filesystem::path& filePath)
{
	std::ifstream inFile(filePath, std::ios::binary);
	if (!inFile)
		throw std::runtime_error("Failed to open file: " + filePath.string());

	std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
	inFile.close();

	if (buffer.empty())
		throw std::runtime_error("File is empty: " + filePath.string());

	return buffer;
}

inline void buffer2File(const std::filesystem::path& filePath, const std::vector<uint8_t>& buffer)
{
	std::ofstream outFile(filePath, std::ios::binary);
	if (!outFile)
		throw std::runtime_error("Failed to open output file: " + filePath.string());
	outFile.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
	outFile.close();
}

inline void backupFile(const std::filesystem::path& filePath, const std::filesystem::path& backupFolder)
{
	std::filesystem::path backupFilePath = backupFolder / filePath.filename();
	if (!std::filesystem::exists(backupFilePath))
	{
		std::filesystem::copy_file(filePath, backupFilePath, std::filesystem::copy_options::overwrite_existing);
		std::cout << "Backup created: " << backupFilePath << std::endl;
	}
}
