#pragma once

#include "Types.h"
#include "WolfDec.h"
#include "WolfPro.h"

enum class UWLExitCode
{
	SUCCESS = 0,
	NOT_INITIALIZED,
	WOLF_DEC_NOT_INITIALIZED,
	INVALID_PATH,
	FILE_NOT_FOUND,
	KEY_MISSING,
	KEY_DETECT_FAILED,
	NOT_WOLF_PRO,
	KEY_DETECT_FAIELD,
	UNPACK_FAILED,
	PROT_KEY_DETECT_FAILED,
	UNKNOWN_ERROR = 999
};

class UberWolfLib
{
public:
	UberWolfLib(const tStrings& argv);
	UberWolfLib(int argc = 0, char* argv[] = nullptr);

	operator bool() const { return m_valid; }

	bool InitGame(const tString& gameExePath);

	UWLExitCode UnpackData();
	UWLExitCode UnpackArchive(const tString& archivePath);
	UWLExitCode FindDxArcKey();
	UWLExitCode FindProtectionKey(std::string& key);
	UWLExitCode FindProtectionKey(std::wstring& key);

	static std::size_t RegisterLogCallback(const LogCallback& callback);
	static void UnregisterLogCallback(const std::size_t& idx);

private:
	UWLExitCode unpackArchive(const tString& archivePath);
	bool findDataFolder();
	UWLExitCode findDxArcKeyFile();
	UWLExitCode findDxArcKeyInject();
	void updateConfig(const bool& useOldDxArc, const Key& key);
	bool findGameFromArchive(const tString& archivePath);
	bool copyDllFromResource(const tString& outDir) const;

private:
	WolfDec m_wolfDec;
	WolfPro m_wolfPro;
	tString m_gameExePath;
	tString m_dataFolder;
	bool m_valid = false;
};
