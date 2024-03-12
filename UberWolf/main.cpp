// UberWolf.cpp : Defines the entry point for the application.
//

#include <UberWolfLib.h>
#include <Utils.h>
#include <windows.h>

#include "ContentDialog.h"
#include "MainWindow.h"

#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Comctl32.lib")

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ [[maybe_unused]] HINSTANCE hPrevInstance, _In_ [[maybe_unused]] LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	// For the archive extraction a subprocess is created, which will be terminated after the extraction is done.
	// Therefore, check if the current process is a subprocess and if so, create an instance of UberWolfLib which will
	// directly call the unpack function and then terminate the process
	if (IsSubProcess())
	{
		[[maybe_unused]] UberWolfLib uwl;
		return 0;
	}

	MainWindow mainWindow(hInstance);

	if (!mainWindow.InitInstance(nCmdShow))
		return -1;

	ContentDialog contentDialog(hInstance, mainWindow.GetHandle());
	contentDialog.SetupLog();

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_UBERWOLF));

	MSG msg;

	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return static_cast<int>(msg.wParam);
}
