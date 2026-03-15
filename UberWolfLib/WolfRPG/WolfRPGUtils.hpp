/*
 *  File: WolfRPGUtils.hpp
 *  Copyright (c) 2024 Sinflower
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

#include "Types.hpp"
#include "WolfRPGException.hpp"

#include <filesystem>
#include <format>
#include <iomanip>
#include <iostream>
#include <regex>
#include <source_location>
#include <sstream>
#include <string>

inline std::string BuildErrorTag(const std::source_location& location)
{
	std::string function = location.function_name();
	// Only keep the file name
	const std::string file = std::filesystem::path(location.file_name()).filename().string();

	// Remove the parameters from the function name
	std::size_t pos = function.find('(');
	if (pos != std::string::npos)
		function = function.substr(0, pos);

	// Remove the return type from the function name
	pos = function.find_last_of(' ');
	if (pos != std::string::npos)
		function = function.substr(pos + 1);

	return std::format("[{}:{} - {}()] ", file, location.line(), function);
}

inline std::wstring BuildErrorTagW(const std::source_location& location)
{
	const std::string tag = BuildErrorTag(location);
	return std::wstring(tag.begin(), tag.end());
}

inline std::string BuildJsonError(const std::string& key, const std::string& obj)
{
	return std::format("Key '{}' for object '{}' not found in patch", key, obj);
}

#define ERROR_TAG  BuildErrorTag(std::source_location::current())
#define ERROR_TAGW BuildErrorTagW(std::source_location::current())

#define CHECK_JSON_KEY(JSON, KEY, OBJ)                                                         \
	do                                                                                         \
		if (!JSON.contains(KEY)) throw WolfRPGException(ERROR_TAG + BuildJsonError(KEY, OBJ)); \
	while (0)

#define VERIFY_MAGIC(CODER, MAGIC) \
	if (!CODER.Verify(MAGIC)) throw WolfRPGException(ERROR_TAG + "MAGIC invalid");

namespace wolfRPGUtils
{
static bool g_skipBackup = false;
}

template<typename T>
inline std::string Dec2Hex(T i)
{
	std::stringstream stream;
	stream << "0x" << std::setfill('0') << std::setw(sizeof(T) * 2);
	// To counter act problems with 1-byte types
	stream << std::hex << (sizeof(T) == 1 ? static_cast<unsigned short>(i) : i);
	return stream.str();
}

template<typename T>
inline std::wstring Dec2HexW(T i)
{
	std::wstringstream stream;
	stream << L"0x" << std::setfill(wchar_t('0')) << std::setw(sizeof(T) * 2);
	// To counter act problems with 1-byte types
	stream << std::hex << (sizeof(T) == 1 ? static_cast<unsigned short>(i) : i);
	return stream.str();
}

inline const std::filesystem::path GetFileName(const std::filesystem::path& file)
{
	return file.filename();
}

inline const std::filesystem::path GetFileNameNoExt(const std::filesystem::path& file)
{
	return file.stem();
}

inline void CreateBackup(const std::filesystem::path& filePath)
{
	// If the skip backup flag is set, do not create a backup
	if (wolfRPGUtils::g_skipBackup) return;

	// If the file does not exist, do not create a backup
	if (!std::filesystem::exists(filePath)) return;

	std::filesystem::path bakPath = filePath;
	bakPath += ".bak";

	// If the backup file already exists, do not create a new backup
	if (std::filesystem::exists(bakPath)) return;

	// Create a backup of the file
	std::filesystem::copy_file(filePath, bakPath);
}

inline tString StrReplaceAll(tString str, const tString& from, const tString& to)
{
	size_t startPos = 0;
	if (from.empty() || str.empty()) return str;

	while ((startPos = str.find(from, startPos)) != std::string::npos)
	{
		str.replace(startPos, from.length(), to);
		startPos += to.length();
	}

	return str;
}

// Strip all leading / trailing whitespace, including fullwidth spaces
inline tString FullStrip(tString str)
{
	StrReplaceAll(str, L" ", L"");
	str = std::regex_replace(str, std::wregex(L"^\\u3000*"), L"");
	str = std::regex_replace(str, std::wregex(L"\\u3000*$"), L"");

	return str;
}

inline tString EscapePath(tString path)
{
	path = FullStrip(path);
	path = std::regex_replace(path, std::wregex(L"[\\/\\\\:\\*\\?\\\"<>\\|]"), L"_");

	return path;
}

inline bool FilenameAnyOf(const std::filesystem::path& path, const std::vector<std::string>& filenames)
{
	for (const auto& fN : filenames)
	{
		if (path.filename() == fN)
			return true;
	}
	return false;
}

inline void CheckAndCreateDir(const std::filesystem::path& path)
{
	if (!std::filesystem::exists(path))
	{
		if (!std::filesystem::create_directories(path))
		{
			if (!std::filesystem::exists(path))
				throw WolfRPGException(ERROR_TAGW + L"Failed to create directory: " + path.wstring());
		}
	}
}

inline std::filesystem::path g_activeFile = L"";