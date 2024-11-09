/*
 *  File: WindowBase.h
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
#include <cstdint>
#include <functional>
#include <map>
#include <vector>

#include "ConfigManager.h"
#include "SlotWrapper.h"

class WindowBase
{
	using GetCheckBoxStateFnc = std::function<bool()>;
	static int32_t s_idCounter;
	static std::map<HWND, class WindowBase*> s_windowMap;
	static std::vector<class WindowBase*> s_localizedWindowList;

public:
	WindowBase(const HINSTANCE hInstance, const HWND hWndParent = nullptr) :
		m_hInstance(hInstance),
		m_hWndParent(hWndParent),
		m_hWnd(nullptr),
		m_id(s_idCounter++)
	{
	}

	virtual ~WindowBase()
	{
		if (m_hWnd)
		{
			s_windowMap.erase(m_hWnd);
			DestroyWindow(m_hWnd);
		}

		for (auto& thread : m_threadList)
		{
			if (thread.joinable())
				thread.join();
		}
	}

	const HWND GetHandle() const
	{
		return m_hWnd;
	}

	void SetHandle(const HWND hWnd)
	{
		setHandle(hWnd);
	}

	bool GetCheckBoxState(const int32_t& id)
	{
		if (m_checkBoxStateMap.count(id) > 0)
			return m_checkBoxStateMap[id]();

		return false;
	}

	virtual void AdjustSizes()
	{
	}

	static int32_t GetCaptionTextWidth(const HWND& hWnd)
	{
		wchar_t text[BUFSIZ];
		GetWindowText(hWnd, text, BUFSIZ);
		return GetTextWidth(hWnd, text);
	}

	static int32_t GetTextWidth(const HWND& hWnd, const wchar_t* pText)
	{
		RECT rc;
		HDC hdc        = GetDC(hWnd);
		HFONT hFont    = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
		HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

		GetClientRect(hWnd, &rc);
		DrawText(hdc, pText, -1, &rc, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
		SelectObject(hdc, hOldFont);
		ReleaseDC(hWnd, hdc);

		return rc.right - rc.left;
	}

	static bool ProcessCommand(const HWND& hWnd, const WPARAM& wParam)
	{
		if (s_windowMap.count(hWnd) == 0)
			return false;

		return s_windowMap[hWnd]->processCommand(wParam);
	}

	static bool ProcessMessage(const HWND& hWnd, const uint32_t& id, const UINT& msg, const WPARAM& wParam, const LPARAM& lParam)
	{
		if (s_windowMap.count(hWnd) == 0)
			return false;

		return s_windowMap[hWnd]->processMessage(id, msg, wParam, lParam);
	}

	static void SignalLocalizationUpdate()
	{
		for (auto& pWin : s_localizedWindowList)
			pWin->updateLocalization();
	}

protected:
	void setHandle(const HWND hWnd)
	{
		m_hWnd              = hWnd;
		s_windowMap[m_hWnd] = this;

		afterHWndInit();
	}

	void unsetHandle()
	{
		s_windowMap.erase(m_hWnd);
		m_hWnd = nullptr;
	}

	bool processMessage(const uint32_t& id, const UINT& msg, const WPARAM& wParam, const LPARAM& lParam)
	{
		if (msg == WM_COMMAND)
			return processCommand(wParam);

		return callSlot(id, msg, wParam);
	}

	bool processCommand(const WPARAM& command)
	{
		int wmId    = LOWORD(command);
		int wmEvent = HIWORD(command);

		return callSlot(wmId, wmEvent);
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

	void registerCheckBoxState(const int32_t& id, const GetCheckBoxStateFnc& fnc)
	{
		m_checkBoxStateMap[id] = fnc;
	}

	void registerLocalizedWindow()
	{
		s_localizedWindowList.push_back(this);
	}

	template<typename T>
	void updateSaveValue(const uint32_t& resID, const T& value)
	{
		ConfigManager::GetInstance().SetValue(m_id, resID, value);
	}

	template<typename T>
	void updateSaveValue(const std::string& res, const T& value)
	{
		ConfigManager::GetInstance().SetValue(m_id, res, value);
	}

	template<typename T>
	T getSaveValue(const uint32_t& resID, const T& defaultValue) const
	{
		return ConfigManager::GetInstance().GetValue(m_id, resID, defaultValue);
	}

	template<typename T>
	T getSaveValue(const std::string& res, const T& defaultValue) const
	{
		return ConfigManager::GetInstance().GetValue(m_id, res, defaultValue);
	}

	virtual void updateLocalization()
	{
	}

	virtual void afterHWndInit() {}

	HWND hWnd() const
	{
		return m_hWnd;
	}

private:
	bool callSlot(const int32_t& id, const int32_t& code, const WPARAM& wParam = -1)
	{
		if (m_slotMap.count(id) > 0)
		{
			if (m_slotMap[id].count(code) > 0)
			{
				if (wParam == -1)
				{
					if (m_slotMap[id][code]->HasNoParam())
						m_threadList.push_back(m_slotMap[id][code]->operator()());
					else
						m_threadList.push_back(m_slotMap[id][code]->operator()(reinterpret_cast<void*>(static_cast<DWORD_PTR>(id))));
				}
				else
					m_threadList.push_back(m_slotMap[id][code]->operator()(reinterpret_cast<void*>(wParam)));
				return true;
			}
		}

		return false;
	}

protected:
	HINSTANCE m_hInstance = nullptr;
	HWND m_hWndParent     = nullptr;

private:
	HWND m_hWnd = nullptr;
	std::map<int32_t, std::map<int32_t, SlotPtr>> m_slotMap;
	std::vector<std::thread> m_threadList;
	std::map<int32_t, GetCheckBoxStateFnc> m_checkBoxStateMap;
	int32_t m_id = -1;
};

int32_t WindowBase::s_idCounter = 0;
std::map<HWND, class WindowBase*> WindowBase::s_windowMap;
std::vector<class WindowBase*> WindowBase::s_localizedWindowList;