/*
 *  File: WolfRng.hpp
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

namespace wolf::crypt::rng
{

struct RngData
{
	static constexpr uint32_t OUTER_VEC_LEN = 0x20;
	static constexpr uint32_t INNER_VEC_LEN = 0x100;
	static constexpr uint32_t DATA_VEC_LEN  = 0x30;

	uint32_t seed1   = 0;
	uint32_t seed2   = 0;
	uint32_t counter = 0;

	std::array<std::array<uint32_t, INNER_VEC_LEN>, OUTER_VEC_LEN> data = {};

	void Reset()
	{
		seed1   = 0;
		seed2   = 0;
		counter = 0;

		for (auto &outer : data)
			std::fill(outer.begin(), outer.end(), 0);
	}
};

inline uint32_t customRng1(RngData &rd)
{
	uint32_t state;
	uint32_t stateMod;

	const uint32_t seed1 = rd.seed1;

	const uint32_t seedP1 = (seed1 ^ (((seed1 << 11) ^ seed1) >> 8));
	const uint32_t seed   = (seed1 << 11) ^ seedP1;

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

inline uint32_t customRng2(RngData &rd)
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

inline uint32_t customRng3(RngData &rd)
{
	uint32_t state;
	const uint32_t seed = rd.seed2;

	state = (1566083941 * seed) ^ (292331520 * seed);
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

inline void rngChain(RngData &rd, std::array<uint32_t, RngData::INNER_VEC_LEN> &data)
{
	uint32_t i = 0;
	for (uint32_t &d : data)
	{
		uint32_t rn = customRng2(rd);

		d = rn ^ customRng3(rd);

		if ((++rd.counter & 1) == 0)
			d += customRng3(rd);

		const uint32_t c = rd.counter;

		if (!(c % 3))
			d ^= customRng1(rd) + 3;

		if (!(c % 7))
			d += customRng3(rd) + 1;

		if ((c & 7) == 0)
			d *= customRng1(rd);

		if (!((i + rd.seed1) % 5))
			d ^= customRng1(rd);

		if (!(c % 9))
			d += customRng2(rd) + 4;

		if (!(c % 0x18))
			d += customRng2(rd) + 7;

		if (!(c % 0x1F))
			d += 3 * customRng3(rd);

		if (!(c % 0x3D))
			d += customRng3(rd) + 1;

		if (!(c % 0xA1))
			d += customRng2(rd);

		if (static_cast<uint16_t>(rn) == 256)
			d += 3 * customRng3(rd);

		i++;
	}
}

inline void aLotOfRngStuff(RngData &rd, uint32_t a2, uint32_t a3, const uint32_t &idx, std::array<uint8_t, RngData::DATA_VEC_LEN> &cryptData)
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

} // namespace wolf::crypt::rng