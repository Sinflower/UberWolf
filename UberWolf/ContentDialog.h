/*
 *  File: ContentDialog.h
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

// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN
#include <Shlwapi.h> // For Path functions
#include <Windows.h>
#include <filesystem>
#include <mutex>
#include <shlobj.h> // For Shell functions

#include <UberWolfLib.h>

#include "Localizer.h"
#include "OptionDialog.h"
#include "PackConfig.h"
#include "WindowBase.h"
#include "WolfUtils.h"
#include "resource.h"

namespace fs = std::filesystem;

namespace
{
// Function to open a file selection dialog
bool OpenFile(HWND hwndParent, LPTSTR lpstrFile, LPCTSTR lpstrFilter, LPCTSTR lpstrTitle)
{
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize     = sizeof(ofn);
	ofn.hwndOwner       = hwndParent;
	ofn.lpstrFile       = lpstrFile;
	ofn.nMaxFile        = MAX_PATH;
	ofn.lpstrFilter     = lpstrFilter;
	ofn.nFilterIndex    = 1;
	ofn.lpstrFileTitle  = NULL;
	ofn.nMaxFileTitle   = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrTitle      = lpstrTitle;
	ofn.Flags           = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

	return GetOpenFileName(&ofn) == TRUE;
}

std::wstring ReplaceAll(const std::wstring& str, const std::wstring& from, const std::wstring& to)
{
	std::wstring result = str;
	size_t startPos     = 0;
	while ((startPos = result.find(from, startPos)) != std::wstring::npos)
	{
		result.replace(startPos, from.length(), to);
		startPos += to.length();
	}

	return result;
}
} // namespace

class ContentDialog : public WindowBase
{
public:
	ContentDialog(const HINSTANCE hInstance, const HWND hWndParent) :
		WindowBase(hInstance, hWndParent),
		m_optionsDialog(hInstance, nullptr),
		m_packConfig(hInstance, nullptr),
		m_mutex()
	{
		setHandle(CreateDialogParamW(m_hInstance, MAKEINTRESOURCE(IDD_CONTENT), m_hWndParent, wndProc, 0));
		registerLocalizedWindow();
		ShowWindow(hWnd(), SW_SHOW);

		// Resize the main window to match the size of the embedded dialog
		RECT rectDialog;
		GetWindowRect(hWnd(), &rectDialog);
		int32_t dialogWidth  = rectDialog.right - rectDialog.left + 16; // I just gave up and added static values until it looked right
		int32_t dialogHeight = rectDialog.bottom - rectDialog.top + 59; // ... See above

		SetWindowPos(m_hWndParent, NULL, 0, 0, dialogWidth, dialogHeight, SWP_NOMOVE | SWP_NOZORDER);

		SetWindowSubclass(GetDlgItem(hWnd(), IDC_LABEL_DROP_FILE), dropProc, 0, (DWORD_PTR)hWnd());

		registerSlot(IDC_OPTIONS, BN_CLICKED, [this]() { onOptionsClicked(); });
		registerSlot(IDC_SELECT_GAME, BN_CLICKED, [this]() { onSelectGameClicked(); });
		registerSlot(IDC_UNPACK, BN_CLICKED, [this]() { onProcessClicked(); });
		registerSlot(IDC_PACK, BN_CLICKED, [this]() { onPackClicked(); });
		registerSlot(IDC_LABEL_DROP_FILE, WM_DROPFILES, [this](void* p) { onDropFile(p); });

		m_optionsDialog.SetParent(hWnd());
		m_packConfig.SetParent(hWnd());
		m_packConfig.Populate(UberWolfLib::GetEncryptionsW());

		// Register the Localizer GetValueW method as the query function for UberWolfLib
		UberWolfLib::RegisterLocQueryFunc([](const std::string& s) -> const tString& { return LOCT(s); });

		// Trigger a localization update to make sure that the window is properly localized
		updateLocalization();
	}

	~ContentDialog()
	{
		if (m_logIndex != -1)
			UberWolfLib::UnregisterLogCallback(m_logIndex);
	}

	void SetupLog()
	{
		m_logIndex = UberWolfLib::RegisterLogCallback([this](const std::wstring& entry, const bool& addNewline) { addLogEntry(entry, addNewline); });
	}

protected:
	void updateLocalization()
	{
		// Drop Label
		SetDlgItemText(hWnd(), IDC_LABEL_DROP_FILE, LOCW("drop_label"));

		// Labels
		SetDlgItemText(hWnd(), IDC_LABEL_GAME_LOCATION, LOCW("game_location"));
		SetDlgItemText(hWnd(), IDC_LABEL_PROTECTION_KEY, LOCW("protection_key"));

		// Buttons
		SetDlgItemText(hWnd(), IDC_SELECT_GAME, LOCW("select_game"));
		SetDlgItemText(hWnd(), IDC_UNPACK, LOCW("process"));
		SetDlgItemText(hWnd(), IDC_PACK, LOCW("pack"));
		SetDlgItemText(hWnd(), IDC_OPTIONS, LOCW("options"));

		adjustLabelEditComb(IDC_LABEL_GAME_LOCATION, IDC_GAME_LOCATION);
		adjustLabelEditComb(IDC_LABEL_PROTECTION_KEY, IDC_PROTECTION_KEY);

		// adjustButton(IDC_UNPACK);
		// adjustButton(IDC_OPTIONS);
	}

private:
	void adjustButton(const int32_t& buttonID)
	{
		HWND hButton = GetDlgItem(hWnd(), buttonID);
		RECT rect;
		GetWindowRect(hButton, &rect);

		int32_t newWidth = GetCaptionTextWidth(hButton) + 20;
		SetWindowPos(hButton, nullptr, 0, 0, newWidth, rect.bottom - rect.top, SWP_NOMOVE | SWP_NOZORDER);
	}

	void setButtonStates(const BOOL& en)
	{
		// Set the "Process" button state
		EnableWindow(GetDlgItem(hWnd(), IDC_UNPACK), en);
		// Set the "Pack" button state
		EnableWindow(GetDlgItem(hWnd(), IDC_PACK), en);
	}

	void adjustLabelEditComb(const int32_t& labelID, const int32_t& editID)
	{
		HWND hLabel = GetDlgItem(hWnd(), labelID);
		// Get the old width
		RECT rect;
		RECT rectPos;
		GetWindowRect(hLabel, &rect);
		int32_t oldWidth = rect.right - rect.left;

		int32_t newWidth = GetCaptionTextWidth(hLabel);
		SetWindowPos(hLabel, nullptr, 0, 0, newWidth, rect.bottom - rect.top, SWP_NOMOVE | SWP_NOZORDER);

		HWND hEdit = GetDlgItem(hWnd(), editID);
		GetWindowRect(hEdit, &rect);
		GetWindowRect(hEdit, &rectPos);
		ScreenToClient(hWnd(), (LPPOINT)&rectPos);

		int32_t widthDiff = newWidth - oldWidth;
		int32_t editWidth = (rect.right - rect.left) - widthDiff;
		int32_t newX      = rectPos.left + widthDiff;

		SetWindowPos(hEdit, nullptr, newX, rectPos.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		SetWindowPos(hEdit, nullptr, 0, 0, editWidth, rect.bottom - rect.top, SWP_NOMOVE | SWP_NOZORDER);
	}

	void onOptionsClicked()
	{
		m_optionsDialog.Show();
	}

	void onPackClicked()
	{
		const fs::path exePath = getExePath();

		if (exePath.empty())
			return;

		setButtonStates(FALSE);

		if (m_packConfig.Show())
		{
			// Pack the data
			UberWolfLib uwl;
			uwl.InitGame(exePath);
			uwl.Configure(m_optionsDialog.Overwrite());
			uwl.PackData(m_packConfig.GetSelectedIndex());
		}

		setButtonStates(TRUE);
	}

	// When the "Select Game" button is clicked
	void onSelectGameClicked()
	{
		WCHAR szFile[MAX_PATH];
		szFile[0] = '\0';

		// Define the file filter and title for the dialog
		LPCTSTR lpstrFilter = L"Wolf (Game[Pro].exe)\0Game.exe;GamePro.exe\0Executable (*.exe)\0*.exe\0All Files (*.*)\0*.*\0";
		LPCTSTR lpstrTitle  = LOCW("open_file");

		// If the user selected a file, set the text of the edit control to be the path
		if (OpenFile(hWnd(), szFile, lpstrFilter, lpstrTitle))
			SetDlgItemText(hWnd(), IDC_GAME_LOCATION, szFile);
	}

	void onProcessClicked()
	{
		const fs::path exePath = getExePath();

		if (exePath.empty())
			return;

		const fs::path basePath = exePath.parent_path();
		const tString dataPath  = FS_PATH_TO_TSTRING(basePath) + TEXT("/") + GetWolfDataFolder();

		// Check if the data folder or data.wolf file exist
		if (!fs::exists(dataPath) && !ExistsWolfDataFile(basePath))
		{
			MessageBox(hWnd(), LOCW("error_msg_2"), LOCW("error"), MB_OK | MB_ICONERROR);
			return;
		}

		setButtonStates(FALSE);

		UberWolfLib uwl;
		uwl.Configure(m_optionsDialog.Overwrite(), m_optionsDialog.Unprotect());
		uwl.InitGame(exePath);
		std::wstring protKey;
		UWLExitCode result = uwl.UnpackData();

		if (result != UWLExitCode::SUCCESS)
		{
			MessageBox(hWnd(), LOCW("error_msg_3"), LOCW("error"), MB_OK | MB_ICONERROR);
			setButtonStates(TRUE);
			return;
		}

		result = uwl.FindProtectionKey(protKey);
		if (result != UWLExitCode::SUCCESS)
		{
			if (result == UWLExitCode::NOT_WOLF_PRO)
				protKey = LOCW("not_protected");
			else
			{
				MessageBox(hWnd(), LOCW("error_msg_4"), LOCW("error"), MB_OK | MB_ICONERROR);
				setButtonStates(TRUE);
				return;
			}
		}

		// Set the text of the protection key edit control to be the key
		SetDlgItemText(hWnd(), IDC_PROTECTION_KEY, protKey.c_str());

		setButtonStates(TRUE);
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
		uwl.Configure(m_optionsDialog.Overwrite(), m_optionsDialog.Unprotect());

		for (const tString& file : files)
		{
			uwl.ResetWolfDec();
			uwl.UnpackArchive(file);
		}
	}

	void addLogEntry(const std::wstring& entry, const bool& addNewline = true)
	{
		// Lock the mutex
		std::lock_guard<std::mutex> lock(m_mutex);

		// Add a new line to the log edit control
		std::wstring msg = entry;
		msg              = ReplaceAll(msg, L"\r\n", L"\n");
		msg              = ReplaceAll(msg, L"\n", L"\r\n");
		if (addNewline)
			msg += L"\r\n";

		// TODO: This appends the text at the current position of the cursor, which can be moved by the user. This is not ideal.
		SendDlgItemMessage(hWnd(), IDC_LOG, EM_REPLACESEL, FALSE, (LPARAM)msg.c_str());
	}

	fs::path getExePath() const
	{
		// Make sure that the user has selected a file
		WCHAR szFile[MAX_PATH];
		GetDlgItemText(hWnd(), IDC_GAME_LOCATION, szFile, MAX_PATH);

		if (szFile[0] == '\0')
		{
			MessageBox(hWnd(), LOCW("select_file"), LOCW("error"), MB_OK | MB_ICONERROR);
			return fs::path();
		}

		fs::path exePath = szFile;

		// Make sure that the file exists
		if (!fs::exists(exePath))
		{
			MessageBox(hWnd(), LOCW("error_msg_1"), LOCW("error"), MB_OK | MB_ICONERROR);
			return fs::path();
		}

		return exePath;
	}

	static void updateLoc(HWND hWnd)
	{
		SetDlgItemText(hWnd, IDC_LABEL_DROP_FILE, LOCW("drop_label"));
		SetDlgItemText(hWnd, IDC_SELECT_GAME, LOCW("select_game"));
		SetDlgItemText(hWnd, IDC_UNPACK, LOCW("process"));
		SetDlgItemText(hWnd, IDC_PACK, LOCW("pack"));
		SetDlgItemText(hWnd, IDC_OPTIONS, LOCW("options"));
		SetDlgItemText(hWnd, IDC_LABEL_PROTECTION_KEY, LOCW("protection_key"));
		SetDlgItemText(hWnd, IDC_LABEL_GAME_LOCATION, LOCW("game_location"));
	}

	static INT_PTR CALLBACK wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		// Check which message was passed in
		switch (uMsg)
		{
			case WM_COMMAND:
				if (WindowBase::ProcessCommand(hWnd, wParam))
					return TRUE;
				break;
			case WM_INITDIALOG:
				updateLoc(hWnd);
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
				if (WindowBase::ProcessMessage(hParent, GetDlgCtrlID(hWnd), uMsg, wParam, lParam))
					return TRUE;
				break;
		}

		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

private:
	OptionDialog m_optionsDialog;
	PackConfig m_packConfig;
	std::mutex m_mutex;
	std::size_t m_logIndex = -1;
};
