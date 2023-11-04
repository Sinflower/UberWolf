#include "WolfKeyFinder.h"

#include <windows.h>
#include <detours.h>

#pragma warning(push)
#if _MSC_VER > 1400
#pragma warning(disable:6102 6103) // /analyze warnings
#endif
#include <strsafe.h>
#pragma warning(pop)

#include <iostream>
#include <string>
#include <filesystem>

#include "SyeLog.h"
#include "Utils.h"
#include "UberLog.h"

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
	bool     fHasOrdinal1;
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
WolfKeyFinder::WolfKeyFinder(const tString& exePath)
	: m_exePath(exePath)
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
		ERROR_LOG << TEXT("Error: ") << m_exePath << TEXT(" does not exist.") << std::endl;
		return false;
	}

	// Make sure the dll exists
	if (!fs::exists(dllPath))
	{
		ERROR_LOG << TEXT("Error: ") << dllPath << TEXT(" does not exist.") << std::endl;
		return false;
	}

	TCHAR szBuffer[1024];
	TCHAR* pszFilePart = nullptr;

	if (!GetFullPathName(dllPath.c_str(), ARRAYSIZE(szBuffer), szBuffer, &pszFilePart))
	{
		ERROR_LOG << TEXT("Error: ") << dllPath << TEXT(" is not a valid path name...") << std::endl;
		return false;
	}

	dllPath = szBuffer;

	HMODULE hDll = LoadLibraryEx(dllPath.c_str(), NULL, DONT_RESOLVE_DLL_REFERENCES);
	if (hDll == NULL)
	{
		ERROR_LOG << TEXT("Error: ") << dllPath << TEXT(" failed to load (error ") << GetLastError() << TEXT(").") << std::endl;
		return false;
	}

	ExportContext ec;
	ec.fHasOrdinal1 = false;
	ec.nExports = 0;
	DetourEnumerateExports(hDll, &ec, ExportCallback);
	FreeLibrary(hDll);

	if (!ec.fHasOrdinal1)
	{
		ERROR_LOG << TEXT("Error: ") << dllPath << TEXT(" does not export ordinal #1.") << std::endl;
		return false;
	}

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&pi, sizeof(pi));
	si.cb = sizeof(si);

#ifdef PRINT_DEBUG
	INFO_LOG << TEXT(": Starting: '") << m_exePath << TEXT("'") << std::endl;
	INFO_LOG << TEXT(":   with '") << dllPath << TEXT("'") << std::endl;
#endif

	DWORD dwFlags = CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED;

	memset(szBuffer, 0, ARRAYSIZE(szBuffer));

	SetLastError(0);
	SearchPath(NULL, m_exePath.c_str(), TEXT(".exe"), ARRAYSIZE(szBuffer), szBuffer, NULL);

	// Can't use wstring here because DetourCreateProcessWithDlls only accepts LPCSTR due to internal problems
	// See: https://stackoverflow.com/questions/76302154/
	const std::string dllPathStr = WStringToString(dllPath).c_str();
	LPCSTR lpszDllPath = dllPathStr.c_str();

	if (!DetourCreateProcessWithDlls(szBuffer, NULL, NULL, NULL, TRUE, dwFlags, NULL, NULL, &si, &pi, 1, &lpszDllPath, NULL))
	{
		DWORD dwError = GetLastError();
		ERROR_LOG << TEXT("DetourCreateProcessWithDlls failed: ") << dwError << std::endl;

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
		ERROR_LOG << TEXT("GetExitCodeProcess failed: ") << GetLastError() << std::endl;
		return false;
	}

	return (dwResult == 0);
}

//
/////////////////////////////////////////////////////////////////
