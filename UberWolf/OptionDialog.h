#pragma once

#include "resource.h"
#include "WindowBase.h"

#include "UberLog.h"

class OptionDialog : public WindowBase
{
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

	static INT_PTR CALLBACK dlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
				WindowBase* pWnd = reinterpret_cast<WindowBase*>(lParam);
				SendDlgItemMessage(hWnd, IDC_CHECK_OVERWRITE, BM_SETCHECK, pWnd->GetCheckBoxState(IDC_CHECK_OVERWRITE), 0);
				SendDlgItemMessage(hWnd, IDC_CHECK_USE_INJECT, BM_SETCHECK, pWnd->GetCheckBoxState(IDC_CHECK_USE_INJECT), 0);
				pWnd->SetHandle(hWnd);
				return TRUE;
		}

		return FALSE;
	}

private:
	bool m_overwrite = false;
	bool m_useInject = false;
};
