/*
 *  File: CrackDetail.hpp
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

#include <array>
#include <bit>
#include <cstdint>
#include <string>
#include <vector>

#include "../Types.hpp"

#include "DataManipDetail.hpp"
#include "GeneratorDetail.hpp"
#include "UtilsDetail.hpp"
#include "ValidateDetail.hpp"

namespace wolfx::detail::crack
{
inline bool tryDecryptP3(DecryptBlob decryptBlob, const DecryptParams &params, DecryptResult &decryptResult)
{
	std::array<uint8_t, 3> modVal = utils::extractBytes<3>(params.intIndex);
	const uint32_t intHash        = (params.magicInt << 13) ^ (73244475 * params.magicInt);
	std::array<uint8_t, 4> intMod = utils::extractBytes<4>(intHash);

	// --- Apply decryption key transformations ---
	for (size_t i = 0; i < decryptBlob.size(); i++)
	{
		uint8_t magicChar = params.magicStr.empty() ? 0 : params.magicStr[i % params.magicStr.length()];
		decryptBlob[i] ^= params.xorBytes[i % 2] ^ magicChar ^ intMod[i & 3] ^ modVal[i % 3];
	}

	dataManip::xorBufferBlob(params.encData, decryptBlob, decryptResult.decData);

	// --- Validate the decrypted data ---
	std::array<uint8_t, 5> checksum = {};

	std::memcpy(checksum.data(), decryptResult.decData.data() + 15, 5);

	return validate::validateChecksum(decryptResult.decData.data() + params.dataOffset, decryptResult.decData.size() - params.dataOffset, checksum);
}

inline bool tryDecryptP2(const DecryptBlob &decryptBlob, DecryptParams &params, const WolfXDecryptCollection &wolfXMagic, DecryptResult &decryptResult)
{
	uint32_t strHash = generator::fnv1(params.magicStr);
	params.intIndex  = ((strHash & 0xFFFF0000) >> 8) ^ (strHash & 0xFFFF) ^ utils::combineBytes<3>(decryptResult.decData, 12);

	if (decryptResult.success)
	{
		params.magicInt = decryptResult.magicInt;
		return tryDecryptP3(decryptBlob, params, decryptResult);
	}

	if (params.intIndex < 1000000)
	{
		if (wolfXMagic.intValues.count(params.intIndex) == 0)
			return false;

		for (const auto &intVal : wolfXMagic.intValues.at(params.intIndex))
		{
			params.magicInt = intVal;
			if (tryDecryptP3(decryptBlob, params, decryptResult))
			{
				decryptResult.magicInt = params.magicInt;
				decryptResult.success  = true;
				return true;
			}
		}
		return false;
	}

	return tryDecryptP3(decryptBlob, params, decryptResult);
}

inline bool tryDecryptP1(const WolfXData &encData, const WolfXKeyData &decryptKey, const WolfXDecryptCollection &wolfXMagic, DecryptResult &decryptResult)
{
	const StaticBlob staticBlob = generator::generateWolfxStaticBlob(decryptKey);

	const uint32_t dataOffset = 512 + staticBlob[0] + staticBlob[1];

	if (dataOffset >= encData.size())
		return false;

	decryptResult.dataOffset = dataOffset;

	uint32_t headerInt = utils::combineBytes<4>(encData, 5);

	uint32_t fnvHash = generator::fnv1(staticBlob);
	uint32_t seed    = fnvHash ^ headerInt;

	DecryptBlob decryptBlob = generator::generateWolfxDecryptBlob(seed, staticBlob, encData.size());

	for (uint32_t i = 0; i < 5; i++)
		decryptResult.decData[10 + i] = encData[10 + i] ^ decryptBlob[i];

	const std::vector<uint8_t> xorBytes = { decryptResult.decData[10], decryptResult.decData[11] };

	const uint16_t magicStrIndex = utils::combineBytes<2>(xorBytes);

	if (decryptResult.success)
	{
		DecryptParams params = { encData, decryptResult.magicStr, xorBytes, dataOffset };
		return tryDecryptP2(decryptBlob, params, wolfXMagic, decryptResult);
	}

	if (magicStrIndex < 10000)
	{
		if (wolfXMagic.stringValues.count(magicStrIndex) == 0)
			return false;

		for (const auto &strVal : wolfXMagic.stringValues.at(magicStrIndex))
		{
			DecryptParams params = { encData, strVal, xorBytes, dataOffset };
			if (tryDecryptP2(decryptBlob, params, wolfXMagic, decryptResult))
			{
				decryptResult.magicStr = params.magicStr;
				return true;
			}
		}

		return false;
	}

	DecryptParams params = { encData, "", xorBytes, dataOffset };
	return tryDecryptP2(decryptBlob, params, wolfXMagic, decryptResult);
}

inline bool crackWolfX(const WolfXFile &file, const WolfXDecryptCollection &decryptCollection, DecryptResult &decryptResult)
{
	static const std::array<uint8_t, 5> WOLFX_MAGIC = { 0x57, 0x4F, 0x4C, 0x46, 0x58 }; // "WOLFX"

	dataManip::initXorBufferBlobFunc();

	WolfXData encData = utils::file2Buffer(file.filePath);

	if (encData.size() < 15 || std::memcmp(encData.data(), WOLFX_MAGIC.data(), 5) != 0)
	{
		std::cerr << "Invalid WOLFX file" << std::endl;
		return false;
	}

	// decryptResult.success = false;
	decryptResult.decData = WolfXData(encData.size());
	// Copy the first 10 bytes of the encrypted data to the decrypted data
	std::copy(encData.begin(), encData.begin() + 10, decryptResult.decData.begin());

	if (decryptResult.success)
	{
		if (!detail::crack::tryDecryptP1(encData, decryptResult.decryptKey.keyData, decryptCollection, decryptResult))
			return false;
	}

	for (const auto &decryptInfo : decryptCollection.decryptKeys)
	{
		if (detail::crack::tryDecryptP1(encData, decryptInfo.keyData, decryptCollection, decryptResult))
		{
			decryptResult.decryptKey = decryptInfo;
			break;
		}
	}

	if (!decryptResult.success)
	{
		std::cerr << "Failed to decrypt the file" << std::endl;
		return false;
	}

	// --- Write output file ---
	std::wstring outputFilename = file.filePath.substr(0, file.filePath.find_last_of('.'));

	utils::buffer2File(outputFilename, decryptResult.decData, decryptResult.dataOffset);

	return true;
}

inline bool crackWolfXFiles(const WolfXFiles &wolfXFiles, const WolfXDecryptCollection &decryptCollection, const uint32_t retries = 0)
{
	if (retries >= 5)
	{
		std::cerr << "Max retries reached, aborting" << std::endl;
		return false;
	}

	DecryptResult decryptResult;

	WolfXFiles retryFiles;

	for (const auto &file : wolfXFiles)
	{
		if (!crackWolfX(file, decryptCollection, decryptResult))
			retryFiles.push_back(file);
	}

	if (!retryFiles.empty())
	{
		std::cout << "Retrying " << retryFiles.size() << " files ..." << std::endl;
		return crackWolfXFiles(retryFiles, decryptCollection, retries + 1);
	}

	return true;
}

inline bool decryptFull(const WolfXData &encData, const WolfXKeyData &decryptKey, const std::string &magicStr, const uint32_t &magicInt, WolfXData &decData, uint32_t &dataOffset)
{
	StaticBlob staticBlob = generator::generateWolfxStaticBlob(decryptKey);

	uint32_t headerInt = utils::combineBytes<4>(encData, 5);

	uint32_t fnvHash = generator::fnv1(staticBlob);
	uint32_t seed    = fnvHash ^ headerInt;

	DecryptBlob decryptBlob = generator::generateWolfxDecryptBlob(seed, staticBlob, encData.size());

	for (uint32_t i = 0; i < 5; i++)
		decData[10 + i] = encData[10 + i] ^ decryptBlob[i];

	std::vector<uint8_t> xorBytes = { decData[10], decData[11] };

	// --- Magic string + int-based transformation ---

	uint16_t magicStrIndex = utils::combineBytes<2>(xorBytes);

	uint32_t strHash  = generator::fnv1(magicStr);
	uint32_t intIndex = ((strHash & 0xFFFF0000) >> 8) ^ (strHash & 0xFFFF) ^ utils::combineBytes<3>(decData, 12);

	std::array<uint8_t, 4> intMod = { 0 };
	if (intIndex < 1000000)
	{
		uint32_t intHash = (magicInt << 13) ^ (73244475 * magicInt);
		intMod           = utils::extractBytes<4>(intHash);
	}

	std::array<uint8_t, 3> modVal = utils::extractBytes<3>(intIndex);

	// --- Apply decryption key transformations ---
	for (size_t i = 0; i < decryptBlob.size(); i++)
	{
		uint8_t magicChar = magicStr.empty() ? 0 : magicStr[i % magicStr.length()];
		decryptBlob[i] ^= xorBytes[i % 2] ^ magicChar ^ intMod[i & 3] ^ modVal[i % 3];
	}

	dataManip::xorBufferBlob(encData, decryptBlob, decData);

	// --- Extract final decrypted data ---
	dataOffset = 512 + staticBlob[0] + staticBlob[1];

	std::array<uint8_t, 5> checksum = {};
	std::memcpy(checksum.data(), decData.data() + 15, 5);

	return validate::validateChecksum(decData.data() + dataOffset, decData.size() - dataOffset, checksum);
}

inline bool decryptFull(const WolfXData &encData, const std::string &decryptKey, const std::string &magicStr, const uint32_t &magicInt, WolfXData &decData, uint32_t &dataOffset)
{
	WolfXKeyData decryptKeyData(decryptKey.begin(), decryptKey.end());
	return decryptFull(encData, decryptKeyData, magicStr, magicInt, decData, dataOffset);
}

inline bool benchmarkDecryptFull(const WolfXData &encData, const WolfXKeyData &decryptKey, const std::string &magicStr, const uint32_t &magicInt, WolfXData &decData)
{
	uint32_t dataOffset = 0;
	return decryptFull(encData, decryptKey, magicStr, magicInt, decData, dataOffset);
}

} // namespace wolfx::detail::crack