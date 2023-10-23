#pragma once

#include <tchar.h>
#include <vector>
#include <ostream>
#include <iostream>
#include <cstdint>
#include <sstream>
#include <functional>

static inline std::basic_ostream <TCHAR>& tcout =
#ifdef _UNICODE
std::wcout;
#else
std::cout;
#endif // _UNICODE

static inline std::basic_ostream <TCHAR>& tcerr =
#ifdef _UNICODE
std::wcerr;
#else
std::cerr;
#endif // _UNICODE


#ifdef _UNICODE
using tString = std::wstring;
using tOstream = std::wostream;
using tStringStream = std::wstringstream;
#define FS_PATH_TO_TSTRING(P) P.wstring()
#else
using tString = std::string;
using tOstream = std::ostream;
using tStringStream = std::stringstream;
#define FS_PATH_TO_TSTRING(P) P.string()
#endif // _UNICODE

using tStrings = std::vector<tString>;
using Key = std::vector<uint8_t>;

using LogCallback = std::function<void(const tString&, const bool&)>;

using LogCallbacks = std::vector<LogCallback>;