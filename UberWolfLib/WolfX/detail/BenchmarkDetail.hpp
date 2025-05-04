/*
 *  File: BenchmarkDetail.hpp
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
#include <omp.h>
#include <string>
#include <vector>

#include "../Types.hpp"

#include "CrackDetail.hpp"
#include "DataManipDetail.hpp"
#include "UtilsDetail.hpp"

#define VALIDATE_MODE 0

#if VALIDATE_MODE
#define KEY_COUNT   10
#define VAL_VERBOSE true
#else
#define KEY_COUNT   100000
#define VAL_VERBOSE false
#endif

namespace wolfx::detail::benchmark
{
static std::vector<WolfXData> g_decBuffers;

inline void benchmark(const std::string &filename)
{
	dataManip::initXorBufferBlobFunc();
	const uint32_t NUM_KEYS = KEY_COUNT; // Number of keys to generate

	const std::array<uint8_t, 5> WOLFX_MAGIC = { 0x57, 0x4F, 0x4C, 0x46, 0x58 }; // "WOLFX"

	WolfXData encData = utils::file2Buffer(filename);

	if (encData.size() < 15 || std::memcmp(encData.data(), WOLFX_MAGIC.data(), 5) != 0)
	{
		std::cerr << "Invalid WOLFX file" << std::endl;
		return;
	}

	std::cout << "Initializing test keys ... " << std::flush;

	std::vector<WolfXKeyData> testKeys;
	testKeys.reserve(NUM_KEYS);
	// Generate test keys consisting of 10 random uint8_t values
	srand(static_cast<unsigned int>(time(nullptr))); // Seed the random number generator
	for (uint32_t i = 0; i < NUM_KEYS; i++)
	{
		// WolfXKeyData key(10);
		// std::generate(key.begin(), key.end(), []() { return static_cast<uint8_t>(rand() % 256); });
		// testKeys.push_back(key);

		testKeys.push_back({ 0x74, 0x65, 0x73, 0x74, 0x6B, 0x65, 0x79 });
	}

	std::cout << "Done" << std::endl;

	// Set the number of threads to use to half of the available threads to only use the physical cores
	static uint32_t numThreads = omp_get_max_threads() / 2;
	omp_set_num_threads(numThreads);
	std::cout << "Initializing global encData for " << numThreads << " threads ... " << std::flush;

	g_decBuffers.resize(numThreads);
	for (auto &fb : g_decBuffers)
	{
		fb.resize(encData.size());
		// Copy the first 10 bytes of the encData to the final encData
		std::copy(encData.begin(), encData.begin() + 10, fb.begin());
	}

	std::cout << "Done" << std::endl;

	std::cout << "Testing decryption speed ... " << std::flush;
	auto start = std::chrono::high_resolution_clock::now();

#pragma omp parallel for
	for (int32_t i = 0; i < testKeys.size(); i++)
		detail::crack::benchmarkDecryptFull(encData, testKeys[i], "testkey", 1337, g_decBuffers[omp_get_thread_num()]);

	auto end = std::chrono::high_resolution_clock::now();
	std::cout << "Done" << std::endl;
	std::chrono::duration<double> elapsed = end - start;
	std::cout << "Decryption time: " << elapsed.count() << " seconds" << std::endl;
	std::cout << "Decryptions per second: " << (NUM_KEYS / elapsed.count()) << std::endl;
}

} // namespace wolfx::detail::benchmark