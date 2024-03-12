/*
 *  File: ConfigManager.h
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

#include <KnownFolders.h>
#include <ShlObj.h>
#include <filesystem>
#include <fstream>
#include <string>

#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

class ConfigManager
{
	inline static const std::wstring CONFIG_FOLDER_NAME = L"UberWolf";
	inline static const std::wstring CONFIG_FILE_NAME   = L"config.json";

public:
	static ConfigManager& GetInstance()
	{
		static ConfigManager instance;
		return instance;
	}

	ConfigManager(ConfigManager const&)  = delete;
	void operator=(ConfigManager const&) = delete;

	template<typename T>
	void SetValue(const int32_t& winID, const uint32_t resID, const T& value)
	{
		m_config[std::to_string(winID)][std::to_string(resID)] = value;
	}

	template<typename T>
	void SetValue(const int32_t& winID, const std::string& res, const T& value)
	{
		m_config[std::to_string(winID)][res] = value;
	}

	template<typename T>
	T GetValue(const int32_t& winID, const uint32_t resID, const T& defaultValue) const
	{
		// Check if the value exists
		if (m_config.find(std::to_string(winID)) == m_config.end() || m_config[std::to_string(winID)].find(std::to_string(resID)) == m_config[std::to_string(winID)].end())
			return defaultValue;

		return m_config[std::to_string(winID)][std::to_string(resID)].get<T>();
	}

	template<typename T>
	T GetValue(const int32_t& winID, const std::string& res, const T& defaultValue) const
	{
		// Check if the value exists
		if (m_config.find(std::to_string(winID)) == m_config.end() || m_config[std::to_string(winID)].find(res) == m_config[std::to_string(winID)].end())
			return defaultValue;

		return m_config[std::to_string(winID)][res].get<T>();
	}

private:
	ConfigManager()
	{
		PWSTR pPath = nullptr;
		if (SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, NULL, &pPath) == S_OK)
		{
			m_configFolder = std::filesystem::path(pPath).wstring() + std::wstring(L"/") + CONFIG_FOLDER_NAME + std::wstring(L"/");
			m_configFile   = m_configFolder + CONFIG_FILE_NAME;
		}

		CoTaskMemFree(pPath);

		loadConfig();
	}

	~ConfigManager()
	{
		saveConfig();
	}

	void loadConfig()
	{
		// Make sure the config folder and file were set
		if (m_configFolder.empty() || m_configFile.empty())
			return;

		// Check if the config folder exists
		if (!fs::exists(m_configFolder))
			return;

		// Check if the config file exists
		if (!fs::exists(m_configFile))
			return;

		// Load the config file
		std::ifstream file(m_configFile);
		file >> m_config;
		file.close();
	}

	void saveConfig()
	{
		// Make sure the config folder and file were set
		if (m_configFolder.empty() || m_configFile.empty())
			return;

		// Check if the config folder exists
		if (!fs::exists(m_configFolder))
			fs::create_directory(m_configFolder);

		// Save the config file
		std::ofstream file(m_configFile);
		file << m_config.dump(4);
		file.close();
	}

private:
	std::wstring m_configFolder = L"";
	std::wstring m_configFile   = L"";
	nlohmann::json m_config;
};