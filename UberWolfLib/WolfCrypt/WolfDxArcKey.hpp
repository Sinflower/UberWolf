/*
 *  File: WolfDxArcKey.hpp
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

#include <cstdint>
#include <vector>

#include "WolfAes.hpp"
#include "WolfCrypt.hpp"
#include "WolfCryptTypes.hpp"
#include "WolfRng.hpp"

namespace wolf::crypt::dxarckey
{
namespace v1
{

}

namespace v2
{

inline void initCrypt(CryptData &cd)
{
	const uint32_t HEADER_SIZE = 31;

	cd.dataSize = static_cast<uint32_t>(cd.gameDatBytes.size()) - HEADER_SIZE;

	const uint32_t sizeDiv = cd.dataSize / 3;

	uint32_t val1 = sizeDiv + 71 + (sizeDiv >> 31);
	uint32_t val2 = cd.dataSize ^ 0x70;
	uint32_t val3 = cd.dataSize % 1200 + 152;
	uint32_t val4 = cd.dataSize + 2 * cd.dataSize + 85;

	cd.keyBytes[0] = val4 ^ val1;
	cd.keyBytes[1] = val3 + val2;
	cd.keyBytes[2] = val2 - val4;
	cd.keyBytes[3] = val2 * val4;

	cd.seedBytes[0] = val1 + cd.gameDatBytes[3];
	cd.seedBytes[1] = val3 + cd.gameDatBytes[7];
	cd.seedBytes[2] = val2 + cd.gameDatBytes[5];
	cd.seedBytes[3] = val4 + cd.gameDatBytes[6];

	cd.seed1 = val1;
	cd.seed2 = val3;
}

inline CryptData decryptGameDat(const std::vector<uint8_t> &gameDataBytes)
{
	CryptData cd;
	rng::RngData rd;

	cd.gameDatBytes = gameDataBytes;

	initCrypt(cd);

	rng::runRngChain(rd, cd.seed1, cd.seed2);

	aes::AesKey aesKey;
	aes::AesIV aesIv;

	aesKeyGen(cd, rd, aesKey, aesIv);

	aes::AesRoundKey roundKey;

	aes::keyExpansion(roundKey.data(), aesKey.data());
	std::copy(aesIv.begin(), aesIv.end(), roundKey.begin() + aes::KEY_EXP_SIZE);

	aes::aesCtrXCrypt(cd.gameDatBytes.data() + 30, roundKey.data(), cd.dataSize);

	return cd;
}

inline std::vector<uint8_t> calcKey(const std::vector<uint8_t> &gameDataBytes)
{
	std::vector<uint8_t> key;
	CryptData cd = decryptGameDat(gameDataBytes);

	int32_t k       = cd.gameDatBytes[4] + ((static_cast<uint16_t>(cd.gameDatBytes[3]) * cd.gameDatBytes[6]) & 0x3FF);
	uint32_t keyLen = cd.gameDatBytes[19];

	for (;; k++)
	{
		int32_t v1 = cd.dataSize;
		int32_t v2 = k;
		int32_t v3 = 0;

		if (k)
		{
			do
			{
				v3 = v1 % v2;
				v1 = v2;
				v2 = v3;
			} while (v3);
		}

		if (v1 <= 1)
			break;
	}

	for (uint32_t i = 0; i < keyLen; i++)
		key.push_back(cd.gameDatBytes.at((i * k) % cd.dataSize + 30 + cd.gameDatBytes[7]));

	key.push_back(0x0);

	for (const uint8_t &s : cd.keyBytes)
		key.push_back(s);

	return key;
}
} // namespace v2
} // namespace wolf::crypt::dxarckey