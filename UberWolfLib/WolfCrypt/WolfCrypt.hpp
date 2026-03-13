/*
 *  File: WolfCrypt.hpp
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

#include <array>
#include <cstdint>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "WolfAes.hpp"
#include "WolfChaCha20.hpp"
#include "WolfCryptTypes.hpp"
#include "WolfRng.hpp"

namespace wolf::crypt
{

inline void wolfCrypt(const uint8_t *pKey, uint8_t *pData, const int64_t &start, const int64_t &end, const bool &updateDataPos, const uint16_t &cryptVersion)
{
	if (updateDataPos)
		pData += start;

	const uint64_t length = end - start;

	uint32_t v1Cnt = start % 256;
	uint32_t v2Cnt = start / 256 % 256;
	int32_t v3Cnt  = start / 0x10000 % 256;

	if (utils::isV35(cryptVersion))
	{
		uint8_t moddedKey[512];
		for (uint32_t i = 0; i < 512; i++)
			moddedKey[i] = pKey[i % 256] ^ (7 * i);

		for (uint64_t i = 0; i < length; i++)
		{
			pData[i] ^= moddedKey[v1Cnt++] ^ moddedKey[v2Cnt + 256];

			if (v1Cnt == 256)
			{
				v1Cnt = 0;
				v2Cnt = (v2Cnt + 1) % 256;
			}
		}
	}
	else
	{
		for (uint64_t i = 0; i < length; i++)
		{
			pData[i] ^= pKey[v1Cnt++] ^ pKey[v2Cnt + 256] ^ pKey[v3Cnt + 512];

			if (v1Cnt == 256)
			{
				v1Cnt = 0;
				v2Cnt++;

				if (v2Cnt == 256)
				{
					v2Cnt = 0;
					v3Cnt = (v3Cnt + 1) % 256;
				}
			}
		}
	}
}

inline void calcSalt(const char *pStr, uint8_t *pSalt)
{
	if (!pSalt)
		return;

	uint32_t len = static_cast<uint32_t>(strlen(pStr));

	for (uint32_t i = 0; i < 128; i++)
		pSalt[i] = (i / len) + pStr[i % len];
}

inline uint32_t xorshift32(const uint32_t &seed = 0)
{
	static uint32_t state = 0;

	if (seed != 0)
		state = seed;

	state ^= state << 0xB;
	state ^= state >> 0x13;
	state ^= state << 0x7;
	return state;
}

inline void initWolfCrypt(const uint16_t &cryptVersion, const uint8_t *pPW, uint8_t *pKey, uint8_t *pKey2 = nullptr, uint8_t *pData = nullptr, const int64_t &start = -1, const int64_t &end = -1, const bool &other = false, const char *pKeyString = nullptr)
{
	uint8_t fac[3] = { 0 };

	// & 0xFF to prevent sign extension
	uint8_t s0 = (pPW[2] & 0xFF);
	uint8_t s1 = (pPW[5] & 0xFF);
	uint8_t s2 = (pPW[12] & 0xFF);
	uint8_t s3 = 0;

	if (!other)
	{
		uint8_t len = (pPW[11] & 0xFF) / 3;

		for (uint8_t i = 0; i < len; i++)
			s3 = i ^ std::rotr<uint8_t>(s3 ^ (pPW[i % 15] & 0xFF), 3);
	}
	else
	{
		uint8_t len = (pPW[8] & 0xFF) / 4;

		for (uint8_t i = 0; i < len; i++)
			s3 = i ^ std::rotr<uint8_t>(s3 ^ (pPW[i % 15] & 0xFF), 2);
	}

	const uint32_t seed = s0 * s1 + s2 + s3;
	srand(seed);

	fac[s3 % 3] = rand() % 256;

	if (!other && utils::isV35(cryptVersion))
		fac[1] = rand() % 0xFB; // This might need to be a += not sure

	for (uint32_t i = 0; i < 256; i++)
	{
		int16_t rn = rand() & 0xFFFF;

		pKey[i]       = fac[0] ^ (rand() & 0xFF);
		pKey[i + 256] = fac[1] ^ (rn >> 8);
		pKey[i + 512] = fac[2] ^ rn;
	}

	if (pKey2)
	{
		for (uint32_t j = 0; j < 128; j++)
		{
			int16_t rn = rand() & 0xFFFF;

			pKey[j] ^= s3 ^ pKey2[2] ^ (rn >> 8);
			pKey[j + 256] ^= s3 ^ pKey2[0] ^ rn;
		}
	}

	if (other)
	{
		// --------------------------------------------------------------

		uint8_t salt[128] = { 0 };
		uint8_t modFactor = 7;

		if (cryptVersion == 0x15E)
			calcSalt("958", salt);
		else
			calcSalt(pKeyString, salt);

		if (utils::isV35(cryptVersion))
		{
			s3 += 0x22;
			modFactor = 16;
		}

		for (uint32_t i = 0; i < 3; i++)
		{
			int32_t t = s3;

			for (uint32_t j = 0; j < 256; j++)
			{
				bool skip = false;

				uint8_t curS  = salt[j & 0x7F];
				uint8_t curS2 = salt[(j + i) % 0x80];
				uint8_t curK  = pKey[i * 256 + j];
				uint8_t sXk   = curS ^ curK;

				uint8_t round = (curS2 | (curS << 8)) % modFactor;

				uint8_t newK = sXk;

				switch (round)
				{
					case 1:
						if ((curS2 % 0xB) == 0)
							newK = curK;
						break;
					case 2:
						if ((curS % 0x1D) == 0)
							newK = ~sXk;
						break;
					case 3:
						if (((round + j) % 0x25) == 0)
							newK = curS2 ^ sXk;
						break;
					case 4:
						if (((curS + curS2) % 97) == 0)
							newK = curS + sXk;
						break;
					case 5:
						if (((j * round) % 0x7B) == 0)
							newK = sXk ^ t;
						break;
					case 6:
						if (curS == 0xFF && curS2 == 0)
						{
							newK = 0;
							skip = true;
						}
						break;
					case 7:
						if (cryptVersion < 0x154 || (cryptVersion > 0x3E8 && cryptVersion < 0x3FC))
							break;

						if ((((round + j) % 0x33) == 0) || cryptVersion >= 0x3FC)
							newK ^= curS;
						break;
					case 8:
						if (cryptVersion < 0x154 || (cryptVersion > 0x3E8 && cryptVersion < 0x3FC))
							break;

						if (((curS % 0x1D) == 0) || cryptVersion >= 0x3FC)
							newK ^= curS;
						break;
					default:
						break;
				}

				if (((j + i) % (curS % 5 + 1)) == 0)
					newK ^= t;
				else if (skip)
					newK = ~sXk;

				pKey[i * 256 + j] = newK;

				t += i;
			}
		}

		wolfCrypt(pKey, pData, start, end, true, cryptVersion);
	}
}

// --------------------------------------------------------------

inline void cryptAddresses(uint8_t *pData, const uint8_t *pKey, const uint32_t cryptVersion)
{
	uint16_t *pDataB16 = reinterpret_cast<uint16_t *>(pData);

	if (utils::isV35(cryptVersion))
	{
		uint32_t seed = 0xC + (pKey[9] & 0xFF) * (pKey[10] & 0xFF) + (pKey[3] & 0xFF);

		srand(seed);

		pDataB16 += 4;

		for (int32_t i = 0; i < 2; i++)
		{
			for (int32_t j = 3; j >= 0; j--)
				pDataB16[j] ^= rand() & 0xFFFF;

			pDataB16 += 4;
		}

		uint32_t *pDataB32 = reinterpret_cast<uint32_t *>(pDataB16);

		uint64_t r0 = static_cast<uint64_t>(rand()) << 17;
		uint64_t r1 = static_cast<uint64_t>(rand()) << 31;
		uint32_t v0 = (r0 & 0xFFFFFFFF) | (r1 & 0xFFFFFFFF) | rand();
		uint32_t v1 = (r0 >> 32) | (r1 >> 32);

		pDataB32[0] ^= v0;
		pDataB32[1] ^= v1;

		pDataB16 += 4;

		for (int32_t i = 3; i >= 0; i--)
			pDataB16[i] ^= rand() & 0xFFFF;
	}
	else
	{
		uint16_t *pDataB16 = reinterpret_cast<uint16_t *>(pData);

		srand((pKey[0] & 0xFF) + (pKey[7] & 0xFF) * (pKey[12] & 0xFF));

		pDataB16 += 4;

		for (int32_t i = 0; i < 4; i++)
		{
			for (int32_t j = 3; j >= 0; j--)
				pDataB16[j] ^= rand() & 0xFFFF;

			pDataB16 += 4;
		}
	}
}

inline void aesKeyGen(CryptData &cd, rng::RngData &rd, aes::AesKey &aesKey, aes::AesIV &aesIv)
{
	rng::runRngChain(rd, cd.seedBytes[0], cd.seedBytes[1]);

	std::array<uint8_t, rng::RngData::DATA_VEC_LEN> cryptData{};

	for (uint32_t i = 0; i < rng::RngData::DATA_VEC_LEN; i++)
		rng::aLotOfRngStuff(rd, i + cd.seedBytes[3], cd.seedBytes[2] - i, i, cryptData);

	uint8_t seed = cd.seedBytes[1] ^ cd.seedBytes[2];

	std::array<uint8_t, rng::RngData::DATA_VEC_LEN> indexes{};
	std::array<uint8_t, rng::RngData::DATA_VEC_LEN> resData{};
	std::iota(indexes.begin(), indexes.end(), 0);

	srand(seed);

	for (uint32_t i = 0; i < rng::RngData::DATA_VEC_LEN; i++)
	{
		uint32_t rn = rand();
		uint8_t old = indexes[i];
		indexes[i]  = indexes[rn % rng::RngData::DATA_VEC_LEN];

		indexes[rn % rng::RngData::DATA_VEC_LEN] = old;
	}

	for (uint32_t i = 0; i < rng::RngData::DATA_VEC_LEN; i++)
		resData[i] = cryptData[indexes[i]];

	const auto ivBegin = resData.begin() + aes::KEY_SIZE;

	std::copy(resData.begin(), ivBegin, aesKey.begin());
	std::copy(ivBegin, ivBegin + aes::IV_SIZE, aesIv.begin());
}


} // namespace wolf::crypt