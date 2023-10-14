// UberWolf.cpp : Defines the entry point for the application.
//

#include <windows.h>

#include "MainWindow.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ [[maybe_unused]] HINSTANCE hPrevInstance, _In_ [[maybe_unused]] LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	MainWindow mainWindow(hInstance);

	if (!mainWindow.InitInstance(nCmdShow))
		return FALSE;

	ContentDialog contentDialog(hInstance, mainWindow.GetHandle());

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_UBERWOLF));

	MSG msg;

	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg) /* && !IsDialogMessage(hWndDiag, &msg)*/)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}
