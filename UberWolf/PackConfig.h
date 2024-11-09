/*
 *  File: PackConfig.h
 *  Copyright (c) 2024 Sinflower
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

#include "Types.h"
#include "WindowBase.h"
#include "resource.h"

class PackConfig : public WindowBase
{
public:
	PackConfig() = delete;

	PackConfig(const HINSTANCE hInstance, const HWND hWndParent) :
		WindowBase(hInstance, hWndParent)
	{
		registerSlot(IDOK, BN_CLICKED, [this]() { onOKClicked(); });
		registerSlot(IDCANCEL, BN_CLICKED, [this]() { onCancelClicked(); });
	}

	~PackConfig()
	{
	}

	void Populate(const tStrings& elems)
	{
		populateComboBox(IDC_COMBO_ENCRYPTION, elems);
	}

	int32_t GetSelectedIndex() const
	{
		HWND cbHWnd = GetDlgItem(hWnd(), IDC_COMBO_ENCRYPTION);

		if (!cbHWnd)
			return m_selectedIndex;

		return static_cast<int32_t>(SendMessage(cbHWnd, CB_GETCURSEL, 0, 0));
	}

	void SetParent(const HWND& hWndParent)
	{
		m_hWndParent = hWndParent;
	}

	bool Show()
	{
		INT_PTR ret = DialogBoxParam(m_hInstance, MAKEINTRESOURCE(IDD_PACK_CONFIG), m_hWndParent, dlgProc, reinterpret_cast<LPARAM>(this));

		return (ret == 1);
	}

	void Hide(const bool& ok = true)
	{
		EndDialog(hWnd(), ok);
		unsetHandle();
	}

protected:
	void afterHWndInit() override
	{
		Populate(m_encryptions);
	}

private:
	void onOKClicked()
	{
		m_selectedIndex = GetSelectedIndex();
		Hide(true);
	}

	void onCancelClicked()
	{
		Hide(false);
	}

	void populateComboBox(const int32_t& id, const tStrings& elems)
	{
		m_encryptions = elems;

		HWND cbHWnd = GetDlgItem(hWnd(), id);

		if (!cbHWnd)
			return;

		SendMessage(cbHWnd, CB_RESETCONTENT, 0, 0);

		for (const auto& elem : elems)
			SendMessage(cbHWnd, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(elem.c_str()));

		SendMessage(cbHWnd, CB_SETCURSEL, 0, 0);
	}

	static INT_PTR CALLBACK dlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		// Check which message was passed in
		switch (uMsg)
		{
			case WM_COMMAND:
				if (WindowBase::ProcessCommand(hWnd, wParam))
					return TRUE;
				break;
			case WM_INITDIALOG:
				WindowBase* pWnd = reinterpret_cast<WindowBase*>(lParam);
				pWnd->SetHandle(hWnd);
				return TRUE;
		}

		return FALSE;
	}

private:
	tStrings m_encryptions  = {};
	int32_t m_selectedIndex = -1;
};
