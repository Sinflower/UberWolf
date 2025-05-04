/*
 *  File: Types.h
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

#include <string>
#include <vector>
#include <windows.h>

// Check MSVC
#if _WIN32 || _WIN64
#if _WIN64
#define BIT_64
#else
#define BIT_32
#endif
#endif

// Check GCC
#if __GNUC__
#if __x86_64__ || __ppc64__
#define BIT_64
#else
#define BIT_32
#endif
#endif

using Bytes    = std::vector<uint8_t>;
using uInts    = std::vector<uint32_t>;
using tString  = std::wstring;
using tStrings = std::vector<tString>;

#define DISABLE_COPY_MOVE(T)             \
	T(T const &)               = delete; \
	void operator=(T const &t) = delete; \
	T(T &&)                    = delete;

enum class WolfFileType
{
	GameDat,
	CommonEvent,
	DataBase,
	Project,
	Map,
	TileSetData,
	None
};
