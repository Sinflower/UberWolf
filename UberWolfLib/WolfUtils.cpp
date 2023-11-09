#include "WolfUtils.h"
#include "Utils.h"

#include <filesystem>

namespace fs = std::filesystem;

const tStrings POSSIBLE_EXTENSIONS = {
	TEXT(".wolf"),
	TEXT(".data"),
	TEXT(".pak"),
	TEXT(".bin"),
	TEXT(".assets"),
	TEXT(".content"),
	TEXT(".res"),
	TEXT(".resource")
};

const tStrings SPECIAL_FILES = {
	TEXT("Game"),
	TEXT("List"),
	TEXT("Data2"),
	TEXT("GameFile"),
	TEXT("BasicData2")
};

const tString WOLF_DATA_FILE_NAME = TEXT("data");

static tStrings g_specialFiles = {};

tStrings GetSpecialFiles()
{
	if(!g_specialFiles.empty()) return g_specialFiles;

	// Create a list of each special file with each possible extension
for (const tString& s : SPECIAL_FILES)
	{
		for (const tString& e : POSSIBLE_EXTENSIONS)
			g_specialFiles.push_back(s + e);
	}

	return g_specialFiles;
}

bool ExistsWolfDataFile(const tString& folder)
{
	for (const tString& e : POSSIBLE_EXTENSIONS)
	{
		const tString path = folder + TEXT("/") + WOLF_DATA_FILE_NAME + e;
		if (fs::exists(path)) return true;
	}

	return false;
}

tString FindExistingWolfFile(const tString& baseName)
{
	for (const tString& e : POSSIBLE_EXTENSIONS)
	{
		const tString path = baseName + e;
		if (fs::exists(path)) return path;
	}

	return TEXT("");
}

bool IsWolfExtension(const tString& ext)
{
	for (const tString& e : POSSIBLE_EXTENSIONS)
	{
		if (ext == e)
			return true;
	}

	return false;
}

tString GetWolfDataFolder()
{
	return WOLF_DATA_FILE_NAME;
}