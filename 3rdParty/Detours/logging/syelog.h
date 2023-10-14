//////////////////////////////////////////////////////////////////////////////
//
//  Detours Test Program (syelog.h of syelog.lib)
//
//  Microsoft Research Detours Package
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
#pragma once
#ifndef _SYELOGD_H_
#define _SYELOGD_H_
#include <stdarg.h>
#include <vector>

#pragma pack(push, 1)
#pragma warning(push)
#pragma warning(disable: 4200)

//////////////////////////////////////////////////////////////////////////////
//
//
#define SYELOG_PIPE_NAMEA       "\\\\.\\pipe\\uberwolf"
#define SYELOG_PIPE_NAMEW       L"\\\\.\\pipe\\uberwolf"
#ifdef UNICODE
#define SYELOG_PIPE_NAME        SYELOG_PIPE_NAMEW
#else
#define SYELOG_PIPE_NAME        SYELOG_PIPE_NAMEA
#endif

//////////////////////////////////////////////////////////////////////////////
//
#define SYELOG_MAXIMUM_MESSAGE  4086    // 4096 - sizeof(header stuff)
#define SYELOG_MAXIMUM_KEY_SIZE 128

typedef struct _SYELOG_MESSAGE
{
    USHORT      nBytes;
	BYTE        nSeverity;
    FILETIME    ftOccurance;
    BOOL        fTerminate;
	BOOL		fIsKey;
	BOOL		fUseOldDxArc;
    CHAR        szMessage[SYELOG_MAXIMUM_MESSAGE];
} SYELOG_MESSAGE, *PSYELOG_MESSAGE;

// Severity Codes.
//
#define SYELOG_SEVERITY_FATAL           0x00            // System is dead.
#define SYELOG_SEVERITY_ALERT           0x10            // Take action immediately.
#define SYELOG_SEVERITY_CRITICAL        0x20            // Critical condition.
#define SYELOG_SEVERITY_ERROR           0x30            // Error
#define SYELOG_SEVERITY_WARNING         0x40            // Warning
#define SYELOG_SEVERITY_NOTICE          0x50            // Significant condition.
#define SYELOG_SEVERITY_INFORMATION     0x60            // Informational
#define SYELOG_SEVERITY_AUDIT_FAIL      0x66            // Audit Failed
#define SYELOG_SEVERITY_AUDIT_PASS      0x67            // Audit Succeeeded
#define SYELOG_SEVERITY_DEBUG           0x70            // Debugging

// Logging Functions.
//
VOID SyelogOpen();
VOID SyelogKey(std::vector<BYTE> key, BOOL useOldDxArc);
VOID Syelog(BYTE nSeverity, PCSTR pszMsgf, ...);
VOID SyelogV(BYTE nSeverity, PCSTR pszMsgf, va_list args);
VOID SyelogClose(BOOL fTerminate);

#pragma warning(pop)
#pragma pack(pop)

#endif //  _SYELOGD_H_
//
///////////////////////////////////////////////////////////////// End of File.
