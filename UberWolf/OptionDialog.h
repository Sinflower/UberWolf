/*
 *  File: OptionDialog.h
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

#include "Localizer.h"
#include "WindowBase.h"
#include "resource.h"

class OptionDialog : public WindowBase
{
	static const int32_t CHECKBOX_WIDTH = 20;
	static const int32_t TEXT_HEIGH     = 20;
	static const int32_t STATIC_WIDTH   = 50;

public:
	OptionDialog() = delete;

	OptionDialog(const HINSTANCE hInstance, const HWND hWndParent) :
		WindowBase(hInstance, hWndParent)
	{
		registerSlot(IDOK, BN_CLICKED, [this]() { onOKClicked(); });
		registerSlot(IDCANCEL, BN_CLICKED, [this]() { onCancelClicked(); });
		registerSlot(IDC_CHECK_OVERWRITE, BN_CLICKED, [this]() { onOverwriteClicked(); });
		registerSlot(IDC_CHECK_UNPROTECT, BN_CLICKED, [this]() { onUnprotectClicked(); });
		registerSlot(IDC_CHECK_DEC_WOLFX, BN_CLICKED, [this]() { onDecWolfXClicked(); });

		registerCheckBoxState(IDC_CHECK_OVERWRITE, [this]() { return m_overwrite; });
		registerCheckBoxState(IDC_CHECK_UNPROTECT, [this]() { return m_unprotect; });
		registerCheckBoxState(IDC_CHECK_DEC_WOLFX, [this]() { return m_decWolfX; });

		m_overwrite = getSaveValue<bool>(IDC_CHECK_OVERWRITE, false);
		m_unprotect = getSaveValue<bool>(IDC_CHECK_UNPROTECT, true);
		m_decWolfX  = getSaveValue<bool>(IDC_CHECK_DEC_WOLFX, false);
	}

	~OptionDialog()
	{
	}

	const bool& Unprotect() const
	{
		return m_unprotect;
	}

	const bool& Overwrite() const
	{
		return m_overwrite;
	}

	const bool& DecWolfX() const
	{
		return m_decWolfX;
	}

	void SetParent(const HWND& hWndParent)
	{
		m_hWndParent = hWndParent;
	}

	void Show()
	{
		DialogBoxParam(m_hInstance, MAKEINTRESOURCE(IDD_OPTIONS), m_hWndParent, dlgProc, reinterpret_cast<LPARAM>(this));
	}

	void Hide(const bool& ok = true)
	{
		EndDialog(hWnd(), ok);
		unsetHandle();
	}

	void AdjustSizes()
	{
		int32_t maxWidth = 0;

		maxWidth = max(maxWidth, adjustCheckBox(IDC_CHECK_OVERWRITE));
		maxWidth = max(maxWidth, adjustCheckBox(IDC_CHECK_UNPROTECT));
		maxWidth = max(maxWidth, adjustCheckBox(IDC_CHECK_DEC_WOLFX));

		// Add the static width for the dialog
		maxWidth += STATIC_WIDTH;

		// Adjust the dialog size to fit the new text
		RECT rc;
		GetWindowRect(hWnd(), &rc);
		int32_t minWidth = rc.right - rc.left;
		SetWindowPos(hWnd(), nullptr, 0, 0, max(minWidth, maxWidth), rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER);
	}

private:
	void onOKClicked()
	{
		Hide(true);
	}

	void onCancelClicked()
	{
		Hide(false);
	}

	void onOverwriteClicked()
	{
		m_overwrite = IsDlgButtonChecked(hWnd(), IDC_CHECK_OVERWRITE);
		updateSaveValue<bool>(IDC_CHECK_OVERWRITE, m_overwrite);
	}

	void onUnprotectClicked()
	{
		m_unprotect = IsDlgButtonChecked(hWnd(), IDC_CHECK_UNPROTECT);
		updateSaveValue<bool>(IDC_CHECK_UNPROTECT, m_unprotect);
	}

	void onDecWolfXClicked()
	{
		m_decWolfX = IsDlgButtonChecked(hWnd(), IDC_CHECK_DEC_WOLFX);
		updateSaveValue<bool>(IDC_CHECK_DEC_WOLFX, m_decWolfX);
	}

	int32_t adjustCheckBox(const int32_t& id)
	{
		// Get the handle of the checkbox
		HWND hCheckBox = GetDlgItem(hWnd(), id);
		// Get the width of the caption text
		int32_t width = GetCaptionTextWidth(hCheckBox);
		// Set the new width of the checkbox
		SetWindowPos(hCheckBox, nullptr, 0, 0, width + CHECKBOX_WIDTH, TEXT_HEIGH, SWP_NOMOVE | SWP_NOZORDER);
		// Return the new width
		return width + CHECKBOX_WIDTH;
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
				SendDlgItemMessage(hWnd, IDC_CHECK_OVERWRITE, BM_SETCHECK, pWnd->GetCheckBoxState(IDC_CHECK_OVERWRITE), 0);
				SendDlgItemMessage(hWnd, IDC_CHECK_UNPROTECT, BM_SETCHECK, pWnd->GetCheckBoxState(IDC_CHECK_UNPROTECT), 0);
				SendDlgItemMessage(hWnd, IDC_CHECK_DEC_WOLFX, BM_SETCHECK, pWnd->GetCheckBoxState(IDC_CHECK_DEC_WOLFX), 0);

				SendDlgItemMessage(hWnd, IDC_CHECK_OVERWRITE, WM_SETTEXT, 0, (LPARAM)LOCW("overwrite_label"));
				SendDlgItemMessage(hWnd, IDC_CHECK_UNPROTECT, WM_SETTEXT, 0, (LPARAM)LOCW("unprotect_label"));
				SendDlgItemMessage(hWnd, IDC_CHECK_DEC_WOLFX, WM_SETTEXT, 0, (LPARAM)LOCW("dec_wolfx_label"));
				SendDlgItemMessage(hWnd, IDOK, WM_SETTEXT, 0, (LPARAM)LOCW("ok"));
				SendDlgItemMessage(hWnd, IDCANCEL, WM_SETTEXT, 0, (LPARAM)LOCW("cancel"));
				SetWindowText(hWnd, LOCW("options"));

				pWnd->SetHandle(hWnd);
				pWnd->AdjustSizes();
				return TRUE;
		}

		return FALSE;
	}

private:
	bool m_overwrite = false;
	bool m_unprotect = false;
	bool m_decWolfX  = false;
};
