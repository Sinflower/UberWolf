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


//////////////////////////////////////////////////////////////////////// main.
//
WolfKeyFinder::WolfKeyFinder(const tString& exePath)
	: m_exePath(exePath)
{
	SyeLog::init();
	SyeLog::registerKeyCallback([this](Key& key, const bool& useOldDxArc) { KeyCallback(key, useOldDxArc); });
}

bool WolfKeyFinder::Inject()
{
	// Make sure the exe exists
	if (!fs::exists(m_exePath))
	{
		tcerr << __func__ << TEXT(": Error: ") << m_exePath << TEXT(" does not exist.") << std::endl;
		return false;
	}

	// Make sure the dll exists
	if (!fs::exists(DLL_NAME))
	{
		tcerr << __func__ << TEXT(": Error: ") << DLL_NAME << TEXT(" does not exist.") << std::endl;
		return false;
	}


	/////////////////////////////////////////////////////////// Validate DLL
	//

	TCHAR szBuffer[1024];
	TCHAR* pszFilePart = nullptr;

	if (!GetFullPathName(DLL_NAME.c_str(), ARRAYSIZE(szBuffer), szBuffer, &pszFilePart))
	{
		tcerr << __func__ << TEXT(": Error: ") << DLL_NAME << TEXT(" is not a valid path name...") << std::endl;
		return false;
	}

	tString dllPath = szBuffer;

	HMODULE hDll = LoadLibraryEx(dllPath.c_str(), NULL, DONT_RESOLVE_DLL_REFERENCES);
	if (hDll == NULL)
	{
		tcerr << __func__ << TEXT(": Error: ") << dllPath << TEXT(" failed to load (error ") << GetLastError() << TEXT(").") << std::endl;
		return false;
	}

	ExportContext ec;
	ec.fHasOrdinal1 = false;
	ec.nExports = 0;
	DetourEnumerateExports(hDll, &ec, ExportCallback);
	FreeLibrary(hDll);

	if (!ec.fHasOrdinal1)
	{
		tcerr << __func__ << TEXT(": Error: ") << dllPath << TEXT(" does not export ordinal #1.") << std::endl;
		tcerr << TEXT("             See help entry DetourCreateProcessWithDllEx in Detours.chm.") << std::endl;
		return false;
	}

	//////////////////////////////////////////////////////////////////////////

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&pi, sizeof(pi));
	si.cb = sizeof(si);

#ifdef PRINT_DEBUG
	tcout << __func__ << TEXT(": Starting: '") << m_exePath << TEXT("'") << std::endl;
	tcout << __func__ << TEXT(":   with '") << dllPath << TEXT("'") << std::endl;
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
		tcerr << __func__ << TEXT(": DetourCreateProcessWithDllEx failed: ") << dwError << std::endl;

		if (dwError == ERROR_INVALID_HANDLE)
		{
#if DETOURS_64BIT
			tcerr << __func__ << TEXT(": Can't detour a 32-bit target process from a 64-bit parent process.") << std::endl;
#else
			tcerr << __func__ << TEXT(": Can't detour a 64-bit target process from a 32-bit parent process.") << std::endl;
#endif
		}

		return false;
	}

	ResumeThread(pi.hThread);

	WaitForSingleObject(pi.hProcess, INFINITE);

	DWORD dwResult = 0;
	if (!GetExitCodeProcess(pi.hProcess, &dwResult))
	{
		tcerr << __func__ << TEXT(": GetExitCodeProcess failed: ") << GetLastError() << std::endl;
		return false;
	}

	return (dwResult == 0);
}

//
///////////////////////////////////////////////////////////////// End of File.
