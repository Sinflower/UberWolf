/*
 *  File: Wolf35Unprotect.hpp
 *  Copyright (c) 2025 Sinflower
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

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <vector>

// This depends on WolfRPG for the WolfFileType enum
#include "WolfRPG/Types.h"
#include "WolfRPG/WolfRPGUtils.h"

#include "WolfSha512.hpp"

#include "Utils.h"
#include <DXLib/WolfNew.h>

namespace wolf::v3_5::unprotect
{

struct ProMagic
{
	std::string staticSalt;
	std::vector<uint8_t> magicBytes;
};

static const std::map<WolfFileType, ProMagic> PRO_MAGIC = {
	{ WolfFileType::GameDat, { "basicD1", { 0x00, 0x57, 0x00, 0x00, 0x4F, 0x4C, 0x00, 0x46, 0x4D, 0x55 } } },
	{ WolfFileType::CommonEvent, { "Commo2", { 0x00, 0x57, 0x00, 0x00, 0x4F, 0x4C, 0x55, 0x46, 0x43, 0x00 } } },
	{ WolfFileType::DataBase, { "DBase4", { 0x00, 0x57, 0x00, 0x00, 0x4F, 0x4C, 0x55, 0x46, 0x4D, 0x00 } } },
	{ WolfFileType::TileSetData, { "TilesetA", { 0x00, 0x57, 0x00, 0x00, 0x4F, 0x4C, 0x55, 0x46, 0x4D, 0x00 } } },
	{ WolfFileType::None, { "", {} } }
};

inline WolfFileType getWolfFileType(const std::filesystem::path &filePath)
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

inline void decryptProV3P1(std::vector<uint8_t> &data, const std::array<uint8_t, 3> seedIdx)
{
	const uint32_t seed = (0xB << 24) | (data[seedIdx[0]] << 16) | (data[seedIdx[1]] << 8) | data[seedIdx[2]];
	int32_t rn          = xorshift32(seed);

	for (uint32_t i = 0xA; i < data.size(); i++)
	{
		int32_t v1 = (((rn << 0xF) ^ rn) >> 0x15) ^ (rn << 0xF) ^ rn;
		rn         = (v1 << 0x9) ^ v1;
		data[i] ^= rn % 0xF9;
	}
}

inline bool decryptProV3Dat(std::vector<uint8_t> &buffer, const WolfFileType &datType)
{
	constexpr uint32_t KEY_START_OFFSET = 12;
	constexpr uint32_t IV_START_OFFSET  = 73;
	constexpr uint32_t AES_DATA_OFFSET  = 20;
	constexpr uint32_t PRO_SPECIAL_SIZE = 143; // 15 byte header + 128 byte hash

	if (buffer.empty() || buffer.size() < PRO_SPECIAL_SIZE)
	{
		std::cerr << "Buffer is empty or too small" << std::endl;
		return false;
	}

	if (buffer[1] != 0x50 || buffer[5] < 0x57)
	{
		std::cout << "File is not protected or not a ProV3 file, skipping decryption" << std::endl;
		return false;
	}

	std::array<uint8_t, 3> seedIdx = { 0, 3, 9 }; // Default idx for everything except Game.dat

	if (datType == WolfFileType::GameDat)
		seedIdx = { 0, 8, 6 };

	decryptProV3P1(buffer, seedIdx);

	srand(buffer[12]);
	std::size_t aesSize = buffer.size() - AES_DATA_OFFSET;
	// ¯\_(ツ)_/¯ that's what to code says (probably) and it works ¯\_(ツ)_/¯
	if (aesSize >= rand() % 126 + 200)
		aesSize = rand() % 126 + 200;

	uint64_t nBuffer = 0;

	const ProMagic &proMagic = PRO_MAGIC.at(datType);

	sha512::s512DynSalt dynSalt = sha512::calcDynSalt(buffer);
	sha512::s512Pwd saltedPwd   = sha512::saltPassword("", dynSalt, proMagic.staticSalt);
	sha512::s512Input sInput    = sha512::preprocess(saltedPwd, nBuffer);
	sha512::s512Hash hashData   = sha512::process(sInput, nBuffer);
	std::string hashString      = sha512::digest(hashData);

	AesKey aesKey;
	AesIV aesIv;
	AesRoundKey roundKey;

	std::copy(hashString.begin() + KEY_START_OFFSET, hashString.begin() + KEY_START_OFFSET + AES_KEY_SIZE, aesKey.begin());
	std::copy(hashString.begin() + IV_START_OFFSET, hashString.begin() + IV_START_OFFSET + AES_IV_SIZE, aesIv.begin());

	keyExpansion(roundKey.data(), aesKey.data());
	std::copy(aesIv.begin(), aesIv.end(), roundKey.begin() + AES_KEY_EXP_SIZE);

	aesCtrXCrypt(buffer.data() + AES_DATA_OFFSET, roundKey.data(), aesSize);

	buffer.erase(buffer.begin(), buffer.begin() + PRO_SPECIAL_SIZE);
	buffer.insert(buffer.begin(), proMagic.magicBytes.begin(), proMagic.magicBytes.end());

	return true;
}

inline void gameDatUpdateSize(std::vector<uint8_t> &bytes, const uint32_t &oldSize)
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

static const std::vector<std::string> PROTECTED_FILES = {
	"Game.dat",
	"CommonEvent.dat",
	"DataBase.dat",
	"SysDatabase.dat",
	"CDatabase.dat",
	"TileSetData.dat"
};

inline void unprotectProject(std::vector<uint8_t> &projData)
{
	// ¯\_(ツ)_/¯ So far it looks like this is how it is done
	srand(0);
	for (uint8_t &byte : projData)
		byte ^= static_cast<uint8_t>(rand());
}

inline void unprotectProFiles(const std::wstring &folder)
{
	// Create a backup folder and copy the original file
	const std::filesystem::path backupFolder = std::filesystem::path(folder) / "backup";
	if (!std::filesystem::exists(backupFolder))
		std::filesystem::create_directory(backupFolder);

	for (const std::string &file : PROTECTED_FILES)
	{
		const std::filesystem::path filePath = std::filesystem::path(folder) / file;
		const WolfFileType datType           = getWolfFileType(filePath.string());

		if (!std::filesystem::exists(filePath))
		{
			std::cerr << "File not found: " << filePath << std::endl;
			continue;
		}

		// Backup the original file
		backupFile(filePath, backupFolder);

		std::vector<uint8_t> buffer = file2Buffer(filePath);
		const uint32_t oldSize      = static_cast<uint32_t>(buffer.size());

		if (!decryptProV3Dat(buffer, datType))
			continue;

		if (datType == WolfFileType::GameDat)
			gameDatUpdateSize(buffer, oldSize);

		buffer2File(filePath, buffer);

		// There is no real way to tell if a project file has already been decrypted, therefore we use the decryption state of the corresponding dat file as an indicator (continue above)
		if (datType == WolfFileType::DataBase)
		{
			std::filesystem::path projPath = filePath;
			projPath.replace_extension(".project");

			backupFile(projPath, backupFolder);

			buffer = file2Buffer(projPath);
			unprotectProject(buffer);
			buffer2File(projPath, buffer);
		}
	}
}

} // namespace wolf::v3_5::unprotect
