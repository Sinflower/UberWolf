#pragma once

#include <Windows.h>
#include <codecvt> // codecvt_utf8
#include <fstream>
#include <locale> // wstring_convert
#include <map>
#include <nlohmann\json.hpp>
#include <string>

#include "resource.h"

#define LOC(_STR_)                  Localizer::GetInstance().GetValue(_STR_)
#define LOCW(_STR_)                 Localizer::GetInstance().GetValueW(_STR_)
#define LOCT(_STR_)                 Localizer::GetInstance().GetValueT(_STR_)
#define LOC_LOAD(_LANG_)            Localizer::GetInstance().LoadLocalization(_LANG_)
#define LOC_ADD_LANG(_LANG_, _RES_) Localizer::GetInstance().AddLanguage(_LANG_, _RES_)
#define LOC_INIT()                  Localizer::GetInstance().Init()

class Localizer
{
public:
	using LocMap  = std::map<std::string, std::string>;
	using LocMapW = std::map<std::string, std::wstring>;

public:
	static Localizer& GetInstance()
	{
		static Localizer instance;
		return instance;
	}

	Localizer(Localizer const&)      = delete;
	void operator=(Localizer const&) = delete;

	void Init()
	{
		if (!readLocalizationFromResource("en", m_defaultLocalization, m_defaultLocalizationW))
			MessageBox(NULL, L"Failed to load default localization", L"Error", MB_OK | MB_ICONERROR);
	}

	bool LoadLocalization(const std::string& lang)
	{
		if (!readLocalizationFromResource(lang, m_localization, m_localizationW))
		{
			MessageBox(NULL, L"Failed to load localization", L"Error", MB_OK | MB_ICONERROR);
			return false;
		}

		return true;
	}

	const char* GetValue(const std::string& key, const bool& forceDefault = false) const
	{
		return getValue(key, forceDefault).c_str();
	}

	LPCTSTR GetValueW(const std::string& key, const bool& forceDefault = false) const
	{
		return getValueW(key, forceDefault).c_str();
	}

	const tString& GetValueT(const std::string& key) const
	{
#ifdef UNICODE
		return getValueW(key);
#else
		return getValue(key);
#endif
	}

	void AddLanguage(const std::string& langCode, const uint16_t& resID)
	{
		m_languageMap[langCode] = resID;
	}

	static bool ReadLocalizationFromResource(const uint16_t& resID, LocMap& map)
	{
		map.clear();

		// Read the default localization file en.json from the executables resource section
		HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(resID), L"LOCALS");
		if (hRes == NULL)
		{
			MessageBox(NULL, L"Failed to find localization resource", L"Error", MB_OK | MB_ICONERROR);
			return false;
		}

		HGLOBAL hData = LoadResource(NULL, hRes);
		if (hData == NULL)
		{
			MessageBox(NULL, L"Failed to load localization resource", L"Error", MB_OK | MB_ICONERROR);
			return false;
		}

		DWORD size = SizeofResource(NULL, hRes);
		if (size == 0)
		{
			MessageBox(NULL, L"Failed to get size of localization resource", L"Error", MB_OK | MB_ICONERROR);
			return false;
		}

		LPVOID pData = LockResource(hData);
		if (pData == NULL)
		{
			MessageBox(NULL, L"Failed to lock localization resource", L"Error", MB_OK | MB_ICONERROR);
			return false;
		}

		std::string strData(static_cast<char*>(pData), size);
		nlohmann::json json = nlohmann::json::parse(strData);

		map = json.get<std::map<std::string, std::string>>();

		return true;
	}

	static std::wstring ToUTF16(const std::string& utf8String)
	{
		static std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
		return conv.from_bytes(utf8String);
	}

private:
	Localizer()
	{
	}

	~Localizer() = default;

	const std::string& getValue(const std::string& key, const bool& forceDefault = false) const
	{
		// Check if the value exists
		if ((m_localization.find(key) == m_localization.end()) || forceDefault)
			return getDefaultValue(key);

		return m_localization.at(key);
	}

	const std::wstring& getValueW(const std::string& key, const bool& forceDefault = false) const
	{
		// Check if the value exists
		if ((m_localizationW.find(key) == m_localizationW.end()) || forceDefault)
			return getDefaultValueW(key);

		return m_localizationW.at(key);
	}

	const std::string& getDefaultValue(const std::string& key) const
	{
		if (m_defaultLocalization.find(key) == m_defaultLocalization.end())
		{
			std::wstring errMsg = L"Failed to find default value for: " + std::wstring(key.begin(), key.end());
			MessageBox(NULL, errMsg.c_str(), L"Error", MB_OK | MB_ICONERROR);
			return m_emptyString;
		}

		return m_defaultLocalization.at(key);
	}

	const std::wstring& getDefaultValueW(const std::string& key) const
	{
		if (m_defaultLocalization.find(key) == m_defaultLocalization.end())
		{
			std::wstring errMsg = L"Failed to find default value for: " + std::wstring(key.begin(), key.end());
			MessageBox(NULL, errMsg.c_str(), L"Error", MB_OK | MB_ICONERROR);
			return m_emptyStringW;
		}

		return m_defaultLocalizationW.at(key);
	}

	bool readLocalizationFromResource(const std::string& lang, LocMap& map, LocMapW& mapW)
	{
		map.clear();
		mapW.clear();

		// Make sure the language is supported
		if (m_languageMap.find(lang) == m_languageMap.end())
		{
			const std::wstring errMsg = L"Language not supported: " + std::wstring(lang.begin(), lang.end());
			MessageBox(NULL, errMsg.c_str(), L"Error", MB_OK | MB_ICONERROR);
			return false;
		}

		if (!ReadLocalizationFromResource(m_languageMap.at(lang), map))
			return false;

		for (const auto& [key, value] : map)
			mapW[key] = ToUTF16(value);

		return true;
	}

private:
	LocMap m_localization;
	LocMapW m_localizationW;

	LocMap m_defaultLocalization;
	LocMapW m_defaultLocalizationW;

	std::string m_emptyString   = "";
	std::wstring m_emptyStringW = L"";

	std::map<std::string, uint16_t> m_languageMap = {};
};
