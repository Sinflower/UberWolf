#pragma once

// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstdint>
#include <map>
#include <vector>
#include <functional>

#include "SlotWrapper.h"
#include "ConfigManager.h"

std::map<HWND, class WindowBase*> g_windowMap;

class WindowBase
{
	using GetCheckBoxStateFnc = std::function<bool()>;
	static int32_t s_idCounter;
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
			g_windowMap.erase(m_hWnd);
			DestroyWindow(m_hWnd);
		}

		for (auto& thread : m_threadList)
		{
			if (thread.joinable())
				thread.join();
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

protected:
	void setHandle(const HWND hWnd)
	{
		m_hWnd = hWnd;
		g_windowMap[m_hWnd] = this;
	}

	void unsetHandle()
	{
		g_windowMap.erase(m_hWnd);
		m_hWnd = nullptr;
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

	template<typename T>
	void updateSaveValue(const uint32_t& resID, const T& value)
	{
		ConfigManager::GetInstance().SetValue(m_id, resID, value);
	}

	template<typename T>
	T getSaveValue(const uint32_t& resID, const T& defaultValue) const
	{
		return ConfigManager::GetInstance().GetValue(m_id, resID, defaultValue);
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
					m_threadList.push_back(m_slotMap[id][code]->operator()());
				else
					m_threadList.push_back(m_slotMap[id][code]->operator()((void*)wParam));
				return true;
			}
		}

		return false;
	}

protected:
	HINSTANCE m_hInstance = nullptr;
	HWND m_hWndParent = nullptr;

private:
	HWND m_hWnd = nullptr;
	std::map<int32_t, std::map<int32_t, SlotPtr>> m_slotMap;
	std::vector<std::thread> m_threadList;
	std::map<int32_t, GetCheckBoxStateFnc> m_checkBoxStateMap;
	int32_t m_id = -1;
};

int32_t WindowBase::s_idCounter = 0;