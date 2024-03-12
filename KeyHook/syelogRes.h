/*
 *  File: syelogRes.h
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
