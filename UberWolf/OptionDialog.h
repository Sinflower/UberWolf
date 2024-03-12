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
		registerSlot(IDC_CHECK_USE_INJECT, BN_CLICKED, [this]() { onUseInjectClicked(); });

		registerCheckBoxState(IDC_CHECK_OVERWRITE, [this]() { return m_overwrite; });
		registerCheckBoxState(IDC_CHECK_USE_INJECT, [this]() { return m_useInject; });

		m_overwrite = getSaveValue<bool>(IDC_CHECK_OVERWRITE, false);
		m_useInject = getSaveValue<bool>(IDC_CHECK_USE_INJECT, false);
	}

	~OptionDialog()
	{
	}

	const bool& UseInject() const
	{
		return m_useInject;
	}

	const bool& Overwrite() const
	{
		return m_overwrite;
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
		maxWidth         = max(maxWidth, adjustCheckBox(IDC_CHECK_OVERWRITE));
		maxWidth         = max(maxWidth, adjustCheckBox(IDC_CHECK_USE_INJECT));

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

	void onUseInjectClicked()
	{
		m_useInject = IsDlgButtonChecked(hWnd(), IDC_CHECK_USE_INJECT);
		updateSaveValue<bool>(IDC_CHECK_USE_INJECT, m_useInject);
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
				SendDlgItemMessage(hWnd, IDC_CHECK_USE_INJECT, BM_SETCHECK, pWnd->GetCheckBoxState(IDC_CHECK_USE_INJECT), 0);

				SendDlgItemMessage(hWnd, IDC_CHECK_OVERWRITE, WM_SETTEXT, 0, (LPARAM)LOCW("overwrite_label"));
				SendDlgItemMessage(hWnd, IDC_CHECK_USE_INJECT, WM_SETTEXT, 0, (LPARAM)LOCW("use_dll_label"));

				pWnd->SetHandle(hWnd);
				pWnd->AdjustSizes();
				return TRUE;
		}

		return FALSE;
	}

private:
	bool m_overwrite = false;
	bool m_useInject = false;
};
