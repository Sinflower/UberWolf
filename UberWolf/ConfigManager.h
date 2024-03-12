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