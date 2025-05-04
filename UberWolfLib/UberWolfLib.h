/*
 *  File: UberWolfLib.h
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
	INVALID_ENCRYPTION,
	UNKNOWN_ERROR = 999
};

class UberWolfLib
{
	inline static const tString UWL_VERSION = _T("0.5.0");
	struct Config
	{
		bool override  = false;
		bool unprotect = false;
		bool decWolfX  = false;
	};

public:
	UberWolfLib(const tStrings& argv);
	UberWolfLib(int argc = 0, char* argv[] = nullptr);

	operator bool() const
	{
		return m_valid;
	}

	void Configure(const bool& override = false, const bool& unprotect = false, const bool decWolfX = false)
	{
		m_config.override  = override;
		m_config.unprotect = unprotect;
		m_config.decWolfX  = decWolfX;
	}

	bool InitGame(const tString& gameExePath);

	UWLExitCode PackData(const int32_t& encIdx);
	UWLExitCode PackDataVec(const tStrings& paths);
	UWLExitCode PackArchive(const tString& archivePath);

	UWLExitCode UnpackData();
	UWLExitCode UnpackDataVec(const tStrings& paths);
	UWLExitCode UnpackArchive(const tString& archivePath);

	UWLExitCode FindDxArcKey(const bool& quiet = false);
	UWLExitCode FindProtectionKey(std::string& key);
	UWLExitCode FindProtectionKey(std::wstring& key);

	void ResetWolfDec();

	static std::size_t RegisterLogCallback(const LogCallback& callback);
	static void UnregisterLogCallback(const std::size_t& idx);
	static void RegisterLocQueryFunc(const LocalizerQuery& queryFunc);

	static tString GetVersion()
	{
		return UWL_VERSION;
	}

	static tStrings GetEncryptionsW()
	{
		return WolfDec::GetEncryptionsW();
	}

	static Strings GetEncryptions()
	{
		return WolfDec::GetEncryptions();
	}

private:
	UWLExitCode packData(const tString& dataPath);
	UWLExitCode unpackArchive(const tString& archivePath, const bool& quiet = false, const bool& secondRun = false);
	bool findDataFolder();
	UWLExitCode findDxArcKeyFile(const bool& quiet = false);
	void updateConfig(const bool& useOldDxArc, const Key& key);
	bool findGameFromArchive(const tString& archivePath);

private:
	WolfDec m_wolfDec;
	WolfPro m_wolfPro;
	tString m_gameExePath;
	tString m_dataFolder;
	bool m_valid      = false;
	bool m_dataAsFile = false;
	Config m_config   = {};
};
