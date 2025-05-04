/*
 *  File: ValidateDetail.hpp
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
#include <iostream>
#include <string>
#include <vector>

#include "../Types.hpp"

namespace wolfx::detail::validate
{
inline bool validateChecksum(const uint8_t *pData, const std::size_t &dataSize, const std::array<uint8_t, 5> &realChecksum, const bool &verbose = false)
{
	std::array<uint8_t, 5> checksum;
	const std::size_t finalIdx = dataSize - 1;

	for (std::size_t i = 0; i < 5; i++)
		checksum[i] = pData[static_cast<std::size_t>(finalIdx * 0.25 * i)]; // Yes, this is really how it is done -- at least I think it is ¯\_(ツ)_/¯

	if (checksum == realChecksum)
	{
		if (verbose)
			std::cout << "Checksum match" << std::endl;
		return true;
	}

	if (verbose)
		std::cerr << "Checksum mismatch" << std::endl;
	return false;
}

inline bool validateChecksum(const std::vector<uint8_t> &data, const std::array<uint8_t, 5> &realChecksum, const bool &verbose = false)
{
	if (data.empty())
	{
		std::cerr << "Data is empty" << std::endl;
		return false;
	}

	return validateChecksum(data.data(), data.size(), realChecksum, verbose);
}
} // namespace wolfx::detail::validate