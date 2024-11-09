/*
 *  File: WolfDec.h
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

#include <cstdint>
#include <exception>
#include <iterator>
#include <string>
#include <tchar.h>
#include <vector>

#include "Types.h"

using DecryptFunction = int (*)(TCHAR*, const TCHAR*, const char*);
using EncryptFunction = int (*)(const TCHAR*, const TCHAR*, bool, const char*, uint16_t);

class InvalidModeException : public std::exception
{};

struct CryptMode
{
	CryptMode(const std::string& name, const uint16_t& cryptVersion, const DecryptFunction& decFunc, const EncryptFunction& encFunc, const std::vector<char> key) :
		name(name),
		cryptVersion(cryptVersion),
		decFunc(decFunc),
		encFunc(encFunc),
		key(key)
	{
	}

	CryptMode(const std::string& name, const uint16_t& cryptVersion, const DecryptFunction& decFunc, const EncryptFunction& encFunc, const std::string& key) :
		name(name),
		cryptVersion(cryptVersion),
		decFunc(decFunc),
		encFunc(encFunc),
		key(key.begin(), key.end())
	{
		this->key.push_back(0x00); // The key needs to end with 0x00 so the parser knows when to stop
	}

	CryptMode(const std::string& name, const uint16_t& cryptVersion, const DecryptFunction& decFunc, const EncryptFunction& encFunc, const std::vector<unsigned char> key) :
		name(name),
		cryptVersion(cryptVersion),
		decFunc(decFunc),
		encFunc(encFunc)
	{
		std::copy(key.begin(), key.end(), std::back_inserter(this->key));
	}

	std::string name;
	uint16_t cryptVersion;
	DecryptFunction decFunc;
	EncryptFunction encFunc;
	std::vector<char> key;
};

using CryptModes = std::vector<CryptMode>;

class WolfDec
{
public:
	inline static const std::string CONFIG_FILE_NAME = "UberWolfConfig.json";

public:
	WolfDec() :
		WolfDec(L"") {}
	WolfDec(const std::wstring& progName, const uint32_t& mode = -1, const bool& isSubProcess = false);
	~WolfDec();

	operator bool() const
	{
		return m_valid;
	}

	bool IsModeSet() const
	{
		return m_mode != -1;
	}

	void SetMode(const uint32_t& mode)
	{
		m_mode = mode;
	}

	bool IsValidFile(const tString& filePath) const;

	bool IsAlreadyUnpacked(const tString& filePath) const;

	bool PackArchive(const tString& folderPath, const bool& override = false);

	bool UnpackArchive(const tString& filePath, const bool& override = false);

	void AddAndSetKey(const std::string& name, const uint16_t& cryptVersion, const bool& useOldDxArc, const Key& key);

	void AddKey(const std::string& name, const uint16_t& cryptVersion, const bool& useOldDxArc, const Key& key);

	void Reset()
	{
		m_mode = -1;
	}

	static tStrings GetEncryptionsW();
	static Strings GetEncryptions();

private:
	void removeOldConfig() const;
	void loadConfig();
	bool detectCrypt(const tString& filePath);
	bool detectMode(const tString& filePath, const bool& override = false);
	bool runProcess(const tString& filePath, const uint32_t& mode, const bool& override = false) const;

	uint16_t getCryptVersion(const tString& filePath) const;

private:
	uint32_t m_mode              = -1;
	CryptModes m_additionalModes = {};
	std::wstring m_progName;
	bool m_isSubProcess = false;
	bool m_valid        = false;
};
