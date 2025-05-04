/*
 *  File: Types.hpp
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
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace wolfx
{
namespace types::detail
{
constexpr std::size_t DECRYPT_BLOB_SIZE = 256;
constexpr std::size_t STATIC_BLOB_SIZE  = 64;
} // namespace types::detail

using WolfXData   = std::vector<uint8_t>;
using DecryptBlob = std::array<uint8_t, types::detail::DECRYPT_BLOB_SIZE>;
using StaticBlob  = std::array<uint8_t, types::detail::STATIC_BLOB_SIZE>;

using WolfXKeyData = std::vector<uint8_t>;
using StringSets   = std::map<uint32_t, std::set<std::string>>;
using IntegerSets  = std::map<uint32_t, std::set<uint32_t>>;

struct WolfXDecryptKey
{
	WolfXDecryptKey(const std::string &folder = "", const std::string &key = "") :
		folder(folder),
		key(key),
		keyData(key.begin(), key.end())
	{}

	std::string folder;
	std::string key;
	WolfXKeyData keyData;

	// Define less-than operator for sorting
	bool operator<(const WolfXDecryptKey &other) const
	{
		return std::tie(folder, key) < std::tie(other.folder, other.key);
	}

	// Define equality operator for comparison
	bool operator==(const WolfXDecryptKey &other) const
	{
		return folder == other.folder && key == other.key;
	}
};

using WolfXDecryptKeys = std::vector<WolfXDecryptKey>;

struct WolfXDecryptCollection
{
	WolfXDecryptKeys decryptKeys;
	StringSets stringValues;
	IntegerSets intValues;

	void clear()
	{
		decryptKeys.clear();
		stringValues.clear();
		intValues.clear();
	}
};

struct WolfXFile
{
	std::wstring filePath;
	std::size_t fileSize;
};

using WolfXFiles = std::vector<WolfXFile>;

struct DecryptParams
{
	const WolfXData &encData;
	const std::string &magicStr;
	const std::vector<uint8_t> &xorBytes;
	uint32_t dataOffset;
	uint32_t intIndex = 0;
	uint32_t magicInt = 0;
};

struct DecryptResult
{
	WolfXData decData;
	WolfXDecryptKey decryptKey = {};

	bool success = false;

	uint32_t dataOffset  = 0;
	std::string magicStr = "";
	uint32_t magicInt    = 0;
};

} // namespace wolfx