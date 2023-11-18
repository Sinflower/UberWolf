#pragma once

// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Shlwapi.h> // For Path functions
#include <shlobj.h> // For Shell functions
#include <mutex>
#include <filesystem>

#include <UberWolfLib.h>

#include "resource.h"
#include "WindowBase.h"
#include "OptionDialog.h"
#include "WolfUtils.h"

namespace fs = std::filesystem;

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

static inline std::wstring ReplaceAll(const std::wstring& str, const std::wstring& from, const std::wstring& to)
{
	std::wstring result = str;
	size_t startPos = 0;
	while ((startPos = result.find(from, startPos)) != std::wstring::npos)
	{
		result.replace(startPos, from.length(), to);
		startPos += to.length();
	}

	return result;
}

class ContentDialog : public WindowBase
{
public:
	ContentDialog(const HINSTANCE hInstance, const HWND hWndParent) :
		WindowBase(hInstance, hWndParent),
		m_optionsDialog(hInstance, nullptr),
		m_mutex()
	{
		setHandle(CreateDialogParamW(m_hInstance, MAKEINTRESOURCE(IDD_CONTENT), m_hWndParent, wndProc, 0));
		ShowWindow(hWnd(), SW_SHOW);

		// Resize the main window to match the size of the embedded dialog
		RECT rectDialog;
		GetWindowRect(hWnd(), &rectDialog);
		int32_t dialogWidth = rectDialog.right - rectDialog.left + 16; // I just gave up and added static values until it looked right
		int32_t dialogHeight = rectDialog.bottom - rectDialog.top + 40; // ... See above

		SetWindowPos(m_hWndParent, NULL, 0, 0, dialogWidth, dialogHeight, SWP_NOMOVE | SWP_NOZORDER);

		SetWindowSubclass(GetDlgItem(hWnd(), IDC_LABEL_DROP_FILE), dropProc, 0, (DWORD_PTR)hWnd());

		registerSlot(IDC_OPTIONS, BN_CLICKED, [this]() { onOptionsClicked(); });
		registerSlot(IDC_SELECT_GAME, BN_CLICKED, [this]() { onSelectGameClicked(); });
		registerSlot(IDC_PROCESS, BN_CLICKED, [this]() { onProcessClicked(); });
		registerSlot(IDC_LABEL_DROP_FILE, WM_DROPFILES, [this](void* p) { onDropFile(p); });

		m_optionsDialog.SetParent(hWnd());
	}

	~ContentDialog()
	{
		if(m_logIndex != -1)
			UberWolfLib::UnregisterLogCallback(m_logIndex);
	}

	void SetupLog()
	{
		m_logIndex = UberWolfLib::RegisterLogCallback([this](const std::wstring& entry, const bool& addNewline) { addLogEntry(entry, addNewline); });
	}

	void onOptionsClicked()
	{
		m_optionsDialog.Show();
	}

	// When the "Select Game" button is clicked
	void onSelectGameClicked()
	{
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

		fs::path exePath = szFile;

		// Make sure that the file exists
		if(!fs::exists(exePath))
		{
			MessageBox(hWnd(), L"File does not exist.", L"Error", MB_OK | MB_ICONERROR);
			return;
		}

		const fs::path basePath = exePath.parent_path();
		const tString dataPath = FS_PATH_TO_TSTRING(basePath) + TEXT("/") + GetWolfDataFolder();

		// Check if the data folder or data.wolf file exist
		if(!fs::exists(dataPath) && !ExistsWolfDataFile(basePath))
		{
			MessageBox(hWnd(), L"Could not find data folder or data.wolf file.", L"Error", MB_OK | MB_ICONERROR);
			return;
		}

		// Disable the "Process" button
		EnableWindow(GetDlgItem(hWnd(), IDC_PROCESS), FALSE);

		UberWolfLib uwl;
		uwl.Configure(m_optionsDialog.Overwrite(), m_optionsDialog.UseInject());
		uwl.InitGame(szFile);
		std::wstring protKey;
		UWLExitCode result = uwl.UnpackData();

		if (result != UWLExitCode::SUCCESS)
		{
			MessageBox(hWnd(), L"Could not unpack all data files.", L"Error", MB_OK | MB_ICONERROR);
			EnableWindow(GetDlgItem(hWnd(), IDC_PROCESS), TRUE);
			return;
		}

		result = uwl.FindProtectionKey(protKey);
		if (result != UWLExitCode::SUCCESS)
		{
			if (result == UWLExitCode::NOT_WOLF_PRO)
				protKey = L"Not Protected";
			else
			{
				MessageBox(hWnd(), L"Could not find protection key.", L"Error", MB_OK | MB_ICONERROR);
				EnableWindow(GetDlgItem(hWnd(), IDC_PROCESS), TRUE);
				return;
			}
		}

		// Set the text of the protection key edit control to be the key
		SetDlgItemText(hWnd(), IDC_PROTECTION_KEY, protKey.c_str());

		// Enable the "Process" button
		EnableWindow(GetDlgItem(hWnd(), IDC_PROCESS), TRUE);
	}

	// When a file is dropped onto the "Drop File" label
	void onDropFile(void* p)
	{
		// Get the file path from the dropped file
		HDROP hDrop = reinterpret_cast<HDROP>(p);

		// Get the number of files dropt onto the label
		int32_t dropCnt = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);

		tStrings files;

		for (int32_t i = 0; i < dropCnt; i++)
		{
			// Get the file path
			TCHAR szFile[MAX_PATH];
			szFile[0] = '\0';

			DragQueryFile(hDrop, i, szFile, MAX_PATH);

			files.push_back(szFile);
		}

		DragFinish(hDrop);

		if (files.size() == 0)
			return;

		// If only one file is dropped, and it is an executable, set the text of the edit control
		if (files.size() == 1 && files[0].find(L".exe") != std::wstring::npos)
		{
			SetDlgItemText(hWnd(), IDC_GAME_LOCATION, files[0].c_str());
			return;
		}

		UberWolfLib uwl;
		uwl.Configure(m_optionsDialog.Overwrite(), m_optionsDialog.UseInject());

		for (const tString& file : files)
		{
			uwl.ResetWolfDec();
			uwl.UnpackArchive(file);
		}
	}

private:
	void addLogEntry(const std::wstring& entry, const bool& addNewline = true)
	{
		// Lock the mutex
		std::lock_guard<std::mutex> lock(m_mutex);

		// Add a new line to the log edit control
		std::wstring msg = entry;
		msg = ReplaceAll(msg, L"\r\n", L"\n");
		msg = ReplaceAll(msg, L"\n", L"\r\n");
		if (addNewline)
			msg += L"\r\n";

		// TODO: This appends the text at the current position of the cursor, which can be moved by the user. This is not ideal.
		SendDlgItemMessage(hWnd(), IDC_LOG, EM_REPLACESEL, FALSE, (LPARAM)msg.c_str());
	}

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
	OptionDialog m_optionsDialog;
	std::mutex m_mutex;
	int32_t m_logIndex = -1;
};
