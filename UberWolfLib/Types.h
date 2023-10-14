#pragma once

#include <tchar.h>
#include <vector>
#include <ostream>
#include <iostream>
#include <cstdint>

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
#define FS_PATH_TO_TSTRING(P) P.wstring()
#else
using tString = std::string;
#define FS_PATH_TO_TSTRING(P) P.string()
#endif // _UNICODE

using tStrings = std::vector<tString>;
using Key = std::vector<uint8_t>;
