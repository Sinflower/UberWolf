#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <tchar.h>
#include <iterator>

#include "Types.h"

using DecryptFunction = int (*)(TCHAR*, const TCHAR*, const char*);

struct DecryptMode
{
	DecryptMode(const std::string& name, const DecryptFunction& decFunc, const std::vector<char> key) :
		name(name),
		decFunc(decFunc),
		key(key)
	{
	}

	DecryptMode(const std::string& name, const DecryptFunction& decFunc, const std::string& key) :
		name(name),
		decFunc(decFunc),
		key(key.begin(), key.end())
	{
		this->key.push_back(0x00); // The key needs to end with 0x00 so the parser knows when to stop
	}


	DecryptMode(const std::string& name, const DecryptFunction& decFunc, const std::vector<unsigned char> key) :
		name(name),
		decFunc(decFunc)
	{
		std::copy(key.begin(), key.end(), std::back_inserter(this->key));
	}

	std::string name;
	DecryptFunction decFunc;
	std::vector<char> key;
};

using DecryptModes = std::vector<DecryptMode>;


class WolfDec
{
public:
	inline static const std::string CONFIG_FILE_NAME = "WolfDecConfig.json";

public:
	WolfDec() : WolfDec(L"") {}
	WolfDec(const std::wstring& progName, const uint32_t& mode = -1, const bool& isSubProcess = false);
	~WolfDec();

	operator bool() const { return m_valid; }

	bool IsModeSet() const { return m_mode != -1; }

	bool UnpackArchive(const TCHAR* pFilePath);

	void AddKey(const std::string& name, const bool& useOldDxArc, const Key& key);

private:
	void loadConfig();
	bool detectMode(const TCHAR* pFilePath);
	bool runProcess(const TCHAR* pFilePath, const uint32_t& mode);

private:
	uint32_t m_mode = -1;
	DecryptModes m_additionalModes = {};
	std::wstring m_progName;
	bool m_isSubProcess = false;
	bool m_valid = false;
};

