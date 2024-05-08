#pragma once

#include <bit>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <windows.h>

void specialCrypt(int8_t *pKey, int8_t *pData, int64_t start, int64_t end, const bool &updateDataPos = false)
{
	const int64_t len = end - start;

	if (updateDataPos)
		pData += start;

	int32_t v1Cnt = start % 256;
	int32_t v2Cnt = start / 256 % 256;
	int32_t v3Cnt = start / 0x10000 % 256;

	for (int64_t i = 0; i < len; i++)
	{
		pData[i] ^= pKey[v1Cnt++] ^ pKey[v2Cnt + 256] ^ pKey[v3Cnt + 512];

		if (v1Cnt == 256)
		{
			v1Cnt = 0;
			++v2Cnt;

			if (v2Cnt == 256)
			{
				v2Cnt = 0;
				++v3Cnt;

				if (v3Cnt == 256)
					v3Cnt = 0;
			}
		}
	}
}

void sub_C38920(const char *a1, char *a2)
{
	if (!a2)
		return;

	uint32_t len = strlen(a1);

	for (uint32_t i = 0; i < 128; i++)
		a2[i] = (i / len) + a1[i % len];

	return;
}

void initSpecialCrypt(int8_t *pA3, int8_t *pKey, int8_t *pData = nullptr, const int64_t &start = -1, const int64_t &end = -1, const bool &other = false)
{
	int32_t v86 = 0;
	int32_t v85 = 0;
	int32_t v84 = 0;

	if (!other)
	{
		uint8_t v14 = 0;
		int64_t v49 = 2863311531i64 * (pA3[11] & 0xFF);
		int32_t v76 = 0;

		uint32_t v50 = (v49 >> 32) >> 1;
		int8_t v81   = 0;

		for (uint32_t i = 0; i < v50; i++)
			v14 = i ^ std::rotr<uint8_t>(v14 ^ (pA3[i % 15] & 0xFF), 3);

		uint32_t v1 = (pA3[2] & 0xFF);
		uint32_t v2 = (pA3[5] & 0xFF);
		uint32_t v3 = (pA3[12] & 0xFF);
		uint32_t v4 = (v14 & 0xFF);

		uint32_t seed = v1 * v2 + v3 + v4;

		srand(seed);

		int32_t v21 = rand() % 256;
		int32_t v22 = v14 % 3;

		switch (v22)
		{
			case 0:
				v84 = v21;
				break;
			case 1:
				v85 = v21;
				break;
			case 2:
				v86 = v21;
				break;
		}

		for (uint32_t i = 0; i < 256; i++)
		{
			int16_t v54 = rand();
			int8_t v55  = rand();

			pKey[i]       = v84 ^ v55;
			pKey[i + 256] = v85 ^ (v54 >> 8);
			pKey[i + 512] = v86 ^ v54;
		}
	}
	else
	{
		uint32_t v15 = 0;
		uint32_t v16 = (pA3[8] & 0xFF) >> 2;
		int8_t v81   = 0;
		uint8_t v17  = 0;
		if (v16)
		{
			do
			{
				v17 = v15 ^ std::rotr<uint8_t>(v81 ^ pA3[v15 + -15 * (v15 / 0xF)], 2);
				++v15;
				v81 = v17;
			} while (v15 < v16);
		}
		else
			v17 = 0;

		int32_t v18 = v17;
		int32_t v19 = (pA3[2] & 0xFF) * (pA3[5] & 0xFF);
		int32_t v20 = (pA3[12] & 0xFF);
		int32_t v76 = v18;
		srand(v18 + v19 + v20);
		int32_t v21 = rand() % 256;
		int32_t v22 = v18 % 3;
		int32_t v23 = 0;
		switch (v22)
		{
			case 0:
				v84 = v21;
				break;
			case 1:
				v85 = v21;
				break;
			case 2:
				v86 = v21;
				break;
		}

		do
		{
			int16_t v24       = rand();
			int8_t v25        = rand();
			pKey[v23]         = v84 ^ v25;
			pKey[v23 + 256]   = v85 ^ (v24 >> 8);
			pKey[v23++ + 512] = v86 ^ v24;
		} while (v23 < 256);

		int8_t v88[128] = { 0 };
		// This is the archive crypt key (see list in wolfdec.cpp)
		uint8_t s[] = { 0xCA, 0x08, 0x4C, 0x5D, 0x17, 0x0D, 0xDA, 0xA1, 0xD7, 0x27, 0xC8, 0x41, 0x54, 0x38, 0x82, 0x32, 0x54, 0xB7, 0xF9, 0x46, 0x8E, 0x13, 0x6B, 0xCA, 0xD0, 0x5C, 0x95, 0x95, 0xE2, 0xDC, 0x03, 0x53, 0x60, 0x9B, 0x4A, 0x38, 0x17, 0xF3, 0x69, 0x59, 0xA4, 0xC7, 0x9A, 0x43, 0x63, 0xE6, 0x54, 0xAF, 0xDB, 0xBB, 0x43, 0x58, 0x00 };

		int32_t v27 = v76;

		if (strlen((const char *)s))
			sub_C38920((const char *)s, (char *)v88);
		else
			sub_C38920("958", (char *)v88);

		int32_t v28  = 0;
		int32_t v74  = 0;
		int8_t *v29  = pKey;
		uint32_t v75 = 97;

		do
		{
			int32_t v30 = 0;
			int32_t v79 = v27;
			int32_t v72 = 0;
			do
			{
				uint8_t v31  = v88[v30 & 0x7F];
				int32_t v77  = v30 + v28;
				uint8_t v32  = v88[(v30 + v28) % 128];
				uint32_t v80 = v31;
				v81          = v32;
				uint32_t v73 = v32;
				uint32_t v33 = (v32 | (v31 << 8)) % 7u;
				char v78     = *v29;
				char v34     = v31 ^ v78;
				*v29         = v31 ^ v78;
				char v35     = v34;
				uint32_t v36 = 0;
				char v37     = 0;

				switch (v33)
				{
					case 1u:
						v35 = v34;
						if (v73 % 0xB)
							goto LABEL_22;
						v35 = v78;
						goto LABEL_21;
					case 2u:
						v36 = v80;
						v35 = v34;
						if (!(v80 % 0x1D))
						{
							v35  = ~v34;
							*v29 = ~v34;
						}
						break;
					case 3u:
						v35 = v34;
						if ((v33 + v30) % 0x25)
							goto LABEL_22;
						v35 = v81 ^ v34;
						goto LABEL_21;
					case 4u:
						v36 = v80;
						v35 = v34;
						if (!((v80 + v73) % v75))
						{
							v35  = v31 + v34;
							*v29 = v31 + v34;
						}
						break;
					case 5u:
						v35 = v34;
						if (v30 * v33 % 0x7B)
							goto LABEL_22;
						v35 = v34 ^ v79;
					LABEL_21:
						*v29 = v35;
					LABEL_22:
						v36 = v80;
						break;
					case 6u:
						v35 = v34;
						if (v31 != 0xFF)
							goto LABEL_22;
						v36 = v80;
						if (!v81)
						{
							v37  = ~v34;
							*v29 = v37;
							v35  = v37;
						}
						break;
					default:
						goto LABEL_22;
				}
				if (!(v77 % (int)(v36 % 5 + 1)))
					*v29 = v35 ^ v79;
				++v29;
				v28 = v74;
				v30 = v72 + 1;
				v79 += v74;
				v72 = v30;
			} while (v30 < 256);
			v27 = v76;
			v28 = v74 + 1;
			v74 = v28;
		} while (v28 < 3);

		specialCrypt(pKey, pData, start, end, true);
	}
}

// --------------------------------------------------------------

void cryptAddresses(int8_t *pData, int8_t *pKey)
{
	uint16_t *pDataB16 = reinterpret_cast<uint16_t *>(pData);

	srand((pKey[0] & 0xFF) + (pKey[7] & 0xFF) * (pKey[12] & 0xFF));

	pDataB16 += 3;

	for (int32_t i = 0; i < 4; i++)
	{
		for (int32_t j = 4; j > 0; j--)
			pDataB16[j] ^= rand() & 0xFFFF;

		pDataB16 += 4;
	}
}

// --------------------------------------------------------------

uint8_t sbox[256] = {
	0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76, 0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0, 0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15, 0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75, 0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B,
	0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84, 0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF, 0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8, 0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2, 0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73, 0x60, 0x81, 0x4F, 0xDC,
	0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB, 0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79, 0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08, 0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A, 0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1,
	0x1D, 0x9E, 0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF, 0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16
};

uint8_t Rcon[11] = { 0x8D, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1B, 0x36 };

#define Nk 4
#define Nb 4
#define Nr 10

#define AES_KEY_EXP_SIZE 176
#define AES_KEY_SIZE     16
#define AES_IV_SIZE      16
#define AES_BLOCKLEN     16

#define AES_ROUND_KEY_SIZE AES_KEY_EXP_SIZE + AES_IV_SIZE

#define PW_SIZE 15

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
		tempa[0]   = pRoundKey[k + 0];
		tempa[1]   = pRoundKey[k + 1];
		tempa[2]   = pRoundKey[k + 2];
		tempa[3]   = pRoundKey[k + 3];

		if ((i % Nk) == 0)
		{
			const uint8_t u8tmp = tempa[0];
			tempa[0]            = tempa[1];
			tempa[1]            = tempa[2];
			tempa[2]            = tempa[3];
			tempa[3]            = u8tmp;

			tempa[0] = sbox[tempa[0]] ^ Rcon[i / Nk];
			tempa[1] = sbox[tempa[1]] >> 4;
			tempa[2] = ~sbox[tempa[2]];
			tempa[3] = std::rotr(sbox[tempa[3]], 7);
		}

		uint32_t j = i * 4;
		k          = (i - Nk) * 4;

		pRoundKey[j + 0] = pRoundKey[k + 0] ^ tempa[0];
		pRoundKey[j + 1] = pRoundKey[k + 1] ^ tempa[1];
		pRoundKey[j + 2] = pRoundKey[k + 2] ^ tempa[2];
		pRoundKey[j + 3] = pRoundKey[k + 3] ^ tempa[3];
	}
}

void initAES128(uint8_t *pRoundKey, uint8_t *pPw)
{
	uint8_t key[AES_KEY_SIZE] = { 0 };
	uint8_t iv[AES_IV_SIZE]   = { 0 };

	for (int32_t i = 0; i < PW_SIZE; i++)
	{
		key[i] ^= pPw[(i * 7) % 0xF] + i * i;
		iv[i] ^= pPw[(i * 11) % 0xF] - i * i;
	}

	for (uint32_t i = 0; i < PW_SIZE; i++)
	{
		key[PW_SIZE] ^= pPw[i] + (i * 3);
		iv[PW_SIZE] ^= pPw[i] + (i * 5);
	}

	keyExpansion(pRoundKey, key);
	std::memcpy(pRoundKey + AES_KEY_EXP_SIZE, iv, AES_IV_SIZE);
}

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

char xtime(uint8_t x)
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
void aesCtrXCrypt(int8_t *pData, uint8_t *pKey, const uint32_t &size)
{
	uint8_t state[AES_BLOCKLEN];
	uint8_t *pIv = pKey + AES_KEY_EXP_SIZE;
	int32_t bi   = AES_BLOCKLEN;

	for (uint32_t i = 0; i < size; i++, bi++)
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
