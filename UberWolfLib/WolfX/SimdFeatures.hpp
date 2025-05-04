/*
 *  File: SimdFeatures.hpp
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

#include <bitset>
#include <intrin.h>
#include <iostream>

namespace simd
{

struct CpuFeatures
{
	bool sse      = false;
	bool sse2     = false;
	bool sse3     = false;
	bool ssse3    = false;
	bool sse4_1   = false;
	bool sse4_2   = false;
	bool avx      = false;
	bool avx2     = false;
	bool avx512f  = false;
	bool avx512dq = false;
	bool avx512cd = false;
	bool avx512bw = false;
	bool avx512vl = false;

	void print(std::ostream& out = std::cout) const
	{
		auto yesno = [](bool val) -> const char* { return val ? "Yes" : "No"; };
		out << "SSE:       " << yesno(sse) << std::endl;
		out << "SSE2:      " << yesno(sse2) << std::endl;
		out << "SSE3:      " << yesno(sse3) << std::endl;
		out << "SSSE3:     " << yesno(ssse3) << std::endl;
		out << "SSE4.1:    " << yesno(sse4_1) << std::endl;
		out << "SSE4.2:    " << yesno(sse4_2) << std::endl;
		out << "AVX:       " << yesno(avx) << std::endl;
		out << "AVX2:      " << yesno(avx2) << std::endl;
		out << "AVX-512F:  " << yesno(avx512f) << std::endl;
		out << "AVX-512DQ: " << yesno(avx512dq) << std::endl;
		out << "AVX-512CD: " << yesno(avx512cd) << std::endl;
		out << "AVX-512BW: " << yesno(avx512bw) << std::endl;
		out << "AVX-512VL: " << yesno(avx512vl) << std::endl;
	}
};

[[nodiscard]]
inline int cpuidMaxLeaf()
{
	int info[4];
	__cpuid(info, 0);
	return info[0];
}

[[nodiscard]]
inline bool osSupportsYmm()
{
	return (_xgetbv(0) & 0x6) == 0x6;
}

[[nodiscard]]
inline bool osSupportsZmm()
{
	return (_xgetbv(0) & 0xE6) == 0xE6;
}

[[nodiscard]]
inline CpuFeatures detectCpuFeatures()
{
	CpuFeatures features{};
	int cpuInfo[4];

	if (cpuidMaxLeaf() < 1) return features;

	__cpuid(cpuInfo, 1);
	std::bitset<32> edx(cpuInfo[3]);
	std::bitset<32> ecx(cpuInfo[2]);

	features.sse    = edx.test(25);
	features.sse2   = edx.test(26);
	features.sse3   = ecx.test(0);
	features.ssse3  = ecx.test(9);
	features.sse4_1 = ecx.test(19);
	features.sse4_2 = ecx.test(20);

	bool osxsave       = ecx.test(27);
	bool avx_supported = ecx.test(28);

	if (osxsave && avx_supported && osSupportsYmm())
	{
		features.avx = true;

		if (cpuidMaxLeaf() >= 7)
		{
			__cpuidex(cpuInfo, 7, 0);
			std::bitset<32> ebx(cpuInfo[1]);

			features.avx2     = ebx.test(5);
			features.avx512f  = ebx.test(16);
			features.avx512dq = ebx.test(17);
			features.avx512cd = ebx.test(28);
			features.avx512bw = ebx.test(30);
			features.avx512vl = ebx.test(31);

			if (!osSupportsZmm())
			{
				features.avx512f  = false;
				features.avx512dq = false;
				features.avx512cd = false;
				features.avx512bw = false;
				features.avx512vl = false;
			}
		}
	}

	return features;
}

} // namespace simd