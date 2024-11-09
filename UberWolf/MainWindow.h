/*
 *  File: MainWindow.h
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
#include <Windows.h>

#include "Localizer.h"
#include "WindowBase.h"
#include "resource.h"

#define MAX_LOADSTRING 100

class MainWindow : public WindowBase
{
	static const int32_t ID_LANGUAGE_EN    = 15000;
	static const int32_t LANGUAGE_MENU_IDX = 0;
	static const int32_t DEFAULT_LANG_ID   = ID_LANGUAGE_EN;

	inline static const std::wstring WINDOW_TITLE = L"Uberwolf v0.4.1";

public:
	MainWindow(const HINSTANCE hInstance) :
		WindowBase(hInstance)
	{
		LoadStringW(m_hInstance, IDC_UBERWOLF, m_szWindowClass, MAX_LOADSTRING);

		registerClass();
	}

	bool InitInstance(int nCmdShow)
	{
		setHandle(CreateWindowW(m_szWindowClass, WINDOW_TITLE.c_str(), WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, m_hInstance, nullptr));

		if (!hWnd()) return false;

		ShowWindow(hWnd(), nCmdShow);
		UpdateWindow(hWnd());

		initLanguages();

		onLanguageChanged(reinterpret_cast<void*>(static_cast<DWORD_PTR>(getSaveValue<int32_t>("language", DEFAULT_LANG_ID))));

		return true;
	}

private:
	void registerClass()
	{
		WNDCLASSEXW wcex;

		wcex.cbSize = sizeof(WNDCLASSEX);

		wcex.style         = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc   = MainWindow::wndProc;
		wcex.cbClsExtra    = 0;
		wcex.cbWndExtra    = 0;
		wcex.hInstance     = m_hInstance;
		wcex.hIcon         = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_UBERWOLF));
		wcex.hCursor       = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wcex.lpszMenuName  = MAKEINTRESOURCEW(IDR_MENU);
		wcex.lpszClassName = m_szWindowClass;
		wcex.hIconSm       = NULL;

		RegisterClassExW(&wcex);
	}

	void initLanguages()
	{
		std::vector<uint16_t> langIDs;
		EnumResourceNames(
			m_hInstance, TEXT("LOCALS"), [](HMODULE hModule, LPCTSTR lpszType, LPTSTR lpszName, LONG_PTR lParam) -> BOOL {
				std::vector<uint16_t>* pLangIDs = reinterpret_cast<std::vector<uint16_t>*>(lParam);
				uint16_t langID;
				// Get the language ID
				if (IS_INTRESOURCE(lpszName))
				{
					langID = LOWORD(reinterpret_cast<DWORD_PTR>(lpszName));
					pLangIDs->push_back(langID);
				}

				return TRUE;
			},
			reinterpret_cast<LONG_PTR>(&langIDs));

		// Get the main menu handle
		HMENU hMenu = GetMenu(hWnd());

		int32_t mc = GetMenuItemCount(hMenu);

		// Get the submenu handle
		HMENU hLangMenu = CreateMenu();

		int32_t idx = ID_LANGUAGE_EN;

		for (const uint16_t& langID : langIDs)
		{
			Localizer::LocMap locMap;
			Localizer::ReadLocalizationFromResource(langID, locMap);

			// Add the language to the menu
			const std::wstring langStr = Localizer::ToUTF16(locMap.at("lang_name"));
			const std::string langCode = locMap.at("lang_code");

			AppendMenuW(hLangMenu, MF_STRING, idx, langStr.c_str());
			registerSlot(idx, BN_CLICKED, [this](void* p) { onLanguageChanged(p); });
			m_menuLangStrMap[idx] = langCode;
			LOC_ADD_LANG(langCode, langID);
			idx++;
		}

		// Add a separator between built-in and file languages
		AppendMenuW(hLangMenu, MF_SEPARATOR, 0, nullptr);

		const std::vector<std::pair<std::wstring, std::string>> fileLangList = Localizer::GetLangCodesFromFolder();

		for (const auto& lang : fileLangList)
		{
			// Add the language to the menu
			const std::wstring langStr = lang.first;
			const std::string langCode = lang.second;

			AppendMenuW(hLangMenu, MF_STRING, idx, langStr.c_str());
			registerSlot(idx, BN_CLICKED, [this](void* p) { onLanguageChanged(p); });
			m_menuLangStrMap[idx] = langCode;
			LOC_ADD_LANG(langCode, -1);
			idx++;
		}

		// Initialize the localizer
		LOC_INIT();

		// Replace the language menu with the new one
		ModifyMenuW(hMenu, LANGUAGE_MENU_IDX, MF_BYPOSITION | MF_POPUP, (UINT_PTR)hLangMenu, LOCW("language"));
	}

	void onLanguageChanged(void* langID)
	{
		const int32_t lID = static_cast<int32_t>(reinterpret_cast<DWORD_PTR>(langID));

		// Get the main menu handle
		HMENU hMenu = GetMenu(hWnd());

		// Get the submenu handle
		HMENU hLangMenu = GetSubMenu(hMenu, LANGUAGE_MENU_IDX);

		// Iterate over the submenu items
		for (int32_t i = 0; i < GetMenuItemCount(hLangMenu); i++)
		{
			// Get the menu item ID
			int32_t menuID = GetMenuItemID(hLangMenu, i);

			// Set the checked state for the selected language to checked and for all others to unchecked
			CheckMenuItem(hLangMenu, menuID, (menuID == lID ? MF_CHECKED : MF_UNCHECKED));
		}

		updateSaveValue<int32_t>("language", lID);

		// Update the localizer
		LOC_LOAD(m_menuLangStrMap.at(lID));

		// Update the caption of hLangMenu
		ModifyMenuW(hMenu, LANGUAGE_MENU_IDX, MF_BYPOSITION | MF_STRING, (UINT_PTR)hLangMenu, LOCW("language"));

		// Force the menu to redraw
		DrawMenuBar(hWnd());

		WindowBase::SignalLocalizationUpdate();
	}

	static LRESULT CALLBACK wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
			case WM_COMMAND:
				if (WindowBase::ProcessCommand(hWnd, wParam))
					return TRUE;
				break;
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
	WCHAR m_szWindowClass[MAX_LOADSTRING];

	std::map<int32_t, std::string> m_menuLangStrMap = {};
};
