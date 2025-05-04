/*
 *  File: UtilsDetail.hpp
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
#include <exception>
#include <filesystem>
#include <fstream>
#include <vector>

namespace wolfx::detail::utils
{
template<std::size_t N>
inline std::array<uint8_t, N> extractBytes(const uint32_t &value)
{
	std::array<uint8_t, N> result = { 0 };

	for (std::size_t i = 0; i < N; i++)
		result[i] = static_cast<uint8_t>(value >> (8 * ((N - 1) - i)));

	return result;
}

template<std::size_t N, class T = std::vector<uint8_t>>
inline uint32_t combineBytes(const T &bytes, const std::size_t &start = 0)
{
	uint32_t result = 0;
	for (std::size_t i = 0; i < N; i++)
	{
		result <<= 8;
		result |= bytes[start + i];
	}
	return result;
}

inline std::vector<uint8_t> file2Buffer(const std::filesystem::path &filePath)
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

inline void buffer2File(const std::filesystem::path &filePath, const std::vector<uint8_t> &buffer, const std::size_t &offset = 0)
{
	std::ofstream outFile(filePath, std::ios::binary);
	if (!outFile)
		throw std::runtime_error("Failed to open output file: " + filePath.string());
	outFile.write(reinterpret_cast<const char *>(buffer.data()) + offset, buffer.size() - offset);
	outFile.close();
}

inline WolfXFiles collectWolfXFiles(const std::filesystem::path &baseFolder)
{
	WolfXFiles wolfxFiles;

	for (const auto &entry : std::filesystem::recursive_directory_iterator(baseFolder))
	{
		if (entry.is_regular_file() && entry.path().extension() == ".wolfx")
		{
			const std::filesystem::path path = entry.path();
			wolfxFiles.push_back({ path.wstring(), std::filesystem::file_size(path) });
		}
	}

	// Sort the wolfxFiles by file size
	std::sort(wolfxFiles.begin(), wolfxFiles.end(), [](const WolfXFile &a, const WolfXFile &b) { return a.fileSize < b.fileSize; });

	return wolfxFiles;
}

} // namespace wolfx::detail::utils