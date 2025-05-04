/*
 *  File: GeneratorDetail.hpp
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
#include <string>
#include <vector>

#include "../Types.hpp"

namespace wolfx::detail::generator
{
inline DecryptBlob generateWolfxDecryptBlob(uint32_t seed, const StaticBlob &staticBlob, const std::size_t &fileSize)
{
	for (uint8_t byte : staticBlob)
		seed ^= byte;

	std::array<uint8_t, 256> decryptionKey;
	for (uint8_t &byte : decryptionKey)
	{
		seed = seed * 1664525 + 1013904223;
		byte = static_cast<uint8_t>(seed);
	}

	return decryptionKey;
}

inline StaticBlob generateWolfxStaticBlob(const WolfXKeyData &key = {})
{
	StaticBlob data;
	data.fill(0xAA);

	uint8_t dynamicVal = 0xBE;

	for (std::size_t i = 0; i < key.size(); i++)
	{
		std::size_t index = i % data.size();
		data[index] ^= key[i];
		data[index] += dynamicVal;
		data[index] = static_cast<uint8_t>((data[index] << 3) | (data[index] >> 5));
		dynamicVal  = key[i] ^ (0xB3 * dynamicVal);
	}

	for (uint32_t round = 0; round < 5; round++)
	{
		for (std::size_t j = 0; j < data.size(); j++)
		{
			uint8_t prev = data[(j + 13) % data.size()];
			uint8_t mix  = data[j] ^ data[(j + 7) % data.size()];
			data[j]      = std::rotr<uint8_t>(prev + mix, 7);
		}
	}

	for (uint8_t &byte : data)
	{
		if (byte == 0)
			byte = 1;
	}

	return data;
}

inline StaticBlob generateWolfxStaticBlob(const std::string &key)
{
	return generateWolfxStaticBlob(WolfXKeyData(key.begin(), key.end()));
}

// Implementation of the Fowler–Noll–Vo hash function
// Based on: https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
template<typename Itr>
inline uint32_t fnv1(const Itr &begin, const Itr &end)
{
	uint32_t hash        = 0x811C9DC5;
	const uint32_t prime = 0x01000193;

	for (Itr it = begin; it != end; it++)
		hash = prime * (hash ^ *it);

	return hash;
}

inline uint32_t fnv1(const std::vector<uint8_t> &data)
{
	return fnv1(data.begin(), data.end());
}

template<std::size_t N>
inline uint32_t fnv1(const std::array<uint8_t, N> &data)
{
	return fnv1(data.begin(), data.end());
}

inline uint32_t fnv1(const std::string &str)
{
	return fnv1(std::vector<uint8_t>(str.begin(), str.end()));
}
} // namespace wolfx::detail::generator