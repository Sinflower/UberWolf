#pragma once

// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Shlwapi.h> // For Path functions
#include <shlobj.h> // For Shell functions

#include "resource.h"
#include "WindowBase.h"

// Function to open a file selection dialog
static inline bool OpenFile(HWND hwndParent, LPTSTR lpstrFile, LPCTSTR lpstrFilter, LPCTSTR lpstrTitle)
{
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwndParent;
	ofn.lpstrFile = lpstrFile;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = lpstrFilter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrTitle = lpstrTitle;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

	return GetOpenFileName(&ofn) == TRUE;
}

class ContentDialog : public WindowBase
{
public:
	ContentDialog(const HINSTANCE hInstance, const HWND hWndParent) :
		WindowBase(hInstance),
		m_hWndParent(hWndParent)
	{
		setHandle(CreateDialogParamW(m_hInstance, MAKEINTRESOURCE(IDD_CONTENT), m_hWndParent, wndProc, 0));
		ShowWindow(hWnd(), SW_SHOW);

		// Resize the main window to match the size of the embedded dialog
		RECT rectDialog;
		GetWindowRect(hWnd(), &rectDialog);
		int32_t dialogWidth = rectDialog.right - rectDialog.left + 16; // I just gave up and added static values until it looked right
		int32_t dialogHeight = rectDialog.bottom - rectDialog.top + 40; // ... See above

		SetWindowPos(m_hWndParent, NULL, 0, 0, dialogWidth, dialogHeight, SWP_NOMOVE | SWP_NOZORDER);

		SetWindowSubclass(GetDlgItem(hWnd(), IDC_LABEL_DROP_GAME), dropProc, 0, (DWORD_PTR)hWnd());

		registerSlot(IDC_SELECT_GAME, BN_CLICKED, [this]() { onSelectGameClicked(); });
		registerSlot(IDC_PROCESS, BN_CLICKED, [this]() { onProcessClicked(); });
		registerSlot(IDC_LABEL_DROP_GAME, WM_DROPFILES, [this](void* p) { onDropGame(p); });
	}

	void onSelectGameClicked()
	{
		// When the "Open File" button is clicked
		WCHAR szFile[MAX_PATH];
		szFile[0] = '\0';

		// Define the file filter and title for the dialog
		LPCTSTR lpstrFilter = L"Wolf (Game[Pro].exe)\0Game.exe;GamePro.exe\0Executable (*.exe)\0*.exe\0All Files (*.*)\0*.*\0";
		LPCTSTR lpstrTitle = L"Open File";

		// If the user selected a file, set the text of the edit control to be the path
		if (OpenFile(hWnd(), szFile, lpstrFilter, lpstrTitle))
			SetDlgItemText(hWnd(), IDC_GAME_LOCATION, szFile);
	}

	void onProcessClicked()
	{
		// Make sure that the user has selected a file
		WCHAR szFile[MAX_PATH];
		GetDlgItemText(hWnd(), IDC_GAME_LOCATION, szFile, MAX_PATH);

		if (szFile[0] == '\0')
		{
			MessageBox(hWnd(), L"Please select a file to process.", L"Error", MB_OK | MB_ICONERROR);
			return;
		}

		// Make sure that the file exists
		if (GetFileAttributes(szFile) == INVALID_FILE_ATTRIBUTES)
		{
			MessageBox(hWnd(), L"File does not exist.", L"Error", MB_OK | MB_ICONERROR);
			return;
		}

		// Check if the data folder or data.wolf file exist
		WCHAR szDataFolder[MAX_PATH];
		wcscpy_s(szDataFolder, szFile);
		PathRemoveFileSpec(szDataFolder);
		PathAppend(szDataFolder, L"data");

		WCHAR szDataWolf[MAX_PATH];
		wcscpy_s(szDataWolf, szDataFolder);
		PathAppend(szDataWolf, L"data.wolf");

		if (GetFileAttributes(szDataFolder) == INVALID_FILE_ATTRIBUTES && GetFileAttributes(szDataWolf) == INVALID_FILE_ATTRIBUTES)
		{
			MessageBox(hWnd(), L"Could not find data folder or data.wolf file.", L"Error", MB_OK | MB_ICONERROR);
			return;
		}
	}

	void onDropGame(void* p)
	{
		// When a file is dropped onto the "Drop Game" label
		WCHAR szFile[MAX_PATH];
		szFile[0] = '\0';

		// Get the file path from the dropped file
		HDROP hDrop = reinterpret_cast<HDROP>(p);
		DragQueryFile(hDrop, 0, szFile, MAX_PATH);
		DragFinish(hDrop);

		// Set the text of the edit control to be the path
		SetDlgItemText(hWnd(), IDC_GAME_LOCATION, szFile);
	}

private:
	static INT_PTR CALLBACK wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		// Check which message was passed in
		switch (uMsg)
		{
			case WM_COMMAND:
				if (g_windowMap[hWnd])
				{
					if (g_windowMap[hWnd]->ProcessCommand(wParam))
						return TRUE;
				}
				break;
			case WM_INITDIALOG:
				return TRUE;
		}

		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	static LRESULT CALLBACK dropProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
	{
		const HWND hParent = (HWND)dwRefData;

		switch (uMsg)
		{
			case WM_DROPFILES:
				if (g_windowMap[hParent])
				{
					if (g_windowMap[hParent]->ProcessMessage(GetDlgCtrlID(hWnd), uMsg, wParam, lParam))
						return TRUE;
				}
				break;
		}

		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}


private:
	HWND m_hWndParent;
};
