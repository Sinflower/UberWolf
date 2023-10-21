#pragma once

// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstdint>
#include <map>
#include <shlobj.h> // For Shell functions
#include <Shlwapi.h> // For Path functions
#include <functional>
#include <memory>

#include "resource.h"

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Shlwapi.lib")

#define MAX_LOADSTRING 100

std::map<HWND, class WindowBase*> g_windowMap;

class SlotWrapper
{
public:
	using noParam = std::function<void()>;
	using oneParam = std::function<void(void*)>;
	using twoParam = std::function<void(void*, void*)>;

public:
	SlotWrapper(const noParam& nP) : m_noParam(nP) { }
	SlotWrapper(const oneParam& oP) : m_oneParam(oP) { }
	SlotWrapper(const twoParam& tP) : m_twoParam(tP) { }

	void operator()() { if (m_noParam) m_noParam(); }
	void operator()(void* p) { if (m_oneParam) m_oneParam(p); }
	void operator()(void* p1, void* p2) { if (m_twoParam) m_twoParam(p1, p2); }

private:

private:
	noParam m_noParam = nullptr;
	oneParam m_oneParam = nullptr;
	twoParam m_twoParam = nullptr;
};

using SlotPtr = std::shared_ptr<SlotWrapper>;

class WindowBase
{
public:
	WindowBase(const HINSTANCE hInstance) :
		m_hInstance(hInstance),
		m_hWnd(nullptr)
	{
	}

	virtual ~WindowBase()
	{
		if (m_hWnd)
		{
			g_windowMap.erase(m_hWnd);
			DestroyWindow(m_hWnd);
		}
	}

	bool ProcessMessage(const uint32_t& id, const UINT& msg, const WPARAM& wParam, const LPARAM& lParam)
	{
		if (msg == WM_COMMAND)
			return ProcessCommand(wParam);

		return callSlot(id, msg, wParam);
	}

	bool ProcessCommand(const WPARAM& command)
	{
		int wmId = LOWORD(command);
		int wmEvent = HIWORD(command);

		return callSlot(wmId, wmEvent);
	}

	const HWND GetHandle() const { return m_hWnd; }

protected:
	void setHandle(const HWND hWnd)
	{
		m_hWnd = hWnd;
		g_windowMap[m_hWnd] = this;
	}

	void registerSlot(const int32_t& id, const int32_t& code, SlotWrapper::noParam nP)
	{
		if (m_slotMap.count(id) == 0)
			m_slotMap[id] = std::map<int32_t, SlotPtr>();

		m_slotMap[id][code] = std::make_shared<SlotWrapper>(nP);
	}

	void registerSlot(const int32_t& id, const int32_t& code, SlotWrapper::oneParam oP)
	{
		if (m_slotMap.count(id) == 0)
			m_slotMap[id] = std::map<int32_t, SlotPtr>();

		m_slotMap[id][code] = std::make_shared<SlotWrapper>(oP);
	}

	HWND hWnd() const { return m_hWnd; }

private:
	bool callSlot(const int32_t& id, const int32_t& code, const WPARAM& wParam = -1)
	{
		if (m_slotMap.count(id) > 0)
		{
			if (m_slotMap[id].count(code) > 0)
			{
				if (wParam == -1)
					m_slotMap[id][code]->operator()();
				else
					m_slotMap[id][code]->operator()((void*)wParam);
				return true;
			}
		}

		return false;
	}

protected:
	HINSTANCE m_hInstance;

private:
	HWND m_hWnd;
	std::map<int32_t, std::map<int32_t, SlotPtr>> m_slotMap;
};

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

		if (OpenFile(hWnd(), szFile, lpstrFilter, lpstrTitle))
		{
			// If the user selected a file, set the text of the edit control to be the path
			SetDlgItemText(hWnd(), IDC_GAME_LOCATION, szFile);
		}
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

class MainWindow : public WindowBase
{
public:
	MainWindow(const HINSTANCE hInstance) :
		WindowBase(hInstance)
	{
		LoadStringW(m_hInstance, IDS_APP_TITLE, m_szTitle, MAX_LOADSTRING);
		LoadStringW(m_hInstance, IDC_UBERWOLF, m_szWindowClass, MAX_LOADSTRING);

		registerClass();
	}

	bool InitInstance(int nCmdShow)
	{

		setHandle(CreateWindowW(m_szWindowClass, m_szTitle, WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, m_hInstance, nullptr));

		if (!hWnd()) return false;

		ShowWindow(hWnd(), nCmdShow);
		UpdateWindow(hWnd());

		return true;
	}

private:
	void registerClass()
	{
		WNDCLASSEXW wcex;

		wcex.cbSize = sizeof(WNDCLASSEX);

		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = MainWindow::wndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = m_hInstance;
		wcex.hIcon = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_UBERWOLF));
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wcex.lpszMenuName = nullptr;
		wcex.lpszClassName = m_szWindowClass;
		wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

		RegisterClassExW(&wcex);
	}


	static LRESULT CALLBACK wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
			case WM_PAINT:
			{
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hWnd, &ps);
				EndPaint(hWnd, &ps);
			}
			break;
			case WM_DESTROY:
				PostQuitMessage(0);
				break;
			default:
				return DefWindowProc(hWnd, message, wParam, lParam);
		}
		return 0;
	}


private:
	WCHAR m_szTitle[MAX_LOADSTRING];
	WCHAR m_szWindowClass[MAX_LOADSTRING];
};