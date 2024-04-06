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
	UNKNOWN_ERROR = 999
};

class UberWolfLib
{
	struct Config
	{
		bool useInject = false;
		bool override  = false;
	};

public:
	UberWolfLib(const tStrings& argv);
	UberWolfLib(int argc = 0, char* argv[] = nullptr);

	operator bool() const
	{
		return m_valid;
	}

	void Configure(const bool& override = false, const bool& useInject = false)
	{
		m_config.override  = override;
		m_config.useInject = useInject;
	}

	bool InitGame(const tString& gameExePath);

	UWLExitCode UnpackData();
	UWLExitCode UnpackDataVec(const tStrings& paths);
	UWLExitCode UnpackArchive(const tString& archivePath);
	UWLExitCode FindDxArcKey();
	UWLExitCode FindProtectionKey(std::string& key);
	UWLExitCode FindProtectionKey(std::wstring& key);

	void ResetWolfDec();

	static std::size_t RegisterLogCallback(const LogCallback& callback);
	static void UnregisterLogCallback(const std::size_t& idx);
	static void RegisterLocQueryFunc(const LocalizerQuery& queryFunc);

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
	bool m_valid      = false;
	bool m_dataAsFile = false;
	Config m_config   = {};
};
