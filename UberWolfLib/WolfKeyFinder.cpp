/*
 *  File: WolfKeyFinder.cpp
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

#include "WolfKeyFinder.h"

#include <windows.h>

// Needs to be included after windows.h
#include <detours.h>

#pragma warning(push)
#if _MSC_VER > 1400
#pragma warning(disable : 6102 6103) // /analyze warnings
#endif
#include <strsafe.h>
#pragma warning(pop)

#include <filesystem>
#include <format>
#include <iostream>
#include <string>

#include "SyeLog.h"
#include "UberLog.h"
#include "Utils.h"

namespace fs = std::filesystem;

const tString WolfKeyFinder::DLL_NAME = TEXT("KeyHook.dll");

//////////////////////////////////////////////////////////////////////////////
//
//  This code verifies that the named DLL has been configured correctly
//  to be imported into the target process.  DLLs must export a function with
//  ordinal #1 so that the import table touch-up magic works.
//
struct ExportContext
{
	bool fHasOrdinal1;
	uint64_t nExports;
};

static BOOL CALLBACK ExportCallback([[maybe_unused]] _In_opt_ PVOID pContext, _In_ ULONG nOrdinal, [[maybe_unused]] _In_opt_ LPCSTR pszSymbol, [[maybe_unused]] _In_opt_ PVOID pbTarget)
{
	ExportContext* pec = reinterpret_cast<ExportContext*>(pContext);

	if (pec == nullptr)
		return FALSE;

	if (nOrdinal == 1)
		pec->fHasOrdinal1 = true;

	pec->nExports++;

	return TRUE;
}

//
//////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
//
WolfKeyFinder::WolfKeyFinder(const tString& exePath) :
	m_exePath(exePath)
{
	SyeLog::init();
	SyeLog::registerKeyCallback([this](Key& key, const bool& useOldDxArc) { KeyCallback(key, useOldDxArc); });
}

WolfKeyFinder::~WolfKeyFinder()
{
	SyeLog::clearKeyCallbacks();
}

bool WolfKeyFinder::Inject(const tString& dllFolder)
{
	tString dllPath = dllFolder + TEXT("/") + DLL_NAME;
	// Make sure the exe exists
	if (!fs::exists(m_exePath))
	{
		ERROR_LOG << std::format(TEXT("Error: {} does not exist."), m_exePath) << std::endl;
		return false;
	}

	// Make sure the dll exists
	if (!fs::exists(dllPath))
	{
		ERROR_LOG << std::format(TEXT("Error: {} does not exist."), dllPath) << std::endl;
		return false;
	}

	TCHAR szBuffer[1024];
	TCHAR* pszFilePart = nullptr;

	if (!GetFullPathName(dllPath.c_str(), ARRAYSIZE(szBuffer), szBuffer, &pszFilePart))
	{
		ERROR_LOG << std::format(TEXT("Error: {} is not a valid path name."), dllPath) << std::endl;
		return false;
	}

	dllPath = szBuffer;

	HMODULE hDll = LoadLibraryEx(dllPath.c_str(), NULL, DONT_RESOLVE_DLL_REFERENCES);
	if (hDll == NULL)
	{
		ERROR_LOG << std::format(TEXT("Error: {} failed to load (error {})."), dllPath, GetLastError()) << std::endl;
		return false;
	}

	ExportContext ec;
	ec.fHasOrdinal1 = false;
	ec.nExports     = 0;
	DetourEnumerateExports(hDll, &ec, ExportCallback);
	FreeLibrary(hDll);

	if (!ec.fHasOrdinal1)
	{
		ERROR_LOG << std::format(TEXT("Error: {} does not export ordinal #1."), dllPath) << std::endl;
		return false;
	}

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&pi, sizeof(pi));
	si.cb = sizeof(si);

#ifdef PRINT_DEBUG
	INFO_LOG << std::format(TEXT(": Starting: '{}'"), m_exePath) << std::endl;
	INFO_LOG << std::format(TEXT(":   with '{}'"), dllPath) << std::endl;
#endif

	DWORD dwFlags = CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED;

	memset(szBuffer, 0, ARRAYSIZE(szBuffer));

	SetLastError(0);
	SearchPath(NULL, m_exePath.c_str(), TEXT(".exe"), ARRAYSIZE(szBuffer), szBuffer, NULL);

	// Can't use wstring here because DetourCreateProcessWithDlls only accepts LPCSTR due to internal problems
	// See: https://stackoverflow.com/questions/76302154/
	const std::string dllPathStr = WStringToString(dllPath).c_str();
	LPCSTR lpszDllPath           = dllPathStr.c_str();

	if (!DetourCreateProcessWithDlls(szBuffer, NULL, NULL, NULL, TRUE, dwFlags, NULL, NULL, &si, &pi, 1, &lpszDllPath, NULL))
	{
		DWORD dwError = GetLastError();
		ERROR_LOG << std::format(TEXT("DetourCreateProcessWithDlls failed: {}"), dwError) << std::endl;

		if (dwError == ERROR_INVALID_HANDLE)
		{
#if DETOURS_64BIT
			ERROR_LOG << TEXT("Can't detour a 32-bit target process from a 64-bit parent process.") << std::endl;
#else
			ERROR_LOG << TEXT("Can't detour a 64-bit target process from a 32-bit parent process.") << std::endl;
#endif
		}

		return false;
	}

	ResumeThread(pi.hThread);

	WaitForSingleObject(pi.hProcess, INFINITE);

	DWORD dwResult = 0;
	if (!GetExitCodeProcess(pi.hProcess, &dwResult))
	{
		ERROR_LOG << std::format(TEXT("GetExitCodeProcess failed: {}"), GetLastError()) << std::endl;
		return false;
	}

	return (dwResult == 0);
}

//
/////////////////////////////////////////////////////////////////
