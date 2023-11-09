#pragma once

#include "Types.h"

tStrings GetSpecialFiles();
bool ExistsWolfDataFile(const tString& folder);
tString FindExistingWolfFile(const tString& baseName);
bool IsWolfExtension(const tString& ext);
tString GetWolfDataFolder();