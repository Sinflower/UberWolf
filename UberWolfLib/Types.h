/*
 *  File: Types.h
 *  Copyright (c) 2023 Sinflower
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
#include <functional>
#include <iostream>
#include <ostream>
#include <sstream>
#include <tchar.h>
#include <vector>

static inline std::basic_ostream<TCHAR>& tcout =
#ifdef _UNICODE
	std::wcout;
#else
	std::cout;
#endif // _UNICODE

static inline std::basic_ostream<TCHAR>& tcerr =
#ifdef _UNICODE
	std::wcerr;
#else
	std::cerr;
#endif // _UNICODE

#ifdef _UNICODE
using tString       = std::wstring;
using tOstream      = std::wostream;
using tStringStream = std::wstringstream;
#define FS_PATH_TO_TSTRING(P) P.wstring()
#else
using tString       = std::string;
using tOstream      = std::ostream;
using tStringStream = std::stringstream;
#define FS_PATH_TO_TSTRING(P) P.string()
#endif // _UNICODE

using tStrings = std::vector<tString>;
using Strings  = std::vector<std::string>;
using Key      = std::vector<uint8_t>;

using LogCallback = std::function<void(const tString&, const bool&)>;

using LogCallbacks = std::vector<LogCallback>;

using LocalizerQuery = std::function<const tString&(const std::string&)>;