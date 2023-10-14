#pragma once

#include <Windows.h>

#include "Types.h"

static inline std::string WStringToString(const std::wstring& wstr)
{
	std::string str;
	std::size_t size;
	str.resize(wstr.length());
	wcstombs_s(&size, &str[0], str.size() + 1, wstr.c_str(), wstr.size());
	return str;
}

static inline tStrings argvToList(int argc, char* argv[])
{
	tStrings args;
#if UNICODE || _UNICODE
	int32_t nArgs;
	LPWSTR* pWargv = CommandLineToArgvW(GetCommandLineW(), &nArgs);
	for (int32_t i = 0; i < nArgs; i++)
		args.push_back(pWargv[i]);

	LocalFree(pWargv);
#else
	for (int32_t i = 0; i < argc; i++)
		args.push_back(argv[i]);
#endif

	return args;
}

static inline std::string ByteToHexString(const uint8_t& byte)
{
	char hex[3];
	sprintf_s(hex, "%02X", byte);
	return std::string(hex);
}
