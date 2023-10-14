#pragma once

#include "Types.h"
#include "WolfDec.h"
#include "WolfPro.h"

enum class UWLExitCode
{
	SUCCESS = 0,
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
	UberWolfLib(const tStrings& argv, const tString& gameExePath);
	UberWolfLib(int argc, char* argv[], const tString& gameExePath);

	UWLExitCode UnpackData();
	UWLExitCode UnpackArchive(const tString& archivePath);
	UWLExitCode FindDxArcKey();
	UWLExitCode FindProtectionKey(std::string& key);

private:
	UWLExitCode unpackArchive(const tString& archivePath);
	void findDataFolder();
	UWLExitCode findDxArcKeyFile();
	UWLExitCode findDxArcKeyInject();
	void updateConfig(const bool& useOldDxArc, const Key& key);

private:
	WolfDec m_wolfDec;
	WolfPro m_wolfPro;
	tString m_gameExePath;
	tString m_dataFolder;
};


