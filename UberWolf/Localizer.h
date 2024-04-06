/*
 *  File: Localizer.h
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

#include <Windows.h>
#include <codecvt> // codecvt_utf8
#include <filesystem>
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

namespace fs = std::filesystem;

class Localizer
{
	static inline const std::string LOC_FILE_FOLDER = "langs";

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
			if (!readLocalizationFromFile(lang, m_localization, m_localizationW))
			{
				MessageBox(NULL, L"Failed to load localization", L"Error", MB_OK | MB_ICONERROR);
				return false;
			}
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

	static bool ReadLocalizationFromResource(const uint16_t& resID, LocMap& map, const bool showMsg = false)
	{
		map.clear();

		// Read the default localization file en.json from the executables resource section
		HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(resID), L"LOCALS");
		if (hRes == NULL)
		{
			if (showMsg)
				MessageBox(NULL, L"Failed to find localization resource", L"Error", MB_OK | MB_ICONERROR);
			return false;
		}

		HGLOBAL hData = LoadResource(NULL, hRes);
		if (hData == NULL)
		{
			if (showMsg)
				MessageBox(NULL, L"Failed to load localization resource", L"Error", MB_OK | MB_ICONERROR);
			return false;
		}

		DWORD size = SizeofResource(NULL, hRes);
		if (size == 0)
		{
			if (showMsg)
				MessageBox(NULL, L"Failed to get size of localization resource", L"Error", MB_OK | MB_ICONERROR);
			return false;
		}

		LPVOID pData = LockResource(hData);
		if (pData == NULL)
		{
			if (showMsg)
				MessageBox(NULL, L"Failed to lock localization resource", L"Error", MB_OK | MB_ICONERROR);
			return false;
		}

		std::string strData(static_cast<char*>(pData), size);
		nlohmann::json json = nlohmann::json::parse(strData);

		map = json.get<std::map<std::string, std::string>>();

		return true;
	}

	static std::vector<std::pair<std::wstring, std::string>> GetLangCodesFromFolder()
	{
		std::vector<std::pair<std::wstring, std::string>> langCodes;

		fs::path path = fs::current_path() / LOC_FILE_FOLDER;

		// Check if the folder exists
		if (!fs::exists(path) || !fs::is_directory(path))
			return langCodes;

		for (const auto& entry : fs::directory_iterator(path))
		{
			if (entry.is_regular_file() && entry.path().extension() == ".json")
			{
				std::ifstream file(entry.path());
				if (!file.is_open())
					continue;

				nlohmann::json json;
				file >> json;
				file.close();

				const std::wstring langStr = Localizer::ToUTF16(json.at("lang_name"));
				const std::string langCode = json.at("lang_code");

				langCodes.push_back({ langStr, langCode });
			}
		}

		return langCodes;
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

	bool readLocalizationFromFile(const std::string& lang, LocMap& map, LocMapW& mapW)
	{
		map.clear();
		mapW.clear();

		// Make sure the file exists
		fs::path path = fs::current_path() / LOC_FILE_FOLDER / (lang + ".json");
		if (!fs::exists(path))
		{
			const std::wstring errMsg = L"Failed to find localization file: " + path.wstring();
			MessageBox(NULL, errMsg.c_str(), L"Error", MB_OK | MB_ICONERROR);
			return false;
		}

		// Read the localization file
		std::ifstream file(path);
		if (!file.is_open())
		{
			const std::wstring errMsg = L"Failed to open localization file: " + path.wstring();
			MessageBox(NULL, errMsg.c_str(), L"Error", MB_OK | MB_ICONERROR);
			return false;
		}

		nlohmann::json json;
		file >> json;

		file.close();

		map = json.get<std::map<std::string, std::string>>();

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
