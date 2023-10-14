#pragma once

#include <Windows.h>
#include <syelog.h>

////////////////////////////////////////////////////////////// Logging System.
//
static BOOL s_bLog = 1;

VOID _Print(const CHAR* psz, ...)
{
	DWORD dwErr = GetLastError();

	if (s_bLog && psz)
	{
		CHAR szBuf[1024];
		PCHAR pszBuf = szBuf;
		PCHAR pszEnd = szBuf + ARRAYSIZE(szBuf) - 1;

		va_list args;
		va_start(args, psz);

		// Copy characters.
		while ((*pszBuf++ = *psz++) != 0 && pszBuf < pszEnd);

		*pszEnd = '\0';
		SyelogV(SYELOG_SEVERITY_INFORMATION, szBuf, args);

		va_end(args);
	}

	SetLastError(dwErr);
}

#define _PrintEnter _Print
#define _PrintExit _Print

//
//////////////////////////////////////////////////////////////////////////////
