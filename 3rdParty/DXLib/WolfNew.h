/*
 *  File: WolfNew.h
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

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <numeric>
#include <random>
#include <string>
#include <vector>

namespace
{
bool isV35(const uint16_t &cryptVersion)
{
	return (cryptVersion >= 0x15E && cryptVersion < 0x3E8) || cryptVersion >= 0x3FC;
}

void wolfCrypt(const uint8_t *pKey, uint8_t *pData, const int64_t &start, const int64_t &end, const bool &updateDataPos, const uint16_t &cryptVersion)
{
	if (updateDataPos)
		pData += start;

	const uint64_t length = end - start;

	uint32_t v1Cnt = start % 256;
	uint32_t v2Cnt = start / 256 % 256;
	int32_t v3Cnt  = start / 0x10000 % 256;

	if (isV35(cryptVersion))
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

void calcSalt(const char *pStr, uint8_t *pSalt)
{
	if (!pSalt)
		return;

	uint32_t len = static_cast<uint32_t>(strlen(pStr));

	for (uint32_t i = 0; i < 128; i++)
		pSalt[i] = (i / len) + pStr[i % len];
}

uint32_t xorshift32(const uint32_t &seed = 0)
{
	static uint32_t state = 0;

	if (seed != 0)
		state = seed;

	state ^= state << 0xB;
	state ^= state >> 0x13;
	state ^= state << 0x7;
	return state;
}

void initWolfCrypt(const uint16_t &cryptVersion, const uint8_t *pPW, uint8_t *pKey, uint8_t *pKey2 = nullptr, uint8_t *pData = nullptr, const int64_t &start = -1, const int64_t &end = -1, const bool &other = false, const char *pKeyString = nullptr)
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

	if (!other && isV35(cryptVersion))
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

		if (isV35(cryptVersion))
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

void cryptAddresses(uint8_t *pData, const uint8_t *pKey, const uint32_t cryptVersion)
{
	uint16_t *pDataB16 = reinterpret_cast<uint16_t *>(pData);

	if (isV35(cryptVersion))
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

// --------------------------------------------------------------

static constexpr uint8_t sbox[256] = {
	0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76, 0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0, 0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15, 0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75, 0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B,
	0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84, 0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF, 0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8, 0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2, 0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73, 0x60, 0x81, 0x4F, 0xDC,
	0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB, 0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79, 0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08, 0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A, 0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1,
	0x1D, 0x9E, 0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF, 0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16
};

static constexpr uint8_t Rcon[11] = { 0x8D, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1B, 0x36 };

#define Nk 4
#define Nb 4
#define Nr 10

#define AES_KEY_EXP_SIZE 176
#define AES_KEY_SIZE     16
#define AES_IV_SIZE      16
#define AES_BLOCKLEN     16

#define AES_ROUND_KEY_SIZE AES_KEY_EXP_SIZE + AES_IV_SIZE

#define PW_SIZE 15

using AesRoundKey = std::array<uint8_t, AES_ROUND_KEY_SIZE>;
using AesKey      = std::array<uint8_t, AES_KEY_SIZE>;
using AesIV       = std::array<uint8_t, AES_IV_SIZE>;

// Init the AES RoundKey
void keyExpansion(uint8_t *pRoundKey, const uint8_t *pKey)
{
	uint8_t tempa[4] = { 0 };

	// The first round key is the key itself.
	for (uint32_t i = 0; i < Nk; i++)
	{
		pRoundKey[(i * 4) + 0] = pKey[(i * 4) + 0];
		pRoundKey[(i * 4) + 1] = pKey[(i * 4) + 1];
		pRoundKey[(i * 4) + 2] = pKey[(i * 4) + 2];
		pRoundKey[(i * 4) + 3] = pKey[(i * 4) + 3];
	}

	for (uint32_t i = Nk; i < Nb * (Nr + 1); i++)
	{
		uint32_t k = (i - 1) * 4;

		tempa[0] = pRoundKey[k + 0];
		tempa[1] = pRoundKey[k + 1];
		tempa[2] = pRoundKey[k + 2];
		tempa[3] = pRoundKey[k + 3];

		if ((i % Nk) == 0)
		{
			const uint8_t u8tmp = tempa[0];
			tempa[0]            = tempa[1];
			tempa[1]            = tempa[2];
			tempa[2]            = tempa[3];
			tempa[3]            = u8tmp;

			// This differs between the original and the WolfRPG version
			tempa[0] = sbox[tempa[0]] ^ Rcon[i / Nk];
			tempa[1] = sbox[tempa[1]] >> 4;
			tempa[2] = ~sbox[tempa[2]];
			tempa[3] = std::rotr(sbox[tempa[3]], 7);
		}

		const uint32_t j = i * 4;

		k = (i - Nk) * 4;

		pRoundKey[j + 0] = pRoundKey[k + 0] ^ tempa[0];
		pRoundKey[j + 1] = pRoundKey[k + 1] ^ tempa[1];
		pRoundKey[j + 2] = pRoundKey[k + 2] ^ tempa[2];
		pRoundKey[j + 3] = pRoundKey[k + 3] ^ tempa[3];
	}
}

void initAES128(uint8_t *pRoundKey, const uint8_t *pPwd, uint8_t *pProKey, const uint16_t &cryptVersion)
{
	uint8_t proKeyZero[4] = { 0 };
	if (pProKey == nullptr)
		pProKey = proKeyZero;

	uint8_t key[AES_KEY_SIZE] = { 0 };
	uint8_t iv[AES_IV_SIZE]   = { 0 };

	if (isV35(cryptVersion))
	{
		for (uint32_t i = 0; i < PW_SIZE; i++)
		{
			const uint8_t &proKeyElem = pProKey[i % 4];
			uint8_t pwIdxKey          = ((i * (proKeyElem % 5 + 7)) ^ (3 * pPwd[i])) % PW_SIZE;
			uint8_t pwIdxIv           = (i * ((pProKey[(i + 1) % 4] % 7) + 0xB) ^ (5 * pPwd[(i + 3) % 15])) % PW_SIZE;

			key[i] ^= ((i ^ proKeyElem) + (pPwd[pwIdxKey] << (i % 3))) % 0xFB;
			iv[i] ^= ((pPwd[pwIdxIv] >> (i % 2)) + ((i * i) ^ pProKey[(i + 2) % 4])) % 0xF6;

			key[PW_SIZE] ^= 7 * (pPwd[i] + ((i + 1) ^ proKeyElem)) % 0xFD;
			iv[PW_SIZE] ^= 11 * static_cast<uint16_t>((pPwd[i] - ((i * 2) ^ pProKey[(i + 2) % 4]))) % 0x100;
		}
	}
	else if (cryptVersion == 0x3F2)
	{
		for (uint32_t i = 0; i < PW_SIZE; i++)
		{
			key[i] ^= (pPwd[(i * 7) % 0xF] + pProKey[i & 3]) * i * i;
			iv[i] ^= (pPwd[(i * 11) % 0xF] + pProKey[(i + 2) % 4]) - i * i;

			key[PW_SIZE] ^= (i * 3) + pPwd[i] + pProKey[i & 3];
			iv[PW_SIZE] ^= (i * 5) + pPwd[i] + pProKey[(i + 2) % 4];
		}
	}
	else
	{
		for (uint32_t i = 0; i < PW_SIZE; i++)
		{
			key[i] ^= pPwd[(i * 7) % 0xF] + i * i;
			iv[i] ^= pPwd[(i * 11) % 0xF] - i * i;

			key[PW_SIZE] ^= pPwd[i] + (i * 3);
			iv[PW_SIZE] ^= pPwd[i] + (i * 5);
		}
	}

	key[0] ^= pProKey[0];
	iv[10] ^= pProKey[0];

	key[4] ^= pProKey[1];
	iv[1] ^= pProKey[1];

	key[8] ^= pProKey[2];
	iv[4] ^= pProKey[2];

	key[12] ^= pProKey[3];
	iv[7] ^= pProKey[3];

	keyExpansion(pRoundKey, key);
	std::memcpy(pRoundKey + AES_KEY_EXP_SIZE, iv, AES_IV_SIZE);
}

/////////////////////////////////
////// AES CTR Crypt -- based on: https://github.com/kokke/tiny-AES-c
// While this code should be a normal AES CTR implementation, the keyExpansion implementation above is not standard AES
// it contains minor changes when the tempa values are modified using the sbox values

void addRoundKey(uint8_t *pState, const uint8_t &round, const uint8_t *pRoundKey)
{
	for (uint32_t i = 0; i < AES_KEY_SIZE; i++)
		pState[i] ^= pRoundKey[(round * AES_KEY_SIZE) + i];
}

void subBytes(uint8_t *pState)
{
	for (uint32_t i = 0; i < AES_KEY_SIZE; i++)
		pState[i] = sbox[pState[i]];
}

void shiftRows(uint8_t *pState)
{
	uint8_t temp;

	temp       = pState[1];
	pState[1]  = pState[5];
	pState[5]  = pState[9];
	pState[9]  = pState[13];
	pState[13] = temp;

	temp       = pState[2];
	pState[2]  = pState[10];
	pState[10] = temp;

	temp       = pState[6];
	pState[6]  = pState[14];
	pState[14] = temp;

	temp       = pState[3];
	pState[3]  = pState[15];
	pState[15] = pState[11];
	pState[11] = pState[7];
	pState[7]  = temp;
}

uint8_t xtime(const uint8_t &x)
{
	return ((x << 1) ^ (((x >> 7) & 1) * 0x1b));
}

void mixColumns(uint8_t *pState)
{
	uint8_t tmp;
	uint8_t t;

	for (uint32_t i = 0; i < 4; i++)
	{
		t   = pState[0];
		tmp = pState[1] ^ pState[0] ^ pState[2] ^ pState[3];

		pState[0] ^= tmp ^ xtime(pState[1] ^ pState[0]);
		pState[1] ^= tmp ^ xtime(pState[2] ^ pState[1]);
		pState[2] ^= tmp ^ xtime(pState[2] ^ pState[3]);
		pState[3] ^= tmp ^ xtime(pState[3] ^ t);

		pState += 4;
	}
}

// AES Cipher
void cipher(uint8_t *pState, const uint8_t *pRoundKey)
{
	addRoundKey(pState, 0, pRoundKey);

	for (uint32_t round = 1; round < Nr; round++)
	{
		subBytes(pState);
		shiftRows(pState);
		mixColumns(pState);
		addRoundKey(pState, round, pRoundKey);
	}

	subBytes(pState);
	shiftRows(pState);
	addRoundKey(pState, Nr, pRoundKey);
}

// AES_CTR_xcrypt
void aesCtrXCrypt(uint8_t *pData, uint8_t *pKey, const std::size_t &size)
{
	uint8_t state[AES_BLOCKLEN];
	uint8_t *pIv = pKey + AES_KEY_EXP_SIZE;
	int32_t bi   = AES_BLOCKLEN;

	for (std::size_t i = 0; i < size; i++, bi++)
	{
		if (bi == AES_BLOCKLEN)
		{
			std::memcpy(state, pIv, AES_BLOCKLEN);

			cipher(state, pKey);

			for (bi = AES_BLOCKLEN - 1; bi >= 0; bi--)
			{
				if (pIv[bi] == 0xFF)
				{
					pIv[bi] = 0;
					continue;
				}
				pIv[bi]++;
				break;
			}
			bi = 0;
		}

		pData[i] ^= state[bi];
	}
}

////// AES CTR Crypt
/////////////////////////////////

struct CryptData
{
	std::array<uint8_t, 4> keyBytes  = { 0 };
	std::array<uint8_t, 4> seedBytes = { 0 };

	std::vector<uint8_t> gameDatBytes = {};

	uint32_t dataSize = 0;

	uint32_t seed1 = 0;
	uint32_t seed2 = 0;
};

struct RngData
{
	static constexpr uint32_t OUTER_VEC_LEN = 0x20;
	static constexpr uint32_t INNER_VEC_LEN = 0x100;
	static constexpr uint32_t DATA_VEC_LEN  = 0x30;

	uint32_t seed1   = 0;
	uint32_t seed2   = 0;
	uint32_t counter = 0;

	std::vector<std::vector<uint32_t>> data = std::vector<std::vector<uint32_t>>(OUTER_VEC_LEN, std::vector<uint32_t>(INNER_VEC_LEN, 0));

	void Reset()
	{
		seed1   = 0;
		seed2   = 0;
		counter = 0;

		data = std::vector<std::vector<uint32_t>>(OUTER_VEC_LEN, std::vector<uint32_t>(INNER_VEC_LEN, 0));
	}
};

uint32_t customRng1(RngData &rd)
{
	uint32_t state;
	uint32_t stateMod;

	const uint32_t seedP1 = (rd.seed1 ^ (((rd.seed1 << 11) ^ rd.seed1) >> 8));
	const uint32_t seed   = (rd.seed1 << 11) ^ seedP1;

	state = 1664525 * seed + 1013904223;

	if (((13 * seedP1 + 95) & 1) == 0)
		stateMod = state / 8;
	else
		stateMod = state * 4;

	state ^= stateMod;

	if ((state & 0x400) != 0)
	{
		state ^= state << 21;
		stateMod = state >> 9;
	}
	else
	{
		state ^= state * 4;
		stateMod = state >> 22;
	}

	state ^= stateMod;

	if ((state & 0xFFFFF) == 0)
		state += 256;

	rd.seed1 = state;
	return state;
}

uint32_t customRng2(RngData &rd)
{
	uint32_t stateMod;
	uint32_t state;

	uint32_t seed = rd.seed1;

	state    = 1664525 * seed + 1013904223;
	stateMod = (seed & 7) + 1;

	if (state % 3)
	{
		if (state % 3 == 1)
			state ^= (state >> stateMod);
		else
			state = ~state + (state << stateMod);
	}
	else
		state ^= (state << stateMod);

	if (state)
	{
		if (!static_cast<uint16_t>(state))
			state ^= 0x55AA55AA;
	}
	else
		state = 0x173BEF;

	rd.seed1 = state;
	return state;
}

uint32_t customRng3(RngData &rd)
{
	uint32_t state;
	uint32_t seed = rd.seed2;

	state = (1566083941 * rd.seed2) ^ (292331520 * rd.seed2);
	state ^= (state >> 17) ^ (32 * (state ^ (state >> 17)));
	state = 69069 * (state ^ (state ^ (state >> 11)) & 0x3FFFFFFF);

	if (state)
	{
		if (!static_cast<uint16_t>(state))
			state ^= 0x59A6F141;

		if ((state & 0xFFFFF) == 0)
			state += 256;
	}
	else
		state = 1566083941;

	rd.seed2 = state;
	return state;
}

void rngChain(RngData &rd, std::vector<uint32_t> &data)
{
	uint32_t i = 0;
	for (uint32_t &d : data)
	{
		uint32_t rn = customRng2(rd);

		d = rn ^ customRng3(rd);

		if ((++rd.counter & 1) == 0)
			d += customRng3(rd);

		if (!(rd.counter % 3))
			d ^= customRng1(rd) + 3;

		if (!(rd.counter % 7))
			d += customRng3(rd) + 1;

		if ((rd.counter & 7) == 0)
			d *= customRng1(rd);

		if (!((i + rd.seed1) % 5))
			d ^= customRng1(rd);

		if (!(rd.counter % 9))
			d += customRng2(rd) + 4;

		if (!(rd.counter % 0x18))
			d += customRng2(rd) + 7;

		if (!(rd.counter % 0x1F))
			d += 3 * customRng3(rd);

		if (!(rd.counter % 0x3D))
			d += customRng3(rd) + 1;

		if (!(rd.counter % 0xA1))
			d += customRng2(rd);

		if (static_cast<uint16_t>(rn) == 256)
			d += 3 * customRng3(rd);

		i++;
	}
}

void runCrypt(RngData &rd, const uint32_t &seed1, const uint32_t &seed2)
{
	rd.seed1   = seed1;
	rd.seed2   = seed2;
	rd.counter = 0;

	srand(seed1);

	for (uint32_t i = 0; i < rd.data.size(); i++)
		rngChain(rd, rd.data[i]);
}

void aLotOfRngStuff(RngData &rd, uint32_t a2, uint32_t a3, const uint32_t &idx, std::vector<uint8_t> &cryptData)
{
	uint32_t itrs = 20;

	for (uint32_t i = 0; i < itrs; i++)
	{
		uint32_t idx1 = (a2 ^ customRng1(rd)) & 0x1F;
		uint32_t idx2 = (a3 ^ customRng2(rd)) & 0xFF;
		a3            = rd.data[idx1][idx2];

		switch ((a2 + rd.counter) % 0x14u)
		{
			case 1:
				rngChain(rd, rd.data[(a2 + 5) & 0x1F]);
				break;
			case 2:
				a3 ^= customRng1(rd);
				break;
			case 5:
				if ((a2 & 0xFFFFF) == 0)
					cryptData[idx] ^= customRng3(rd);
				break;
			case 9:
			case 0xE:
				cryptData[customRng2(rd) % 0x30] += a3;
				break;
			case 0xB:
				cryptData[idx] ^= customRng1(rd);
				break;
			case 0x11:
				itrs++;
				break;
			case 0x13:
				if (static_cast<uint16_t>(a2) == 0)
					cryptData[idx] ^= customRng2(rd);
				break;
			default:
				break;
		}

		a2 += customRng3(rd);

		if (itrs > 50)
			itrs = 50;
	}

	cryptData[idx] += a3;
}

void initCrypt(CryptData &cd)
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

void aesKeyGen(CryptData &cd, RngData &rd, std::array<uint8_t, AES_KEY_SIZE> &aesKey, std::array<uint8_t, AES_IV_SIZE> &aesIv)
{
	runCrypt(rd, cd.seedBytes[0], cd.seedBytes[1]);

	std::vector<uint8_t> cryptData(RngData::DATA_VEC_LEN, 0);

	for (uint32_t i = 0; i < RngData::DATA_VEC_LEN; i++)
		aLotOfRngStuff(rd, i + cd.seedBytes[3], cd.seedBytes[2] - i, i, cryptData);

	uint8_t seed = cd.seedBytes[1] ^ cd.seedBytes[2];

	std::vector<uint8_t> indexes(RngData::DATA_VEC_LEN, 0);
	std::vector<uint8_t> resData(RngData::DATA_VEC_LEN, 0);
	std::iota(indexes.begin(), indexes.end(), 0);

	srand(seed);

	for (uint32_t i = 0; i < RngData::DATA_VEC_LEN; i++)
	{
		uint32_t rn = rand();
		uint8_t old = indexes[i];
		indexes[i]  = indexes[rn % RngData::DATA_VEC_LEN];

		indexes[rn % RngData::DATA_VEC_LEN] = old;
	}

	for (uint32_t i = 0; i < RngData::DATA_VEC_LEN; i++)
		resData[i] = cryptData[indexes[i]];

	const auto ivBegin = resData.begin() + AES_KEY_SIZE;

	std::copy(resData.begin(), resData.begin() + AES_KEY_SIZE, aesKey.begin());
	std::copy(ivBegin, ivBegin + AES_IV_SIZE, aesIv.begin());
}

CryptData decryptV2File(const std::vector<uint8_t> &gameDataBytes)
{
	CryptData cd;
	RngData rd;

	cd.gameDatBytes = gameDataBytes;

	initCrypt(cd);

	runCrypt(rd, cd.seed1, cd.seed2);

	AesKey aesKey;
	AesIV aesIv;

	aesKeyGen(cd, rd, aesKey, aesIv);

	AesRoundKey roundKey;

	keyExpansion(roundKey.data(), aesKey.data());
	std::copy(aesIv.begin(), aesIv.end(), roundKey.begin() + AES_KEY_EXP_SIZE);

	aesCtrXCrypt(cd.gameDatBytes.data() + 30, roundKey.data(), cd.dataSize);

	return cd;
}

std::vector<uint8_t> calcKey(const std::vector<uint8_t> &gameDataBytes)
{
	std::vector<uint8_t> key;
	CryptData cd = decryptV2File(gameDataBytes);

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

uint32_t genMTSeed(const std::array<uint8_t, 3> &seeds)
{
	uint32_t x = (seeds[0] << 16) | (seeds[1] << 8) | seeds[2];
	uint32_t y = (x << 13) ^ x;
	uint32_t z = (y >> 17) ^ y;

	return z ^ (z << 5);
}

void decrpytProV2P1(std::vector<uint8_t> &data, const uint32_t &seed)
{
	const uint32_t NUM_RNDS = 128;

	std::mt19937 gen;
	gen.seed(seed);

	std::array<uint32_t, NUM_RNDS> rnds;

	for (uint32_t &rnd : rnds)
		rnd = gen();

	for (uint32_t i = 0xA; i < data.size(); i++)
		data[i] ^= rnds[i % NUM_RNDS];
}

void initCryptProt(CryptData &cd)
{
	uint32_t fileSize = static_cast<uint32_t>(cd.gameDatBytes.size());

	if (fileSize - 20 < 326)
		cd.dataSize = fileSize - 20;
	else
		cd.dataSize = 326;

	decrpytProV2P1(cd.gameDatBytes, genMTSeed({ cd.gameDatBytes[0], cd.gameDatBytes[8], cd.gameDatBytes[6] }));

	std::copy(cd.gameDatBytes.begin() + 0xB, cd.gameDatBytes.begin() + 0xF, cd.keyBytes.begin());

	cd.seedBytes[0] = cd.gameDatBytes[7] + 3 * cd.keyBytes[0];
	cd.seedBytes[1] = cd.keyBytes[1] ^ cd.keyBytes[2];
	cd.seedBytes[2] = cd.keyBytes[3] ^ cd.gameDatBytes[7];
	cd.seedBytes[3] = cd.keyBytes[2] + cd.gameDatBytes[7] - cd.keyBytes[0];

	cd.seed1 = cd.keyBytes[1] ^ cd.keyBytes[2];
	cd.seed2 = cd.keyBytes[1] ^ cd.keyBytes[2];
}

static constexpr uint32_t ENCRYPTED_KEY_SIZE = 128;

bool validateKey(const std::vector<uint8_t> &key, const std::array<uint8_t, ENCRYPTED_KEY_SIZE> &tarKey)
{
	if (key.empty()) return false;

	const uint32_t keyLen = static_cast<uint32_t>(key.size());

	if (keyLen > ENCRYPTED_KEY_SIZE)
		throw std::runtime_error("Key is too long");

	std::array<uint8_t, ENCRYPTED_KEY_SIZE> keyBytes;

	for (uint32_t i = 0; i < keyLen; i++)
		keyBytes[i] = key[i];

	for (uint32_t i = 0; i < ENCRYPTED_KEY_SIZE; i++)
		keyBytes[i] = i / keyLen + key[i % keyLen];

	// Compare the target key and the generated key
	return std::equal(keyBytes.begin(), keyBytes.end(), tarKey.begin());
}

std::vector<uint8_t> findKey(const std::array<uint8_t, ENCRYPTED_KEY_SIZE> &encKey)
{
	const uint32_t MIN_KEY_LEN = 4;
	for (uint32_t i = MIN_KEY_LEN; i < ENCRYPTED_KEY_SIZE; i++)
	{
		std::vector<uint8_t> key(i, 0);
		std::copy(encKey.begin(), encKey.begin() + i, key.begin());

		if (validateKey(key, encKey))
			return key;
	}

	return {};
}

std::vector<uint8_t> calcKeyProt(const std::vector<uint8_t> &gameDatBytes)
{
	CryptData cd;
	RngData rd;

	cd.gameDatBytes = gameDatBytes;
	initCryptProt(cd);

	runCrypt(rd, cd.seed1, cd.seed2);

	AesKey aesKey;
	AesIV aesIv;

	aesKeyGen(cd, rd, aesKey, aesIv);

	AesRoundKey roundKey;

	keyExpansion(roundKey.data(), aesKey.data());
	std::copy(aesIv.begin(), aesIv.end(), roundKey.begin() + AES_KEY_EXP_SIZE);

	aesCtrXCrypt(cd.gameDatBytes.data() + 20, roundKey.data(), cd.dataSize);

	rd.Reset();

	runCrypt(rd, cd.keyBytes[3], cd.keyBytes[0]);

	cd.seedBytes = cd.keyBytes;

	aesKeyGen(cd, rd, aesKey, aesIv);

	keyExpansion(roundKey.data(), aesKey.data());
	std::copy(aesIv.begin(), aesIv.end(), roundKey.begin() + AES_KEY_EXP_SIZE);

	std::array<uint8_t, ENCRYPTED_KEY_SIZE> encryptedKey;
	const auto &keyBegin = cd.gameDatBytes.begin() + 0xF;
	std::copy(keyBegin, keyBegin + ENCRYPTED_KEY_SIZE, encryptedKey.begin());

	aesCtrXCrypt(encryptedKey.data(), roundKey.data(), ENCRYPTED_KEY_SIZE);

	return findKey(encryptedKey);
}

// TODO: Move ChaCha20 into a class
////////////////////////////
// ChaCha20 implementation
// Based on: https://github.com/Ginurx/chacha20-c

uint32_t pack4(const uint8_t *a)
{
	uint32_t res = 0;
	res |= (uint32_t)a[0] << 0 * 8;
	res |= (uint32_t)a[1] << 1 * 8;
	res |= (uint32_t)a[2] << 2 * 8;
	res |= (uint32_t)a[3] << 3 * 8;
	return res;
}

void chacha20_init_block(uint32_t *pState, const uint8_t *pKey, const uint8_t *pNonce)
{
	const uint8_t *magic_constant = (uint8_t *)"expand 32-byte k";

	pState[0]  = pack4(magic_constant + 0 * 4);
	pState[1]  = pack4(magic_constant + 1 * 4);
	pState[2]  = pack4(magic_constant + 2 * 4);
	pState[3]  = pack4(magic_constant + 3 * 4);
	pState[4]  = pack4(pKey + 0 * 4);
	pState[5]  = pack4(pKey + 1 * 4);
	pState[6]  = pack4(pKey + 2 * 4);
	pState[7]  = pack4(pKey + 3 * 4);
	pState[8]  = pack4(pKey + 4 * 4);
	pState[9]  = pack4(pKey + 5 * 4);
	pState[10] = pack4(pKey + 6 * 4);
	pState[11] = pack4(pKey + 7 * 4);

	pState[12] = 1; // Initialize the counter to 1
	pState[13] = pack4(pNonce + 0 * 4);
	pState[14] = pack4(pNonce + 1 * 4);
	pState[15] = pack4(pNonce + 2 * 4);
}

uint32_t rotl32(uint32_t x, int n)
{
	return (x << n) | (x >> (32 - n));
}

void chacha20_block_next(uint32_t *pState, uint32_t *pKeyStream)
{
	// This is where the crazy voodoo magic happens.
	// Mix the bytes a lot and hope that nobody finds out how to undo it.
	for (uint32_t i = 0; i < 16; i++)
		pKeyStream[i] = pState[i];

#define CHACHA20_QUARTERROUND(x, a, b, c, d) \
	x[a] += x[b];                            \
	x[d] = rotl32(x[d] ^ x[a], 16);          \
	x[c] += x[d];                            \
	x[b] = rotl32(x[b] ^ x[c], 12);          \
	x[a] += x[b];                            \
	x[d] = rotl32(x[d] ^ x[a], 8);           \
	x[c] += x[d];                            \
	x[b] = rotl32(x[b] ^ x[c], 7);

	for (uint32_t i = 0; i < 10; i++)
	{
		CHACHA20_QUARTERROUND(pKeyStream, 0, 4, 8, 12)
		CHACHA20_QUARTERROUND(pKeyStream, 1, 5, 9, 13)
		CHACHA20_QUARTERROUND(pKeyStream, 2, 6, 10, 14)
		CHACHA20_QUARTERROUND(pKeyStream, 3, 7, 11, 15)
		CHACHA20_QUARTERROUND(pKeyStream, 0, 5, 10, 15)
		CHACHA20_QUARTERROUND(pKeyStream, 1, 6, 11, 12)
		CHACHA20_QUARTERROUND(pKeyStream, 2, 7, 8, 13)
		CHACHA20_QUARTERROUND(pKeyStream, 3, 4, 9, 14)
	}

	for (uint32_t i = 0; i < 16; i++)
		pKeyStream[i] += pState[i];

	uint32_t *counter = pState + 12;

	// increment counter
	counter[0]++;
	if (0 == counter[0])
		counter[1]++;
}

// Slightly modified version of the default functionality
// - Counter is initialized to (1 + startPos / 64)
// - The number of steps done can vary at the beginning or end of the function, depending on startPos
//   - For startPos % 64 != 0, the first block will not process 64 bit
//   - For length % 64 != 0, the last block will not process 64 bit
void chacha20_xor(uint32_t *pState, uint32_t *pKeyStream, const uint32_t &startPos, uint8_t *bytes, const uint64_t &length)
{
	uint8_t *keystream8 = (uint8_t *)pKeyStream;
	uint64_t position   = 0;
	uint64_t offset     = startPos % 64;

	pState[12] += startPos / 64;

	while (position < length)
	{
		uint32_t steps = static_cast<uint32_t>(std::min(64 - offset, length - position));
		chacha20_block_next(pState, pKeyStream);

		for (uint32_t i = 0; i < steps; i++)
			bytes[position + i] ^= keystream8[offset + i];

		position += steps;
		offset = 0;
	}
}

void chacha20_keySetup(const std::array<uint8_t, 4> &data, std::array<uint8_t, 64> &key)
{
	static constexpr uint8_t mod1[4] = { 0x3F, 0xA7, 0xD2, 0x1C };
	static constexpr uint8_t mod2[4] = { 0xB4, 0xE1, 0x9D, 0x58 };
	static constexpr uint8_t mod3[4] = { 0x6A, 0x2B, 0x4C, 0x8E };

	key = { 0 };

	// Only go to < 63 because the last byte is hard set to 0
	for (uint32_t i = 0; i < 63; i++)
	{
		uint8_t index = i % 4;
		uint8_t temp  = (data[index] + mod2[index]) ^ (mod1[index] + i + (16 * i));

		if ((i % 2) == 0)
			temp = (temp >> 5) | (temp << 3);
		else
			temp = (temp >> 2) | (temp << 6);

		key[i] = ~(temp ^ data[index] ^ mod3[index]);
	}
}
} // namespace
