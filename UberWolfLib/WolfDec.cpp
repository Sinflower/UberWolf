/*
 *  File: WolfDec.cpp
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

// TODO: For Pro Games always detect the key before unpacking and skip mode detection

#include "WolfDec.h"

#include <DXLib/DXArchive.h>
#include <DXLib/DXArchiveVer5.h>
#include <DXLib/DXArchiveVer6.h>
#include <DXLib/FileLib.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <codecvt>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <windows.h>

#include "UberLog.h"
#include "Utils.h"
#include "WolfUtils.h"

namespace fs = std::filesystem;

static constexpr uint16_t PRO_CRYPT_VERSION = 1000;
static constexpr uint16_t CC2_PRO_VERSION   = 0xC8;

const CryptModes DEFAULT_CRYPT_MODES = {
	{ "Wolf RPG v2.01", 0x0, &DXArchive_VER5::DecodeArchive, &DXArchive_VER5::EncodeArchiveOneDirectory, std::vector<unsigned char>{ 0x0f, 0x53, 0xe1, 0x3e, 0x04, 0x37, 0x12, 0x17, 0x60, 0x0f, 0x53, 0xe1 } },
	{ "Wolf RPG v2.10", 0x0, &DXArchive_VER5::DecodeArchive, &DXArchive_VER5::EncodeArchiveOneDirectory, std::vector<unsigned char>{ 0x4c, 0xd9, 0x2a, 0xb7, 0x28, 0x9b, 0xac, 0x07, 0x3e, 0x77, 0xec, 0x4c } },
	{ "Wolf RPG v2.20", 0x0, &DXArchive_VER6::DecodeArchive, &DXArchive_VER6::EncodeArchiveOneDirectory, std::vector<unsigned char>{ 0x38, 0x50, 0x40, 0x28, 0x72, 0x4f, 0x21, 0x70, 0x3b, 0x73, 0x35, 0x38 } },
	{ "Wolf RPG v2.225", 0x0, &DXArchive::DecodeArchive, &DXArchive::EncodeArchiveOneDirectoryWolf, "WLFRPrO!p(;s5((8P@((UFWlu$#5(=" },
	{ "Wolf RPG v3.00", 0x12C, &DXArchive::DecodeArchive, &DXArchive::EncodeArchiveOneDirectoryWolf, std::vector<unsigned char>{ 0x0F, 0x53, 0xE1, 0x3E, 0x8E, 0xB5, 0x41, 0x91, 0x52, 0x16, 0x55, 0xAE, 0x34, 0xC9, 0x8F, 0x79, 0x59, 0x2F, 0x59, 0x6B, 0x95, 0x19, 0x9B, 0x1B, 0x35, 0x9A, 0x2F, 0xDE, 0xC9, 0x7C, 0x12, 0x96, 0xC3, 0x14, 0xB5, 0x0F, 0x53, 0xE1, 0x3E, 0x8E, 0x00 } },
	{ "Wolf RPG v3.14", 0x13A, &DXArchive::DecodeArchive, &DXArchive::EncodeArchiveOneDirectoryWolf, std::vector<unsigned char>{ 0x31, 0xF9, 0x01, 0x36, 0xA3, 0xE3, 0x8D, 0x3C, 0x7B, 0xC3, 0x7D, 0x25, 0xAD, 0x63, 0x28, 0x19, 0x1B, 0xF7, 0x8E, 0x6C, 0xC4, 0xE5, 0xE2, 0x76, 0x82, 0xEA, 0x4F, 0xED, 0x61, 0xDA, 0xE0, 0x44, 0x5B, 0xB6, 0x46, 0x3B, 0x06, 0xD5, 0xCE, 0xB6, 0x78, 0x58, 0xD0, 0x7C, 0x82, 0x00 } },
	{ "Wolf RPG v3.31", 0x14B, &DXArchive::DecodeArchive, &DXArchive::EncodeArchiveOneDirectoryWolf, std::vector<unsigned char>{ 0xCA, 0x08, 0x4C, 0x5D, 0x17, 0x0D, 0xDA, 0xA1, 0xD7, 0x27, 0xC8, 0x41, 0x54, 0x38, 0x82, 0x32, 0x54, 0xB7, 0xF9, 0x46, 0x8E, 0x13, 0x6B, 0xCA, 0xD0, 0x5C, 0x95, 0x95, 0xE2, 0xDC, 0x03, 0x53, 0x60, 0x9B, 0x4A, 0x38, 0x17, 0xF3, 0x69, 0x59, 0xA4, 0xC7, 0x9A, 0x43, 0x63, 0xE6, 0x54, 0xAF, 0xDB, 0xBB, 0x43, 0x58, 0x00 } },
	{ "Wolf RPG v3.50", 0x15E, &DXArchive::DecodeArchive, &DXArchive::EncodeArchiveOneDirectoryWolf, std::vector<unsigned char>{ 0xD2, 0x84, 0xCE, 0x28, 0xCE, 0x88, 0x82, 0xE4, 0x2A, 0x18, 0x2E, 0x4C, 0x06, 0xB4, 0xEA, 0x84, 0x06, 0xB8, 0xC6, 0x88, 0x5A, 0xA0, 0x9E, 0x7C, 0x56, 0x40, 0xBA, 0x34, 0x52, 0xCC, 0xC6, 0x7C, 0x2E, 0x14, 0x12, 0x68, 0xFE, 0x5C, 0x76, 0x94, 0x86, 0x78, 0x8E, 0x4C, 0xBE, 0x88, 0x66, 0x9C, 0x1E, 0xE0, 0x8E, 0x6C, 0x00 } },
	{ "Wolf RPG ChaCha2 v1", 0x64, &DXArchive::DecodeArchive, &DXArchive::EncodeArchiveOneDirectoryWolf, std::vector<unsigned char>{ 0xC9, 0x82, 0xF8, 0xB4, 0x2C, 0x93, 0x9E, 0x83, 0x0E, 0xBC, 0xBC, 0x92, 0x68, 0x8D, 0x59, 0xA1, 0x4A, 0x9E, 0x7F, 0xB0, 0xAC, 0xAF, 0x1D, 0x8F, 0x8E, 0xB8, 0x3B, 0x9E, 0xE8, 0x89, 0xD9, 0xAD, 0xFF, 0xBC, 0x2D, 0xAB, 0x9D, 0x8B, 0x0F, 0xB4, 0xBB, 0x9A, 0x69, 0x85, 0x00 } }, // First 32 bytes of the key and the next 12 byte are the nonce, 0 terminator for the unused keygen to not crash
	{ "One Way Heroics", 0x0, &DXArchive::DecodeArchive, &DXArchive::EncodeArchiveOneDirectoryWolf, "nGui9('&1=@3#a" },
	{ "One Way Heroics Plus", 0x0, &DXArchive::DecodeArchive, &DXArchive::EncodeArchiveOneDirectoryWolf, "Ph=X3^]o2A(,1=@3#a" }
};

WolfDec::WolfDec(const std::wstring& progName, const uint32_t& mode, const bool& isSubProcess) :
	m_progName(progName),
	m_mode(mode),
	m_isSubProcess(isSubProcess),
	m_valid(true)
{
	loadConfig();
}

WolfDec::~WolfDec()
{
}

bool WolfDec::IsValidFile(const tString& filePath) const
{
	const tStrings specialFiles = GetSpecialFiles();
	return (std::find(specialFiles.begin(), specialFiles.end(), FS_PATH_TO_TSTRING(fs::path(filePath).filename())) == specialFiles.end());
}

bool WolfDec::IsAlreadyUnpacked(const tString& filePath) const
{
	const fs::path fp = fs::path(filePath);

	const tString directoryPath = fp.parent_path();
	const tString fileName      = fp.stem();
	const tString outDir        = directoryPath + TEXT("/") + fileName;

	if (!fs::exists(outDir)) return false;
	if (fs::is_empty(outDir)) return false;

	// Get all files in the directory
	std::vector<fs::path> files;
	for (const auto& entry : fs::directory_iterator(outDir))
		files.push_back(entry.path());

	if (files.size() == 1 && files.front().filename() == "decrypt_temp")
	{
		// Remove the temporary directory
		fs::remove(files.front());
		return false;
	}

	return true;
}

bool WolfDec::PackArchive(const tString& folderPath, const bool& override)
{
	const fs::path fp = fs::path(folderPath);

	if (!fs::exists(fp) || !fs::is_directory(fp))
	{
		ERROR_LOG << std::format(TEXT("Invalid directory: {}"), folderPath) << std::endl;
		if (m_isSubProcess)
			ExitProcess(1);
		else
			return false;
	}

	const tString directoryPath = fp.parent_path();
	const tString fileName      = fp.stem();
	// TODO: How to detect the other file extensions?
	const tString outputFile = directoryPath + TEXT("/") + fileName + TEXT(".wolf");

	// Check if the output file already exists
	if (!override && fs::exists(outputFile))
		return true;

	if (m_mode == -1)
		throw InvalidModeException();

	if (m_mode >= (DEFAULT_CRYPT_MODES.size() + m_additionalModes.size()))
	{
		ERROR_LOG << std::format(TEXT("Specified Mode: {} out of range"), m_mode) << std::endl;
		if (m_isSubProcess)
			ExitProcess(1);
		else
			return false;
	}

	const CryptMode curMode = (m_mode < DEFAULT_CRYPT_MODES.size() ? DEFAULT_CRYPT_MODES.at(m_mode) : m_additionalModes.at(m_mode - DEFAULT_CRYPT_MODES.size()));

	fs::current_path(directoryPath);

	if (curMode.encFunc == nullptr)
	{
		const std::wstring modeName = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(curMode.name);
		ERROR_LOG << std::format(TEXT("Encryption function not found for mode: {}"), modeName) << std::endl;
		if (m_isSubProcess)
			ExitProcess(1);
		else
			return false;
	}

	const bool failed = curMode.encFunc(outputFile.c_str(), folderPath.c_str(), true, curMode.key.data(), curMode.cryptVersion) < 0;

	if (failed)
		fs::remove(outputFile);

	if (m_isSubProcess)
		ExitProcess(failed);

	return !failed;
}

bool WolfDec::UnpackArchive(const tString& filePath, const bool& override)
{
	TCHAR pFullPath[MAX_PATH];
	const fs::path fp = fs::path(filePath);
	const tString cwd = fs::current_path();

	const tString directoryPath = fp.parent_path();
	const tString fileName      = fp.stem();

	ConvertFullPath__(filePath.c_str(), pFullPath);

	// Check if the basename of the file is in the ignore list
	if (!IsValidFile(filePath))
		return true;

	// Check if the file is already unpacked, i.e., if the directory exists and is not empty
	if (!override && IsAlreadyUnpacked(filePath))
		return true;

	if (m_mode == -1)
	{
		const uint16_t cryptVersion = getCryptVersion(filePath);

		if (cryptVersion == 0x0)
			return detectMode(filePath, override);
		// For Pro Games always return false and let UberWolfLib calculate the key
		else if (cryptVersion >= PRO_CRYPT_VERSION)
			return false;
		else if (cryptVersion == CC2_PRO_VERSION)
			return false;
		else if (!detectCrypt(filePath))
			return false;
	}

	if (m_mode >= (DEFAULT_CRYPT_MODES.size() + m_additionalModes.size()))
	{
		ERROR_LOG << std::format(TEXT("Specified Mode: {} out of range"), m_mode) << std::endl;
		if (m_isSubProcess)
			ExitProcess(1);
		else
			return false;
	}

	// Always run the unpacking in a subprocess
	if (!m_isSubProcess)
		return runProcess(filePath, m_mode, override);

	const CryptMode curMode = (m_mode < DEFAULT_CRYPT_MODES.size() ? DEFAULT_CRYPT_MODES.at(m_mode) : m_additionalModes.at(m_mode - DEFAULT_CRYPT_MODES.size()));

	fs::current_path(directoryPath);
	fs::create_directory(fileName);
	fs::current_path(fileName);

	const bool failed = curMode.decFunc(pFullPath, TEXT(""), curMode.key.data()) < 0;

	if (failed)
	{
		fs::current_path(directoryPath);
		fs::remove_all(fileName);
	}

	if (m_isSubProcess)
		ExitProcess(failed);

	fs::current_path(cwd);

	return !failed;
}

void WolfDec::AddAndSetKey(const std::string& name, const uint16_t& cryptVersion, const bool& useOldDxArc, const Key& key)
{
	AddKey(name, cryptVersion, useOldDxArc, key);
	m_mode = static_cast<uint32_t>(DEFAULT_CRYPT_MODES.size() + m_additionalModes.size() - 1);
}

void WolfDec::AddKey(const std::string& name, const uint16_t& cryptVersion, const bool& useOldDxArc, const Key& key)
{
	m_additionalModes.push_back({ name, cryptVersion, (useOldDxArc ? &DXArchive_VER6::DecodeArchive : &DXArchive::DecodeArchive), nullptr, key });
}

tStrings WolfDec::GetEncryptionsW()
{
	tStrings encryptions;
	for (const auto& mode : DEFAULT_CRYPT_MODES)
		encryptions.push_back(StringToWString(mode.name));

	return encryptions;
}

Strings WolfDec::GetEncryptions()
{
	Strings encryptions;
	for (const auto& mode : DEFAULT_CRYPT_MODES)
		encryptions.push_back(mode.name);

	return encryptions;
}

void WolfDec::removeOldConfig() const
{
	if (fs::exists(CONFIG_FILE_NAME))
		fs::remove(CONFIG_FILE_NAME);
}

void WolfDec::loadConfig()
{
	// Return if the config file does not exist or is empty
	if (!fs::exists(CONFIG_FILE_NAME) || fs::file_size(CONFIG_FILE_NAME) == 0)
		return;

	try
	{
		std::ifstream f(CONFIG_FILE_NAME);
		nlohmann::ordered_json data = nlohmann::ordered_json::parse(f);

		if (data.contains("keys"))
		{
			for (const auto& [name, value] : data["keys"].items())
			{
				if (value.contains("mode") && value.contains("key"))
				{
					std::string mode        = value["mode"];
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
						key                = std::vector<unsigned char>(keyStr.begin(), keyStr.end());
						key.push_back(0x00);
					}

					m_additionalModes.push_back({ name, 0x0, decFunc, nullptr, key });
				}
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
}

bool WolfDec::detectCrypt(const tString& filePath)
{
	// Check if the file contains a crypt version in the header
	const uint16_t cryptVersion = getCryptVersion(filePath);

	// If the crypt version is != 0, use the corresponding mode
	if (cryptVersion != 0)
	{
		for (uint32_t i = 0; i < DEFAULT_CRYPT_MODES.size(); i++)
		{
			if (DEFAULT_CRYPT_MODES.at(i).cryptVersion == cryptVersion)
			{
				m_mode = i;
				return true;
			}
		}

		for (uint32_t i = 0; i < m_additionalModes.size(); i++)
		{
			if (m_additionalModes.at(i).cryptVersion == cryptVersion)
			{
				m_mode = static_cast<uint32_t>(DEFAULT_CRYPT_MODES.size() + i);
				return true;
			}
		}
	}

	return false;
}

bool WolfDec::detectMode(const tString& filePath, const bool& override)
{
	bool success = false;

	if (m_mode == -1)
	{
		for (uint32_t i = 0; i < DEFAULT_CRYPT_MODES.size(); i++)
		{
			success = runProcess(filePath, i, override);
			if (success)
			{
				m_mode = i;
				return success;
			}
		}

		for (uint32_t i = 0; i < m_additionalModes.size(); i++)
		{
			success = runProcess(filePath, static_cast<uint32_t>(DEFAULT_CRYPT_MODES.size() + i), override);
			if (success)
			{
				m_mode = static_cast<uint32_t>(DEFAULT_CRYPT_MODES.size() + i);
				return success;
			}
		}
	}
	else
		success = runProcess(filePath, m_mode);

	return success;
}

bool WolfDec::runProcess(const tString& filePath, const uint32_t& mode, const bool& override) const
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	const std::wstring wstr = m_progName + L" -m " + std::to_wstring(mode) + L" \"" + std::wstring(filePath) + L"\"" + (override ? L" -o" : L"");

	if (!CreateProcess(NULL, const_cast<LPWSTR>(wstr.c_str()), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		ERROR_LOG << std::format(TEXT("CreateProcess() failed: {}"), GetLastError()) << std::endl;
		return false;
	}

	// Wait until child process exits.
	WaitForSingleObject(pi.hProcess, INFINITE);

	bool success = false;

	DWORD ec;
	if (FALSE == GetExitCodeProcess(pi.hProcess, &ec))
		ERROR_LOG << std::format(TEXT("GetExitCodeProcess() failed: {}"), GetLastError()) << std::endl;
	else
		success = (ec == 0);

	// Close process and thread handles.
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return success;
}

uint16_t WolfDec::getCryptVersion(const tString& filePath) const
{
	// Read the DARC_HEAD from the file
	std::ifstream f(filePath, std::ios::binary);
	if (!f.is_open())
	{
		ERROR_LOG << std::format(TEXT("Failed to open file: {}"), filePath) << std::endl;
		return 0;
	}

	DARC_HEAD header;
	f.read(reinterpret_cast<char*>(&header), sizeof(DARC_HEAD));
	f.close();

	if (header.Head != DXA_HEAD)
		return 0;

	return (header.Flags >> 16);
}
