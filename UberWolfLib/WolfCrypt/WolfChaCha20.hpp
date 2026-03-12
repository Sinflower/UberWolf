/*
 *  File: WolfChaCha20.hpp
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
#include <cstdint>

namespace wolf::crypt::chacha20
{
// TODO: Move ChaCha20 into a class
////////////////////////////
// ChaCha20 implementation
// Based on: https://github.com/Ginurx/chacha20-c

inline uint32_t pack4(const uint8_t *a)
{
	uint32_t res = 0;
	res |= (uint32_t)a[0] << 0 * 8;
	res |= (uint32_t)a[1] << 1 * 8;
	res |= (uint32_t)a[2] << 2 * 8;
	res |= (uint32_t)a[3] << 3 * 8;
	return res;
}

inline void initBlock(uint32_t *pState, const uint8_t *pKey, const uint8_t *pNonce)
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

inline uint32_t rotl32(uint32_t x, int n)
{
	return (x << n) | (x >> (32 - n));
}

inline void blockNext(uint32_t *pState, uint32_t *pKeyStream)
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
inline void execute(uint32_t *pState, uint32_t *pKeyStream, const uint32_t &startPos, uint8_t *bytes, const uint64_t &length)
{
	uint8_t *keystream8 = (uint8_t *)pKeyStream;
	uint64_t position   = 0;
	uint64_t offset     = startPos % 64;

	pState[12] += startPos / 64;

	while (position < length)
	{
		uint32_t steps = static_cast<uint32_t>(std::min(64 - offset, length - position));
		blockNext(pState, pKeyStream);

		for (uint32_t i = 0; i < steps; i++)
			bytes[position + i] ^= keystream8[offset + i];

		position += steps;
		offset = 0;
	}
}

inline void keySetup(const std::array<uint8_t, 4> &data, std::array<uint8_t, 64> &key)
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

} // namespace wolf::crypt::chacha20