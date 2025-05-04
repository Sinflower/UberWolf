/*
 *  File: DataManipDetail.hpp
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
#include <cstdint>
#include <immintrin.h>
#include <vector>

#include "../SimdFeatures.hpp"
#include "../Types.hpp"

namespace wolfx::detail::dataManip
{
// --- SIMD Implementations ---

inline void xorBufferBlobPlain(const WolfXData &inBuffer, const DecryptBlob &decryptBlob, WolfXData &outBuffer)
{
	for (std::size_t i = 15; i < inBuffer.size(); i++)
		outBuffer[i] = inBuffer[i] ^ decryptBlob[(i - 10) % decryptBlob.size()];
}

inline void xorBufferBlobSSE2(const WolfXData &inBuffer, const DecryptBlob &decryptBlob, WolfXData &outBuffer)
{
	constexpr std::size_t simd_width = 16;

	std::size_t i              = 15;
	const uint8_t *pBlob       = decryptBlob.data();
	const std::size_t blobSize = decryptBlob.size();
	const std::size_t size     = inBuffer.size();
	const uint8_t *pBuffer     = inBuffer.data();

	uint8_t *pOutBuffer = outBuffer.data();

	// Due to i starting at 15 and the decryptBlob starting at i - 10, i.e, 5, the first 11 (16 - 5) bytes have to be processed without SIMD to avoid missalignment
	for (; ((i - 10) % simd_width) != 0 && i < size; i++)
		pOutBuffer[i] = pBuffer[i] ^ pBlob[(i - 10) % blobSize];

	for (; i + simd_width <= size; i += simd_width)
	{
		__m128i data      = _mm_load_si128(reinterpret_cast<const __m128i *>(pBuffer + i));
		__m128i blobChunk = _mm_loadu_si128(reinterpret_cast<const __m128i *>(pBlob + ((i - 10) % blobSize)));
		data              = _mm_xor_si128(data, blobChunk);
		_mm_store_si128(reinterpret_cast<__m128i *>(pOutBuffer + i), data);
	}

	// Fallback for remaining bytes
	for (; i < size; i++)
		pOutBuffer[i] = pBuffer[i] ^ pBlob[(i - 10) % blobSize];
}

inline void xorBufferBlobAVX2(const WolfXData &inBuffer, const DecryptBlob &decryptBlob, WolfXData &outBuffer)
{
	constexpr std::size_t simd_width = 32;

	std::size_t i              = 15;
	const uint8_t *pBlob       = decryptBlob.data();
	const std::size_t blobSize = decryptBlob.size();
	const std::size_t size     = inBuffer.size();
	const uint8_t *pBuffer     = inBuffer.data();

	uint8_t *pOutBuffer = outBuffer.data();

	// Due to i starting at 15 and the decryptBlob starting at i - 10, i.e, 5, the first 27 (32 - 5) bytes have to be processed without SIMD to avoid missalignment
	for (; ((i - 10) % simd_width) != 0 && i < size; i++)
		pOutBuffer[i] = pBuffer[i] ^ pBlob[(i - 10) % blobSize];

	for (; i + simd_width <= size; i += simd_width)
	{
		__m256i data      = _mm256_load_si256(reinterpret_cast<const __m256i *>(pBuffer + i));
		__m256i blobChunk = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(pBlob + ((i - 10) % blobSize)));
		data              = _mm256_xor_si256(data, blobChunk);
		_mm256_store_si256(reinterpret_cast<__m256i *>(pOutBuffer + i), data);
	}

	// Fallback for remaining bytes
	for (; i < size; i++)
		pOutBuffer[i] = pBuffer[i] ^ pBlob[(i - 10) % blobSize];
}

// --- Dispatcher ---

using DecryptFunction = void (*)(const WolfXData &, const DecryptBlob &, WolfXData &);

inline DecryptFunction g_decryptFunc = nullptr;

// Initializes the correct function at runtime
inline void initXorBufferBlobFunc(const simd::CpuFeatures &features)
{
	if (features.avx2)
		g_decryptFunc = xorBufferBlobAVX2;
	else if (features.sse2)
		g_decryptFunc = xorBufferBlobSSE2;
	else
		g_decryptFunc = xorBufferBlobPlain;
}

inline void initXorBufferBlobFunc()
{
	initXorBufferBlobFunc(simd::detectCpuFeatures());
}

inline void xorBufferBlob(const WolfXData &inBuffer, const DecryptBlob &decryptBlob, WolfXData &outBuffer)
{
	if (g_decryptFunc)
		g_decryptFunc(inBuffer, decryptBlob, outBuffer);
	else
		xorBufferBlobPlain(inBuffer, decryptBlob, outBuffer); // Fallback (should not happen if initialized properly)
}

} // namespace wolfx::detail::dataManip