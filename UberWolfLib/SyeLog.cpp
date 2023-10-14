#include "SyeLog.h"

//////////////////////////////////////////////////////////////////////////////
//
//  Detours Test Program (syelogd.cpp of syelogd.exe)
//
//  Microsoft Research Detours Package
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#pragma warning(push)
#if _MSC_VER > 1400
#pragma warning(disable:6102 6103) // /analyze warnings
#endif
#include <strsafe.h>
#pragma warning(pop)
#include <syelog.h>

#if (_MSC_VER < 1299)
typedef ULONG* PULONG_PTR;
typedef ULONG ULONG_PTR;
typedef LONG* PLONG_PTR;
typedef LONG LONG_PTR;
#endif

typedef struct _CLIENT : OVERLAPPED
{
	HANDLE          hPipe;
	BOOL            fAwaitingAccept;
	PVOID           Zero;
	SYELOG_MESSAGE  Message;
} CLIENT, * PCLIENT;

//////////////////////////////////////////////////////////////////////////////
//
LONG        s_nActiveClients = 0;
LONGLONG    s_llStartTime = 0;
LONGLONG    s_llLastTime = 0;

BOOL LogMessageV(BYTE nSeverity, const char* pszMsg, ...);
VOID AcceptAndCreatePipeConnection(PCLIENT pClient, HANDLE hCompletionPort);

std::vector<SyeLog::KeyCallback> g_keyCallbacks;

//////////////////////////////////////////////////////////////////////////////
//
VOID MyErrExit(PCSTR pszMsg)
{
	DWORD error = GetLastError();

	LogMessageV(SYELOG_SEVERITY_FATAL, "Error %d in %s.", error, pszMsg);
	fprintf(stderr, "SYELOGD: Error %ld in %s.\n", error, pszMsg);
	fflush(stderr);
	exit(1);
}

//////////////////////////////////////////////////////////////////////////////
//
BOOL CloseConnection(PCLIENT pClient)
{
	LogMessageV(SYELOG_SEVERITY_INFORMATION, "Client closed pipe.");

	InterlockedDecrement(&s_nActiveClients);
	if (pClient != NULL)
	{
		if (pClient->hPipe != INVALID_HANDLE_VALUE)
		{
			FlushFileBuffers(pClient->hPipe);
			if (!DisconnectNamedPipe(pClient->hPipe))
				MyErrExit("DisconnectNamedPipe");

			CloseHandle(pClient->hPipe);
			pClient->hPipe = INVALID_HANDLE_VALUE;
		}

		GlobalFree(pClient);
		pClient = NULL;
	}

	return TRUE;
}

// Creates a pipe instance and initiate an accept request.
//
PCLIENT CreatePipeConnection(HANDLE hCompletionPort)
{
	HANDLE hPipe = CreateNamedPipe(SYELOG_PIPE_NAME,           // pipe name
								   PIPE_ACCESS_INBOUND |       // read-only access
								   FILE_FLAG_OVERLAPPED,       // overlapped mode
								   PIPE_TYPE_MESSAGE |         // message-type pipe
								   PIPE_READMODE_MESSAGE |     // message read mode
								   PIPE_WAIT,                   // blocking mode
								   PIPE_UNLIMITED_INSTANCES,   // unlimited instances
								   0,                          // output buffer size
								   0,                          // input buffer size
								   20000,                      // client time-out
								   NULL);                      // no security attributes

	if (hPipe == INVALID_HANDLE_VALUE)
		MyErrExit("CreatePipe");

	// Allocate the client data structure.
	//
	PCLIENT pClient = (PCLIENT)GlobalAlloc(GPTR, sizeof(CLIENT));
	if (pClient == NULL)
		MyErrExit("GlobalAlloc pClient");

	ZeroMemory(pClient, sizeof(*pClient));
	pClient->hPipe = hPipe;
	pClient->fAwaitingAccept = TRUE;

	// Associate file with our complietion port.
	//
	if (!CreateIoCompletionPort(pClient->hPipe, hCompletionPort, (ULONG_PTR)pClient, 0))
		MyErrExit("CreateIoComplietionPort pClient");

	if (!ConnectNamedPipe(hPipe, pClient))
	{
		DWORD dwLastErr = GetLastError();
		//Handle race between multiple client connections
		//example: multi thread or client request at alomst same time.
		if (ERROR_PIPE_CONNECTED == dwLastErr)
			AcceptAndCreatePipeConnection(pClient, hCompletionPort);
		else if (dwLastErr != ERROR_IO_PENDING && dwLastErr != ERROR_PIPE_LISTENING)
			MyErrExit("ConnectNamedPipe");
	}
	else
		LogMessageV(SYELOG_SEVERITY_INFORMATION, "ConnectNamedPipe accepted immediately.");

	return pClient;
}

VOID AcceptAndCreatePipeConnection(PCLIENT pClient, HANDLE hCompletionPort)
{
	DWORD nBytes = 0;
	InterlockedIncrement(&s_nActiveClients);
	pClient->fAwaitingAccept = FALSE;
	BOOL b = ReadFile(pClient->hPipe, &pClient->Message, sizeof(pClient->Message), &nBytes, pClient);

	if (!b)
	{
		if (GetLastError() != ERROR_IO_PENDING)
			LogMessageV(SYELOG_SEVERITY_ERROR, "ReadFile failed %d.", GetLastError());
	}

	CreatePipeConnection(hCompletionPort);
}

BOOL LogMessageV(BYTE nSeverity, const char* pszMsg, ...)
{
#ifdef PRINT_DEBUG
	va_list args;
	va_start(args, pszMsg);
	vprintf(pszMsg, args);
	va_end(args);
	printf("\n");
#endif

	return TRUE;
}

BOOL LogKey(PSYELOG_MESSAGE pMessage, DWORD nBytes)
{
	// Sanity check the size of the message.
	//
	if (nBytes > pMessage->nBytes)
		nBytes = pMessage->nBytes;

	if (nBytes >= sizeof(*pMessage))
		nBytes = sizeof(*pMessage) - 1;

	// Ignore messages without key data
	if (nBytes <= offsetof(SYELOG_MESSAGE, szMessage))
		return FALSE;

	PBYTE pszMsg = reinterpret_cast<PBYTE>(pMessage->szMessage);

#ifdef PRINT_DEBUG
	printf("Got a key using the %s DXArc version:\n", (pMessage->fUseOldDxArc ? "old" : "new"));
#endif

	Key key;

	while (*pszMsg)
	{
#ifdef PRINT_DEBUG
		printf("0x%02X ", *pszMsg);
#endif
		key.push_back(*pszMsg);
		pszMsg++;
	}

	// Make sure the key ends with 0x00 so the parser knows when to stop
	if (key.back() != 0x00)
		key.push_back(0x00);

#ifdef PRINT_DEBUG
	printf("\n");
#endif

	for (const auto& callback : g_keyCallbacks)
		callback(key, pMessage->fUseOldDxArc);

	return TRUE;
}

BOOL LogMessage(PSYELOG_MESSAGE pMessage, DWORD nBytes)
{
	// Sanity check the size of the message.
	//
	if (nBytes > pMessage->nBytes)
		nBytes = pMessage->nBytes;

	if (nBytes >= sizeof(*pMessage))
		nBytes = sizeof(*pMessage) - 1;

	// Don't log message if there isn't and message text.
	//
	if (nBytes <= offsetof(SYELOG_MESSAGE, szMessage))
		return FALSE;

	PCHAR pszMsg = pMessage->szMessage;
	while (*pszMsg)
		pszMsg++;

	while (pszMsg > pMessage->szMessage && isspace(pszMsg[-1]))
		*--pszMsg = '\0';

#ifdef PRINT_DEBUG
	printf("%s\n", pMessage->szMessage);
#endif

	return TRUE;
}

DWORD WINAPI WorkerThread(LPVOID pvVoid)
{
	PCLIENT pClient;
	BOOL b;
	LPOVERLAPPED lpo;
	DWORD nBytes;
	HANDLE hCompletionPort = (HANDLE)pvVoid;

	for (BOOL fKeepLooping = TRUE; fKeepLooping;)
	{
		pClient = NULL;
		lpo = NULL;
		nBytes = 0;
		b = GetQueuedCompletionStatus(hCompletionPort, &nBytes, (PULONG_PTR)&pClient, &lpo, INFINITE);

		if (!b || lpo == NULL)
		{
			fKeepLooping = FALSE;
			MyErrExit("GetQueuedCompletionState");
			break;
		}
		else if (!b)
		{
			if (pClient)
			{
				if (GetLastError() == ERROR_BROKEN_PIPE)
					LogMessageV(SYELOG_SEVERITY_INFORMATION, "Client closed pipe.");
				else
					LogMessageV(SYELOG_SEVERITY_ERROR, "GetQueuedCompletionStatus failed %d [%p]", GetLastError(), pClient);

				CloseConnection(pClient);
			}

			continue;
		}

		if (pClient->fAwaitingAccept)
			AcceptAndCreatePipeConnection(pClient, hCompletionPort);
		else
		{
			if (nBytes < offsetof(SYELOG_MESSAGE, szMessage))
				CloseConnection(pClient);

			if (pClient->Message.fTerminate)
				LogMessageV(SYELOG_SEVERITY_NOTICE, "Client requested terminate on next connection close.");

			if (pClient->Message.fIsKey)
				LogKey(&pClient->Message, nBytes);
			else
				LogMessage(&pClient->Message, nBytes);

			b = ReadFile(pClient->hPipe, &pClient->Message, sizeof(pClient->Message), &nBytes, pClient);

			if (!b && GetLastError() == ERROR_BROKEN_PIPE)
				CloseConnection(pClient);
		}
	}
	return 0;
}

BOOL CreateWorkers(HANDLE hCompletionPort)
{
	DWORD dwThread;
	HANDLE hThread;
	DWORD i;
	SYSTEM_INFO SystemInfo;

	GetSystemInfo(&SystemInfo);

	for (i = 0; i < 2 * SystemInfo.dwNumberOfProcessors; i++)
	{
		hThread = CreateThread(NULL, 0, WorkerThread, hCompletionPort, 0, &dwThread);

		if (!hThread)
			MyErrExit("CreateThread WorkerThread");

		CloseHandle(hThread);
	}
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
namespace SyeLog
{
	bool init()
	{
		HANDLE hCompletionPort;
		BOOL fNeedHelp = FALSE;

		// Create the completion port.
		hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
		if (hCompletionPort == NULL)
			MyErrExit("CreateIoCompletionPort");

		// Create completion port worker threads.
		//
		CreateWorkers(hCompletionPort);
		CreatePipeConnection(hCompletionPort);

		return true;
	}

	void registerKeyCallback(KeyCallback callback)
	{
		g_keyCallbacks.push_back(callback);
	}

}

//
//////////////////////////////////////////////////////////////////////////////
