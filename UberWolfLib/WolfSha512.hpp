/*
 *  File: WolfSha512.hpp
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
#include <bit>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// SHA-512 implementation with some WolfRPG specific changes
// Based on: https://github.com/pr0f3ss/SHA
namespace wolf::sha512
{
// SHA-512 macros with s512w_ (SHA-512 Wolf) prefix
#define s512w_Ch(x, y, z)  ((x & y) ^ (~x & z))
#define s512w_Maj(x, y, z) ((x & y) ^ (x & z) ^ (y & z))
#define s512w_RotR(x, n)   ((x >> n) | (x << ((sizeof(x) << 3) - n)))
#define s512w_Sig0(x)      ((s512w_RotR(x, 28)) ^ (s512w_RotR(x, 34)) ^ (s512w_RotR(x, 39)))
#define s512w_Sig1(x)      ((s512w_RotR(x, 14)) ^ (s512w_RotR(x, 18)) ^ (s512w_RotR(x, 41)))
#define s512w_sig0(x)      (s512w_RotR(x, 1) ^ s512w_RotR(x, 8) ^ (x >> 7))
#define s512w_sig1(x)      (s512w_RotR(x, 19) ^ s512w_RotR(x, 61) ^ (x >> 6))

// SHA-512 constants
static const uint32_t SEQUENCE_LEN         = 16;
static const uint32_t WORKING_VAR_LEN      = 8;
static const uint32_t MESSAGE_SCHEDULE_LEN = 80;
static const uint32_t MESSAGE_BLOCK_SIZE   = 1024;
static const uint32_t CHAR_LEN_BITS        = 8;
static const uint32_t OUTPUT_LEN           = 8;
static const uint32_t WORD_LEN             = 8;

// NOTE: Custom Wolf specific primes
static const uint64_t hPrime[8] = {
	0x123456789ABCDEF0ULL, 0xFEDCBA9876543210ULL, 0x0F1E2D3C4B5A6978ULL, 0x89ABCDEF01234567ULL,
	0x13579BDF02468ACEULL, 0xF0E1D2C3B4A59687ULL, 0x5A6B7C8D9E0F1A2BULL, 0x1A2B3C4D5E6F7890ULL
};

// Original SHA-512 k values
static const uint64_t k[80] = {
	0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL, 0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL, 0x3956c25bf348b538ULL,
	0x59f111f1b605d019ULL, 0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL, 0xd807aa98a3030242ULL, 0x12835b0145706fbeULL,
	0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL, 0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL, 0x9bdc06a725c71235ULL,
	0xc19bf174cf692694ULL, 0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL, 0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
	0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL, 0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL, 0x983e5152ee66dfabULL,
	0xa831c66d2db43210ULL, 0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL, 0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL,
	0x06ca6351e003826fULL, 0x142929670a0e6e70ULL, 0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL, 0x4d2c6dfc5ac42aedULL,
	0x53380d139d95b3dfULL, 0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL, 0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
	0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL, 0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL, 0xd192e819d6ef5218ULL,
	0xd69906245565a910ULL, 0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL, 0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL,
	0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL, 0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL, 0x5b9cca4f7763e373ULL,
	0x682e6ff3d6b2b8a3ULL, 0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL, 0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
	0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL, 0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL, 0xca273eceea26619cULL,
	0xd186b8c721c0c207ULL, 0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL, 0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL,
	0x113f9804bef90daeULL, 0x1b710b35131c471bULL, 0x28db77f523047d84ULL, 0x32caab7b40c72493ULL, 0x3c9ebe0a15c9bebcULL,
	0x431d67c49c100d4cULL, 0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL, 0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL
};

using s512Hash    = std::array<uint64_t, OUTPUT_LEN>;
using s512Input   = std::vector<uint64_t>;
using s512DynSalt = std::array<uint8_t, 4>;
using s512Pwd     = std::vector<char>;

inline std::string digest(const s512Hash &hash)
{
	std::stringstream ss;
	for (const uint64_t &h : hash)
		ss << std::hex << std::setw(16) << std::setfill('0') << h;

	return ss.str();
}

inline s512Input preprocess(const s512Pwd &pwd, uint64_t &nBuffer)
{
	uint64_t len = static_cast<uint64_t>(pwd.size());
	uint64_t l   = len * CHAR_LEN_BITS;

	nBuffer = ((895 - l) % MESSAGE_BLOCK_SIZE + l + 129) / MESSAGE_BLOCK_SIZE;

	s512Input buffer(nBuffer * SEQUENCE_LEN, 0);

	uint32_t index = 0;

	for (uint64_t i = 0; i < nBuffer * SEQUENCE_LEN; i++)
	{
		uint64_t chunk = 0;
		for (uint32_t j = 0; j < WORD_LEN; j++)
		{
			chunk <<= 8;
			if (index < len)
				chunk |= static_cast<uint8_t>(pwd[index]);
			else if (index == len)
				chunk |= 0x80;

			index++;
		}

		buffer[i] = chunk;
	}

	// Append the length to the last two 64-bit blocks
	buffer[buffer.size() - 2] = 0x0ULL;
	buffer[buffer.size() - 1] = l;

	return buffer;
}

inline s512Hash process(const s512Input input, const uint64_t &nBuffer)
{
	uint64_t s[WORKING_VAR_LEN]      = { 0 };
	uint64_t w[MESSAGE_SCHEDULE_LEN] = { 0 };

	s512Hash result;
	uint64_t *const h = reinterpret_cast<uint64_t *>(result.data());

	std::memcpy(h, hPrime, WORKING_VAR_LEN * sizeof(uint64_t));

	const uint64_t *pInput = input.data();

	for (uint64_t i = 0; i < nBuffer; i++)
	{
		std::memcpy(w, pInput, SEQUENCE_LEN * sizeof(uint64_t));

		for (uint32_t j = 16; j < MESSAGE_SCHEDULE_LEN; j++)
			w[j] = w[j - 16] + s512w_sig0(w[j - 15]) + w[j - 7] + s512w_sig1(w[j - 2]);

		std::memcpy(s, h, WORKING_VAR_LEN * sizeof(uint64_t));

		for (uint32_t j = 0; j < MESSAGE_SCHEDULE_LEN; j++)
		{
			// NOTE: The xor with (s[4] >> 3) is a custom change made to the original code
			uint64_t temp1 = s[7] + s512w_Sig1(s[4]) + ((s[4] >> 3) ^ s512w_Ch(s[4], s[5], s[6])) + k[j] + w[j];
			uint64_t temp2 = s512w_Sig0(s[0]) + s512w_Maj(s[0], s[1], s[2]);

			s[7] = s[6];
			s[6] = s[5];
			s[5] = s[4];
			s[4] = s[3] + temp1;
			s[3] = s[2];
			s[2] = s[1];
			s[1] = s[0];
			s[0] = temp1 + temp2;
		}

		// NOTE: The xor with 0x123456789ABCDEF0 is a custom change made to the original code
		for (uint32_t i = 0; i < WORKING_VAR_LEN; i++)
			h[i] += s[i] ^ 0x123456789ABCDEF0;

		pInput += SEQUENCE_LEN;
	}

	return result;
}

inline s512DynSalt calcDynSalt(const std::vector<uint8_t>& data)
{
	if (data.size() <= 0x10)
		throw std::runtime_error("Invalid data size");

	const uint8_t d0 = data[7];
	const uint8_t d1 = data[11];
	const uint8_t d2 = data[13];

	uint8_t r0 = (d0 + 2 * d1) % 0xF6;
	uint8_t r1 = d2 ^ data[14];
	uint8_t r2 = d0 ^ data[12];
	uint8_t r3 = d0 + d2 - d1;

	s512DynSalt res = { r0, r1, r2, r3 };

	// Make sure that the values are not 0 as that would result in the being treated as null terminator
	for (auto& c : res)
	{
		if (c == 0) c = 1;
	}

	return res;
}

inline s512Pwd saltPassword(const std::string &pwd, const s512DynSalt &dynSalt, const std::string &staticSalt)
{
	// The Salted password contains a dynamic (dynSalt) and a static ("basicD1") salt and looks like this:
	// <pwd><dynSalt><staticSalt>
	std::vector<char> saltedPwd(pwd.begin(), pwd.end());
	saltedPwd.insert(saltedPwd.end(), dynSalt.begin(), dynSalt.end());
	saltedPwd.insert(saltedPwd.end(), staticSalt.begin(), staticSalt.end());

	return saltedPwd;
}
} // namespace wolf::sha512