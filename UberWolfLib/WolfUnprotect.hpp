/*
 *  File: WolfUnprotect.hpp
 *  Copyright (c) 2026 Sinflower
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

#include <iostream>
#include <map>

#include "Localizer.h"
#include "UberLog.h"
#include "Utils.h"

#include "WolfRPG/CommonEvents.hpp"
#include "WolfRPG/Database.hpp"

#include "WolfCrypt/WolfDataDecrypt.hpp"

namespace wolf::unprotect::v3_5
{

WolfFileType getWolfFileType(const std::filesystem::path &filePath)
{
	if (filePath.filename() == "Game.dat")
		return WolfFileType::GameDat;
	else if (filePath.filename() == "CommonEvent.dat")
		return WolfFileType::CommonEvent;
	else if (FilenameAnyOf(filePath, { "DataBase.dat", "CDatabase.dat", "SysDatabase.dat" }))
		return WolfFileType::DataBase;
	else if (filePath.filename() == "TileSetData.dat")
		return WolfFileType::TileSetData;
	else if (filePath.extension() == ".project")
		return WolfFileType::Project;
	else if (filePath.extension() == ".mps")
		return WolfFileType::Map;
	return WolfFileType::None;
}

const std::vector<std::string> PROTECTED_FILES = {
	"Game.dat",
	"CommonEvent.dat",
	"DataBase.dat",
	"SysDatabase.dat",
	"CDatabase.dat",
	"TileSetData.dat"
};

void gameDatUpdateSize(std::vector<uint8_t> &bytes, const uint32_t &oldSize)
{
	std::size_t offset = 10;                                     // Size of the header
	offset += *reinterpret_cast<uint32_t *>(&bytes[offset]) + 4; // Bytes
	offset += 4;                                                 // DWORD
	offset += *reinterpret_cast<uint32_t *>(&bytes[offset]) + 4; // Title
	offset += *reinterpret_cast<uint32_t *>(&bytes[offset]) + 4; // Number (0000-0000)
	offset += *reinterpret_cast<uint32_t *>(&bytes[offset]) + 4; // Decrypt Key
	offset += *reinterpret_cast<uint32_t *>(&bytes[offset]) + 4; // Font

	while (*reinterpret_cast<uint32_t *>(&bytes[offset]) != (oldSize - 1))
		offset += *reinterpret_cast<uint32_t *>(&bytes[offset]) + 4; // ?

	*reinterpret_cast<uint32_t *>(&bytes[offset]) = static_cast<uint32_t>(bytes.size()) - 1;
}

void unprotectProject(std::vector<uint8_t> &projData)
{
	// ¯\_(ツ)_/¯ So far it looks like this is how it is done
	srand(0);
	for (uint8_t &byte : projData)
		byte ^= static_cast<uint8_t>(rand());
}

void unprotectProFiles(const std::wstring &folder)
{
	// Create a backup folder and copy the original file
	const std::filesystem::path backupFolder = std::filesystem::path(folder) / L"backup";
	if (!std::filesystem::exists(backupFolder))
		std::filesystem::create_directory(backupFolder);

	for (const std::string &file : PROTECTED_FILES)
	{
		INFO_LOG << vFormat(LOCALIZE("remove_prot"), std::filesystem::path(file).wstring()) << std::flush;
		const std::filesystem::path filePath = std::filesystem::path(folder) / file;
		const WolfFileType datType           = getWolfFileType(filePath);

		if (!std::filesystem::exists(filePath))
		{
			INFO_LOG << LOCALIZE("failed_msg") << std::endl;
			ERROR_LOG << "File not found: " << filePath << std::endl;
			continue;
		}

		// Backup the original file
		backupFile(filePath, backupFolder);

		std::vector<uint8_t> buffer = file2Buffer(filePath);
		const uint32_t oldSize      = static_cast<uint32_t>(buffer.size());

		if (!wolf::crypt::data_decrypt::v3_5::decryptProV3Dat(buffer, datType))
		{
			INFO_LOG << LOCALIZE("failed_msg") << std::endl;
			continue;
		}

		if (datType == WolfFileType::GameDat)
			gameDatUpdateSize(buffer, oldSize);

		buffer2File(filePath, buffer);
		buffer.clear();

		if (datType == WolfFileType::CommonEvent)
		{
			try
			{
				CommonEvents comEv(filePath.wstring());

				if (!comEv.IsValid())
				{
					INFO_LOG << LOCALIZE("failed_msg") << std::endl;
					ERROR_LOG << "Failed to load CommonEvent.dat" << std::endl;
					continue;
				}

				// Remove an additional layer of protection
				comEv.FixPro35EventDescriptions();

				// Dump the fixed CommonEvent back into the folder
				comEv.Dump(folder);
			}
			catch (const std::exception &e)
			{
				INFO_LOG << LOCALIZE("failed_msg") << std::endl;
				ERROR_LOG << "Error processing CommonEvent.dat: " << e.what() << std::endl;
				continue;
			}
		}

		// There is no real way to tell if a project file has already been decrypted, therefore we use the decryption state of the corresponding dat file as an indicator (continue above)
		if (datType == WolfFileType::DataBase)
		{
			std::filesystem::path projPath = filePath;
			projPath.replace_extension(".project");

			backupFile(projPath, backupFolder);

			buffer = file2Buffer(projPath);
			unprotectProject(buffer);
			buffer2File(projPath, buffer);

			try
			{
				Database db(projPath.wstring(), filePath.wstring());

				if (!db.IsValid())
				{
					INFO_LOG << LOCALIZE("failed_msg") << std::endl;
					ERROR_LOG << "Failed to load Database.dat" << std::endl;
					continue;
				}

				// Remove an additional layer of protection
				db.FixPro35TypeDescriptions();

				// Dump the fixed Database back into the folder
				db.Dump(folder);
			}
			catch (const std::exception &e)
			{
				INFO_LOG << LOCALIZE("failed_msg") << std::endl;
				ERROR_LOG << "Error processing Database.dat: " << e.what() << std::endl;
				continue;
			}
		}

		INFO_LOG << LOCALIZE("done_msg") << std::endl;
	}
}

} // namespace wolf::unprotect::v3_5
