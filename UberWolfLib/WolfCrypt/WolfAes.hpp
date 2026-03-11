/*
 *  File: WolfAes.hpp
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

#include <array>
#include <bit>
#include <cstdint>

#include "WolfCryptUtils.hpp"

namespace wolf::aes
{
inline constexpr uint8_t sbox[256] = {
	0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76, 0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0, 0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15, 0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75, 0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B,
	0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84, 0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF, 0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8, 0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2, 0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73, 0x60, 0x81, 0x4F, 0xDC,
	0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB, 0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79, 0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08, 0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A, 0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1,
	0x1D, 0x9E, 0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF, 0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16
};

inline constexpr uint8_t Rcon[11] = { 0x8D, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1B, 0x36 };

inline constexpr uint32_t Nk = 4;
inline constexpr uint32_t Nb = 4;
inline constexpr uint32_t Nr = 10;

inline constexpr uint32_t KEY_EXP_SIZE = 176;
inline constexpr uint32_t KEY_SIZE     = 16;
inline constexpr uint32_t IV_SIZE      = 16;
inline constexpr uint32_t BLOCKLEN     = 16;

inline constexpr uint32_t ROUND_KEY_SIZE = KEY_EXP_SIZE + IV_SIZE;

inline constexpr uint32_t PW_SIZE = 15;

using AesRoundKey = std::array<uint8_t, ROUND_KEY_SIZE>;
using AesKey      = std::array<uint8_t, KEY_SIZE>;
using AesIV       = std::array<uint8_t, IV_SIZE>;

// Init the AES RoundKey
inline void keyExpansion(uint8_t *pRoundKey, const uint8_t *pKey)
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

inline void initAES128(uint8_t *pRoundKey, const uint8_t *pPwd, uint8_t *pProKey, const uint16_t &cryptVersion)
{
	uint8_t proKeyZero[4] = { 0 };
	if (pProKey == nullptr)
		pProKey = proKeyZero;

	uint8_t key[KEY_SIZE] = { 0 };
	uint8_t iv[IV_SIZE]   = { 0 };

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
	std::memcpy(pRoundKey + KEY_EXP_SIZE, iv, IV_SIZE);
}

/////////////////////////////////
////// AES CTR Crypt -- based on: https://github.com/kokke/tiny-AES-c
// While this code should be a normal AES CTR implementation, the keyExpansion implementation above is not standard AES
// it contains minor changes when the tempa values are modified using the sbox values

inline void addRoundKey(uint8_t *pState, const uint8_t &round, const uint8_t *pRoundKey)
{
	for (uint32_t i = 0; i < KEY_SIZE; i++)
		pState[i] ^= pRoundKey[(round * KEY_SIZE) + i];
}

inline void subBytes(uint8_t *pState)
{
	for (uint32_t i = 0; i < KEY_SIZE; i++)
		pState[i] = sbox[pState[i]];
}

inline void shiftRows(uint8_t *pState)
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

inline uint8_t xtime(const uint8_t &x)
{
	return ((x << 1) ^ (((x >> 7) & 1) * 0x1b));
}

inline void mixColumns(uint8_t *pState)
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
inline void cipher(uint8_t *pState, const uint8_t *pRoundKey)
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
inline void aesCtrXCrypt(uint8_t *pData, uint8_t *pKey, const std::size_t &size)
{
	uint8_t state[BLOCKLEN];
	uint8_t *pIv = pKey + KEY_EXP_SIZE;
	int32_t bi   = BLOCKLEN;

	for (std::size_t i = 0; i < size; i++, bi++)
	{
		if (bi == BLOCKLEN)
		{
			std::memcpy(state, pIv, BLOCKLEN);

			cipher(state, pKey);

			for (bi = BLOCKLEN - 1; bi >= 0; bi--)
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

} // namespace wolf::aes