/*
 *  File: Crack.hpp
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
#include <iostream>
#include <vector>

#include "DataManip.hpp"
#include "Types.hpp"
#include "Utils.hpp"

#include "detail/CrackDetail.hpp"

namespace wolfx
{
inline bool decryptFile(const std::wstring &filename, const std::string &decryptKey, const std::string &magicStr, const uint32_t &magicInt)
{
	dataManip::initXorBufferBlobFunc();
	const WolfXData encData = utils::file2Buffer(filename);

	WolfXData decData;
	uint32_t dataOffset = 0;

	decData.resize(encData.size());
	std::copy(encData.begin(), encData.begin() + 10, decData.begin());

	if (detail::crack::decryptFull(encData, decryptKey, magicStr, magicInt, decData, dataOffset))
	{
		std::cout << "Decryption successful!" << std::endl;
		std::wstring outputFilename = filename.substr(0, filename.find_last_of('.'));
		utils::buffer2File(outputFilename, decData, dataOffset);
		return true;
	}
	else
	{
		std::cerr << "Decryption failed!" << std::endl;
		return false;
	}
}

inline bool crackWolfX(const WolfXFile &file, const WolfXDecryptCollection &decryptCollection, DecryptResult &decryptResult)
{
	return detail::crack::crackWolfX(file, decryptCollection, decryptResult);
}

inline bool crackWolfXFiles(const WolfXFiles &wolfXFiles, const WolfXDecryptCollection &decryptCollection)
{
	return detail::crack::crackWolfXFiles(wolfXFiles, decryptCollection);
}

} // namespace wolfx