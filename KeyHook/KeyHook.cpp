/*
 *  File: KeyHook.cpp
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

// windows.h needs to be included before detours.h to prevent errors
#include <windows.h>

#include <detours.h>

#include <fstream>
#include <iomanip>
#include <string>
#include <vector>

#define USE_LOGGING

#ifdef USE_LOGGING
#include "syelogRes.h"
#else
#define Syelog(x, y, ...) do {} while(0)
#define _PrintEnter(x, ...) do {} while(0)
#define _PrintExit(x, ...) do {} while(0)
#define _Print(x, ...) do {} while(0)
#endif

void stopProcess();

//////////////////////////////////////////////////////////////////////////////
static bool s_useOld = false;
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
static const std::vector<BYTE> WOLF_RPG_V3_BYTES = { 0x8D, 0x85, 0xFC, 0xFB, 0xFF, 0xFF, 0x50, 0x68, 0xFD };             // Search bytes for WolfRPG >= 3.0
static const std::vector<BYTE> WOLF_RPG_V2_225_BYTES = { 0x83, 0xFE, 0x04, 0x73, 0x3A, 0x55, 0x8D, 0x44, 0x24, 0x14, 0x50 }; // Search bytes for WolfRPG >= 2.225 && < 3.0
static const std::vector<BYTE> WOLF_RPG_V2_20_BYTES = { 0x56, 0x57, 0x68, 0xFD, 0x7F };                                     // Search bytes for WolfRPG >= 2.20 && < 2.225
static const std::vector<BYTE> WOLF_RPG_V2_10_BYTES = { 0x56, 0x8B, 0xF2, 0x8D, 0x4E, 0x01 };                               // Search bytes for WolfRPG < 2.20
//////////////////////////////////////////////////////////////////////////////

extern "C"
{
	void(__stdcall* Real_KeyCreate)(const char* Source, size_t SourceBytes, unsigned char* Key) = nullptr;
	void(__stdcall* Real_KeyCreateOld)(const char* Source, unsigned char* Key) = nullptr;
}

void writeKeyToFile(const char* pKey, const size_t& len)
{
	std::ofstream keyFile("key.json");
	const uint8_t* pData = reinterpret_cast<const uint8_t*>(pKey);

	keyFile << "		\"Unknown\": {" << std::endl
		<< "			\"mode\": \"" << (s_useOld ? "VER6" : "VER8") << "\"," << std::endl
		<< "			\"key\": [";

	for (size_t i = 0; i < len; i++)
	{
		if (s_useOld && pData[i] == 0x00) break;

		if (i > 0) keyFile << ", ";

		keyFile << std::hex << "\"0x" << std::setfill('0') << std::setw(2) << static_cast<uint32_t>(pData[i]) << "\"";
	}

	if (!s_useOld && pData[len - 1] != 0x00)
		keyFile << ", \"0x00\"";

	keyFile << "]" << std::endl
		<< "		}";
	keyFile.close();
}


void writeKeyToPipe(const char* pKey, const size_t& len)
{
	const uint8_t* pData = reinterpret_cast<const uint8_t*>(pKey);

	std::vector<BYTE> keyBytes;

	for (size_t i = 0; i < len; i++)
	{
		if (s_useOld && pData[i] == 0x00) break;
		keyBytes.push_back(pData[i]);
	}

	SyelogKey(keyBytes, s_useOld);
}


// From DXArchive.cpp - KeyCreate
void __stdcall Mine_KeyCreate(const char* Source, size_t SourceBytes, unsigned char* Key)
{
	_PrintEnter("KeyCreate(%p, %d, %p)\n", Source, SourceBytes, Key);

	writeKeyToPipe(Source, SourceBytes);

	// Just exit the process after writing the key to a file
	stopProcess();

	__try
	{
		Real_KeyCreate(Source, SourceBytes, Key);
	}
	__finally
	{
		_PrintExit("KeyCreate()\n");
	};
}

// From DXArchiveV6.cpp - KeyCreate
void __stdcall Mine_KeyCreateOld(const char* Source, unsigned char* Key)
{
	_PrintEnter("KeyCreateOld(%p, %p)\n", Source, Key);

	const size_t len = 12;
	writeKeyToPipe(Source, len);

	// Just exit the process after writing the key to a file
	stopProcess();

	__try
	{
		Real_KeyCreateOld(Source, Key);
	}
	__finally
	{
		_PrintExit("KeyCreateOld()\n");
	};
}

/////////////////////////////////////////////////////////////
// AttachDetours
//
PCHAR DetRealName(const char* psz)
{
	PCHAR locPsz = const_cast<PCHAR>(psz);
	PCHAR pszBeg = const_cast<PCHAR>(psz);
	// Move to end of name.
	while (*locPsz)
		locPsz++;

	// Move back through A-Za-z0-9 names.
	while (locPsz > pszBeg && ((locPsz[-1] >= 'A' && locPsz[-1] <= 'Z') || (locPsz[-1] >= 'a' && locPsz[-1] <= 'z') || (locPsz[-1] >= '0' && locPsz[-1] <= '9')))
		locPsz--;

	return locPsz;
}

VOID DetAttach(PVOID* ppbReal, PVOID pbMine, [[maybe_unused]] const char* psz)
{
	LONG l = DetourAttach(ppbReal, pbMine);

	if (l != 0)
		Syelog(SYELOG_SEVERITY_NOTICE, "Attach failed: `%s': error %d\n", DetRealName(psz), l);
	else
		Syelog(SYELOG_SEVERITY_INFORMATION, "Attach successful: `%s'\n", DetRealName(psz));
}

VOID DetDetach(PVOID* ppbReal, PVOID pbMine, [[maybe_unused]] const char* psz)
{
	LONG l = DetourDetach(ppbReal, pbMine);

	if (l != 0)
		Syelog(SYELOG_SEVERITY_NOTICE, "Detach failed: `%s': error %d\n", DetRealName(psz), l);
}

#define ATTACH(x) DetAttach(&(PVOID&)Real_##x, Mine_##x, #x)
#define DETACH(x) DetDetach(&(PVOID&)Real_##x, Mine_##x, #x)

LONG AttachDetours(VOID)
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	if (!s_useOld)
		ATTACH(KeyCreate);
	else
		ATTACH(KeyCreateOld);

	return DetourTransactionCommit();
}

LONG DetachDetours(VOID)
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	if (!s_useOld)
		DETACH(KeyCreate);
	else
		DETACH(KeyCreateOld);

	return DetourTransactionCommit();
}

//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
// Determine the offset for the KeyCreate function
//
uintptr_t findFunction(const std::vector<BYTE>& tarBytes)
{
	const uintptr_t startAddress = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
	MEMORY_BASIC_INFORMATION info;
	uintptr_t endAddress = startAddress;

	do
	{
		VirtualQuery((void*)endAddress, &info, sizeof(info));
		endAddress = (uintptr_t)info.BaseAddress + info.RegionSize;
	} while (info.Protect > PAGE_NOACCESS);

	endAddress -= info.RegionSize;

	const std::vector<BYTE> ccBytes = { 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC }; // a 0xCC based alignment is used after/before a function

	const DWORD procMemLength = static_cast<DWORD>(endAddress - startAddress);
	const DWORD tarLength = static_cast<DWORD>(tarBytes.size());
	const DWORD ccLength = static_cast<DWORD>(ccBytes.size());
	const BYTE* pProcData = reinterpret_cast<BYTE*>(startAddress);

	DWORD lastCCOffset = 0;

	for (DWORD i = 0; i < procMemLength - tarLength; i++)
	{
		for (DWORD j = 0; j <= tarLength; j++)
		{
			if (j == tarLength)
			{
				Syelog(SYELOG_SEVERITY_INFORMATION, "Func Address : %p", startAddress + lastCCOffset);
				return startAddress + lastCCOffset;
			}
			else if (pProcData[i + j] != tarBytes[j])
				break;
		}

		for (DWORD j = 0; j <= ccLength; j++)
		{
			if (j == ccLength)
				lastCCOffset = i + ccLength;
			else if (pProcData[i + j] != ccBytes[j])
				break;
		}
	}

	return -1;
}

//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
// DLL module information
//
BOOL ThreadAttach([[maybe_unused]] HMODULE hDll)
{
	return TRUE;
}

BOOL ThreadDetach([[maybe_unused]] HMODULE hDll)
{
	return TRUE;
}

BOOL ProcessAttach(HMODULE hDll)
{
#ifdef USE_LOGGING
	s_bLog = FALSE;
	SyelogOpen();
#endif

	uintptr_t funcAddr = findFunction(WOLF_RPG_V3_BYTES);

	if (funcAddr == ~0)
		funcAddr = findFunction(WOLF_RPG_V2_225_BYTES);

	if (funcAddr == ~0)
	{
		funcAddr = findFunction(WOLF_RPG_V2_20_BYTES);
		s_useOld = true;
	}

	if (funcAddr == ~0)
	{
		funcAddr = findFunction(WOLF_RPG_V2_10_BYTES);
		s_useOld = true;
	}

	if (funcAddr == ~0)
	{
		Syelog(SYELOG_SEVERITY_FATAL, "### Error: Unable to find the KeyCreate function\n");
		return FALSE;
	}

	if (!s_useOld)
		Real_KeyCreate = (void(__stdcall*)(const char*, size_t, unsigned char*))(funcAddr);
	else
		Real_KeyCreateOld = (void(__stdcall*)(const char*, unsigned char*))(funcAddr);

	LONG error = AttachDetours();
	if (error != NO_ERROR)
		Syelog(SYELOG_SEVERITY_FATAL, "### Error attaching detours: %d\n", error);

	ThreadAttach(hDll);

#ifdef USE_LOGGING
	s_bLog = TRUE;
#endif

	return TRUE;
}

BOOL ProcessDetach(HMODULE hDll)
{
	ThreadDetach(hDll);
#ifdef USE_LOGGING
	s_bLog = FALSE;
#endif

	if (Real_KeyCreate != nullptr || Real_KeyCreateOld != nullptr)
	{
		LONG error = DetachDetours();
		if (error != NO_ERROR)
			Syelog(SYELOG_SEVERITY_FATAL, "### Error detaching detours: %d\n", error);
	}

#ifdef USE_LOGGING
	SyelogClose(TRUE);
#endif

	return TRUE;
}

BOOL APIENTRY DllMain([[maybe_unused]] HINSTANCE hModule, DWORD dwReason, [[maybe_unused]] PVOID lpReserved)
{
	if (DetourIsHelperProcess())
		return TRUE;

	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
			DetourRestoreAfterWith();
			return ProcessAttach(hModule);
		case DLL_PROCESS_DETACH:
			return ProcessDetach(hModule);
		case DLL_THREAD_ATTACH:
			return ThreadAttach(hModule);
		case DLL_THREAD_DETACH:
			return ThreadDetach(hModule);
	}

	return TRUE;
}

void stopProcess()
{
	// A call to exit will still result in the dll detaching from the process
	exit(0);
}

//
///////////////////////////////////////////////////////////////// End of File.
