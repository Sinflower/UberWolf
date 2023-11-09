#pragma once

#include <string>
#include <KnownFolders.h>
#include <ShlObj.h>
#include <filesystem>

namespace fs = std::filesystem;

class ConfigManager
{
	inline static const std::wstring CONFIG_FOLDER_NAME = L"";
	inline static const std::wstring CONFIG_FILE_NAME = L"config.json";

public:
	static ConfigManager& GetInstance()
	{
		static ConfigManager instance;
		return instance;
	}

	ConfigManager(ConfigManager const&) = delete;
	void operator=(ConfigManager const&) = delete;

private:
	ConfigManager()
	{
		PWSTR pPath = nullptr;
		if (SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, NULL, &pPath) == S_OK)
		{
			m_configFolder = std::filesystem::path(pPath).wstring() + std::wstring(L"/") + CONFIG_FOLDER_NAME + std::wstring(L"/");
			m_configFile = m_configFolder + CONFIG_FILE_NAME;
		}

		CoTaskMemFree(pPath);
	}

	~ConfigManager()
	{
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
		if(!fs::exists(m_configFile))
			return;
	}

	void saveConfig()
	{
		// Make sure the config folder and file were set
		if (m_configFolder.empty() || m_configFile.empty())
			return;

		// Check if the config folder exists
		if (!fs::exists(m_configFolder))
			fs::create_directory(m_configFolder);
	
	}

private:
	std::wstring m_configFolder = L"";
	std::wstring m_configFile = L"";
};