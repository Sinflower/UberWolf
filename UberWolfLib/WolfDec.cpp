#include "WolfDec.h"

#include <DXLib/DXArchive.h>
#include <DXLib/DXArchiveVer5.h>
#include <DXLib/DXArchiveVer6.h>
#include <DXLib/FileLib.h>

#include <nlohmann/json.hpp>

#include <windows.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>

#include "Utils.h"

namespace fs = std::filesystem;

const DecryptModes DEFAULT_DECRYPT_MODES = {
	{"Wolf RPG v2.01", &DXArchive_VER5::DecodeArchive, std::vector<unsigned char>{ 0x0f, 0x53, 0xe1, 0x3e, 0x04, 0x37, 0x12, 0x17, 0x60, 0x0f, 0x53, 0xe1 } },
	{"Wolf RPG v2.10", &DXArchive_VER5::DecodeArchive, std::vector<unsigned char>{ 0x4c, 0xd9, 0x2a, 0xb7, 0x28, 0x9b, 0xac, 0x07, 0x3e, 0x77, 0xec, 0x4c } },
	{"Wolf RPG v2.20", &DXArchive_VER6::DecodeArchive, std::vector<unsigned char>{ 0x38, 0x50, 0x40, 0x28, 0x72, 0x4f, 0x21, 0x70, 0x3b, 0x73, 0x35, 0x38 } },
	{"Wolf RPG v2.225", &DXArchive::DecodeArchive, "WLFRPrO!p(;s5((8P@((UFWlu$#5(=" },
	{"Wolf RPG v3.00", &DXArchive::DecodeArchive, std::vector<unsigned char>{ 0x0F, 0x53, 0xE1, 0x3E, 0x8E, 0xB5, 0x41, 0x91, 0x52, 0x16, 0x55, 0xAE, 0x34, 0xC9, 0x8F, 0x79, 0x59, 0x2F, 0x59, 0x6B, 0x95, 0x19, 0x9B, 0x1B, 0x35, 0x9A, 0x2F, 0xDE, 0xC9, 0x7C, 0x12, 0x96, 0xC3, 0x14, 0xB5, 0x0F, 0x53, 0xE1, 0x3E, 0x8E, 0x00 } },
	{"Wolf RPG v3.14", &DXArchive::DecodeArchive, std::vector<unsigned char>{ 0x31, 0xF9, 0x01, 0x36, 0xA3, 0xE3, 0x8D, 0x3C, 0x7B, 0xC3, 0x7D, 0x25, 0xAD, 0x63, 0x28, 0x19, 0x1B, 0xF7, 0x8E, 0x6C, 0xC4, 0xE5, 0xE2, 0x76, 0x82, 0xEA, 0x4F, 0xED, 0x61, 0xDA, 0xE0, 0x44, 0x5B, 0xB6, 0x46, 0x3B, 0x06, 0xD5, 0xCE, 0xB6, 0x78, 0x58, 0xD0, 0x7C, 0x82, 0x00 } },
	{"One Way Heroics", &DXArchive::DecodeArchive, "nGui9('&1=@3#a" },
	{"One Way Heroics Plus", &DXArchive::DecodeArchive, "Ph=X3^]o2A(,1=@3#a" }
};

const tStrings IGNORE_WOLF_FILES = {
	TEXT("Game.wolf"),
	TEXT("List.wolf"),
	TEXT("Data2.wolf"),
	TEXT("GameFile.wolf"),
	TEXT("BasicData2.wolf")
};

WolfDec::WolfDec(const std::wstring& progName, const uint32_t& mode, const bool& isSubProcess) :
	m_progName(progName),
	m_mode(mode),
	m_isSubProcess(isSubProcess)
{
	loadConfig();
}

WolfDec::~WolfDec()
{
}

bool WolfDec::UnpackArchive(const TCHAR* pFilePath)
{
	TCHAR fullPath[MAX_PATH];
	TCHAR filePath[MAX_PATH];
	TCHAR directoryPath[MAX_PATH];
	TCHAR fileName[MAX_PATH];
	bool failed = true;

	ConvertFullPath__(pFilePath, fullPath);
	AnalysisFileNameAndDirPath(fullPath, filePath, directoryPath);
	AnalysisFileNameAndExeName(filePath, fileName, NULL);

	tString outDir = tString(directoryPath) + TEXT("/") + tString(fileName);

	// Check if the basename of the file is in the ignore list
	if (std::find(IGNORE_WOLF_FILES.begin(), IGNORE_WOLF_FILES.end(), FS_PATH_TO_TSTRING(fs::path(pFilePath).filename())) != IGNORE_WOLF_FILES.end())
		return true;

	// Check if the file is already unpacked, i.e., if the directory exists and is not empty
	if (fs::exists(outDir) && !fs::is_empty(outDir))
		return true;

	if (m_mode == -1)
		return detectMode(pFilePath);

	if (m_mode >= (DEFAULT_DECRYPT_MODES.size() + m_additionalModes.size()))
	{
		std::cerr << "Specified Mode: " << m_mode << " out of range" << std::endl;
		if (m_isSubProcess)
			ExitProcess(1);
		else
			return false;
	}

	const DecryptMode curMode = (m_mode < DEFAULT_DECRYPT_MODES.size() ? DEFAULT_DECRYPT_MODES.at(m_mode) : m_additionalModes.at(m_mode - DEFAULT_DECRYPT_MODES.size()));

	SetCurrentDirectory(directoryPath);

	CreateDirectory(fileName, NULL);

	SetCurrentDirectory(fileName);

	failed = curMode.decFunc(fullPath, TEXT(""), curMode.key.data()) < 0;

	if (failed)
	{
		SetCurrentDirectory(directoryPath);
		RemoveDirectory(fileName);
	}

	if (m_isSubProcess)
		ExitProcess(failed);

	return !failed;
}

void WolfDec::AddKey(const std::string& name, const bool& useOldDxArc, const Key& key)
{
	m_additionalModes.push_back({ name, (useOldDxArc ? &DXArchive_VER6::DecodeArchive : &DXArchive::DecodeArchive), key });
}

void WolfDec::loadConfig()
{
	// Return if the config file does not exist or is empty
	if (!fs::exists(CONFIG_FILE_NAME) || fs::file_size(CONFIG_FILE_NAME) == 0)
		return;

	try
	{
		std::ifstream f(CONFIG_FILE_NAME);
		nlohmann::json data = nlohmann::json::parse(f);

		if (data.contains("keys"))
		{
			for (const auto& [name, value] : data["keys"].items())
			{
				if (value.contains("mode") && value.contains("key"))
				{
					std::string mode = value["mode"];
					DecryptFunction decFunc = nullptr;
					std::transform(mode.begin(), mode.end(), mode.begin(), [](const unsigned char& c) { return std::tolower(c); });
					if (mode == "ver5")
						decFunc = &DXArchive_VER5::DecodeArchive;
					else if (mode == "ver6")
						decFunc = &DXArchive_VER6::DecodeArchive;
					else if (mode == "ver8")
						decFunc = &DXArchive::DecodeArchive;
					else
						throw std::runtime_error("Invalid mode: " + mode);

					std::vector<unsigned char> key;

					if (value["key"].is_array())
					{
						for (const auto& v : value["key"])
							key.push_back(static_cast<unsigned char>(std::stoul(std::string(v), nullptr, 16)));

						if (key.back() != 0x00)
							key.push_back(0x00);
					}
					else
					{
						std::string keyStr = value["key"];
						key = std::vector<unsigned char>(keyStr.begin(), keyStr.end());
						key.push_back(0x00);
					}

					m_additionalModes.push_back({ name, decFunc, key });
				}
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
}

bool WolfDec::detectMode(const TCHAR* pFilePath)
{
	bool success = false;

	if (m_mode == -1)
	{
		for (uint32_t i = 0; i < DEFAULT_DECRYPT_MODES.size(); i++)
		{
			success = runProcess(pFilePath, i);
			if (success)
			{
				m_mode = i;
				break;
			}
		}

		for (uint32_t i = 0; i < m_additionalModes.size(); i++)
		{
			success = runProcess(pFilePath, static_cast<uint32_t>(DEFAULT_DECRYPT_MODES.size() + i));
			if (success)
			{
				m_mode = static_cast<uint32_t>(DEFAULT_DECRYPT_MODES.size() + i);
				break;
			}
		}
	}
	else
		success = runProcess(pFilePath, m_mode);

	return success;
}

bool WolfDec::runProcess(const TCHAR* pFilePath, const uint32_t& mode)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	const std::wstring wstr = m_progName + L" -m " + std::to_wstring(mode) + L" \"" + std::wstring(pFilePath) + L"\"";

	if (!CreateProcess(NULL, const_cast<LPWSTR>(wstr.c_str()), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		std::cerr << "CreateProcess() failed: " << GetLastError() << std::endl;
		return false;
	}

	// Wait until child process exits.
	WaitForSingleObject(pi.hProcess, INFINITE);

	bool success = false;

	DWORD ec;
	if (FALSE == GetExitCodeProcess(pi.hProcess, &ec))
		std::cerr << "GetExitCodeProcess() failed: " << GetLastError() << std::endl;
	else
		success = (ec == 0);

	// Close process and thread handles. 
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return success;
}
