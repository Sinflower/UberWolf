#include "WolfPro.h"

#include <windows.h>

#include <array>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>

#include "UberLog.h"
#include "Utils.h"
#include "WolfUtils.h"

namespace fs = std::filesystem;

namespace ProtKey
{
static const uint32_t KEY_SEED          = 0x5D93EBF;
static const uint32_t SEED_COUNT        = 3;
static const std::size_t START_OFFSET   = 0xA;
static const std::size_t KEY_LEN_OFFSET = START_OFFSET + 5;
static const std::size_t KEY_OFFSET     = KEY_LEN_OFFSET + 4;
static const uint32_t SHIFT             = 12;

static const std::vector<uint8_t> DEC_START = { 0x00, 0x57, 0x00, 0x00, 0x4F, 0x4C, 0x55, 0x46, 0x4D, 0x00 };

static const tString GAME_DAT = TEXT("Game.dat");

static const tString PROTECTION_KEY_ARCHIVE = TEXT("BasicData");

static const tString UNPROTECTED_FOLDER = TEXT("unprotected");

static const tString COM_EVENT                = TEXT("CommonEvent.dat");
static const tStrings GENERAL_PROTECTED_FILES = { TEXT("TileSetData"), TEXT("SysDatabase"), TEXT("DataBase"), TEXT("CDataBase") };
static const tStrings PROTECTED_FILES_EXT     = { TEXT(".dat"), TEXT(".project") };
} // namespace ProtKey

namespace DxArcKey
{
static const uint32_t SEED_OFFSET         = 4;
static const uint32_t KEY_LEN_OFFSET      = 19;
static const uint32_t KEY_START_OFFSET    = 30;
static const uint32_t STEP_DIVISOR        = 0x58B1;
static const uint32_t MIN_FILESIZE        = 0x5CB8;
static const uint32_t SHIFT               = 12;
static const std::size_t XOR_START_OFFSET = 20;
} // namespace DxArcKey

enum class BasicDataFiles
{
	GENERAL = 0,
	GAME_DAT,
	COM_EVENT
};

WolfPro::WolfPro(const tString& dataFolder, const bool& dataInBaseFolder) :
	m_dataFolder(dataFolder),
	m_unprotectedFolder(dataFolder + TEXT("/") + ProtKey::UNPROTECTED_FOLDER),
	m_basicDataFolder(TEXT("")),
	m_protKeyFile(TEXT("")),
	m_dxArcKeyFile(TEXT("")),
	m_dataInBaseFolder(dataInBaseFolder)
{
	// Check if the data folder exists
	if (!fs::exists(m_dataFolder))
	{
		m_dataFolder = TEXT("");
		ERROR_LOG << std::format(TEXT("ERROR: Data folder \"{}\" does not exist, exiting ..."), m_dataFolder) << std::endl;
		return;
	}

	// Check if the data folder is a directory
	if (!fs::is_directory(m_dataFolder))
	{
		m_dataFolder = TEXT("");
		ERROR_LOG << std::format(TEXT("ERROR: Data folder \"{}\" is not a directory, exiting ..."), m_dataFolder) << std::endl;
		return;
	}

	const tStrings keyFiles = GetSpecialFiles();

	// Check if the data folder contains any of the DxArc key files
	for (const tString& file : keyFiles)
	{
		const tString filePath = m_dataFolder + TEXT("/") + file;
		if (fs::exists(filePath))
		{
			m_dxArcKeyFile = filePath;
			break;
		}
	}

	if (m_dxArcKeyFile.empty())
	{
		ERROR_LOG << TEXT("WARNING: Unable to find DxArc key file, this does not look like a WolfPro game") << std::endl;
		return;
	}
	else
		m_isWolfPro = true;

	RecheckProtFileState();
}

Key WolfPro::GetProtectionKey()
{
	if (m_protKeyFile.empty() && !RecheckProtFileState())
		return Key();

	Key key = findProtectionKey(m_protKeyFile);

	if (!validateProtectionKey(key))
	{
		ERROR_LOG << TEXT("ERROR: Invalid protection key") << std::endl;
		return Key();
	}

	return key;
}

Key WolfPro::GetDxArcKey()
{
	if (m_dxArcKeyFile.empty())
		return Key();

	Key key = findDxArcKey(m_dxArcKeyFile);

	if (key.empty())
	{
		ERROR_LOG << TEXT("ERROR: Unable to find DxArc key") << std::endl;
		return Key();
	}

	// Make sure the key ends with 0x00
	if (key.back() != 0x00)
		key.push_back(0x00);

	return key;
}

bool WolfPro::RecheckProtFileState()
{
	m_protKeyFile    = TEXT("");
	m_needsUnpacking = false;

	// Check if the data folder contains the protection key file
	if (m_dataInBaseFolder)
		m_basicDataFolder = m_dataFolder + TEXT("/") + GetWolfDataFolder() + TEXT("/");
	else
		m_basicDataFolder = m_dataFolder + TEXT("/");

	m_basicDataFolder += ProtKey::PROTECTION_KEY_ARCHIVE;

	const tString protKeyFile = m_basicDataFolder + TEXT("/") + ProtKey::GAME_DAT;

	if (fs::exists(protKeyFile))
		m_protKeyFile = protKeyFile;
	else
		m_needsUnpacking = true;

	return (!m_protKeyFile.empty());
}

tString WolfPro::GetProtKeyArchiveName() const
{
	return ProtKey::PROTECTION_KEY_ARCHIVE;
}

bool WolfPro::RemoveProtection()
{
	if (!m_isWolfPro)
	{
		ERROR_LOG << TEXT("ERROR: Unable to remove protection, this does not look like a WolfPro game") << std::endl;
		return false;
	}

	if (m_dataFolder.empty())
	{
		ERROR_LOG << TEXT("ERROR: Unable to remove protection, data folder not set") << std::endl;
		return false;
	}

	// Check if the unprotected folder exists in the data folder and create it if it doesn't
	if (!fs::exists(m_unprotectedFolder))
	{
		if (!fs::create_directory(m_unprotectedFolder))
		{
			ERROR_LOG << std::format(TEXT("ERROR: Unable to create unprotected folder ({})"), m_unprotectedFolder) << std::endl;
			return false;
		}
	}

	// Remove protection from all protected general files
	for (const tString& fileName : ProtKey::GENERAL_PROTECTED_FILES)
		removeProtection(fileName, BasicDataFiles::GENERAL);

	// Remove protection from Game.dat
	removeProtection(ProtKey::GAME_DAT, BasicDataFiles::GAME_DAT);

	// Remove protection from CommonEvents.dat
	removeProtection(ProtKey::COM_EVENT, BasicDataFiles::COM_EVENT);

	INFO_LOG << std::format(TEXT("Unprotected files can be found in: {}"), m_unprotectedFolder) << std::endl;

	return true;
}

/////////////////////////////////////////

Key WolfPro::findDxArcKey(const tString& filePath) const
{
	Key key;
	std::vector<uint8_t> bytes;
	uint32_t fileSize;

#ifdef PRINT_DEBUG
	INFO_LOG << std::format(TEXT("Searching for DxArc key in: {} ... "), filePath) << std::endl;
#endif

	if (!readFile(filePath, bytes, fileSize)) return key;

	if (fileSize < DxArcKey::MIN_FILESIZE) return key;

	srand(bytes[DxArcKey::SEED_OFFSET]);

	for (std::size_t j = DxArcKey::XOR_START_OFFSET; j < bytes.size(); j++)
		bytes[j] ^= static_cast<uint8_t>(rand() >> DxArcKey::SHIFT);

	uint8_t keyLen  = bytes[DxArcKey::KEY_LEN_OFFSET];
	uint32_t steps  = DxArcKey::STEP_DIVISOR / keyLen;
	uint32_t offset = DxArcKey::KEY_START_OFFSET;

#ifdef PRINT_DEBUG
	INFO_LOG << std::format(TEXT("Key Length: {}"), static_cast<uint32_t>(keyLen)) << std::endl;
#endif

	for (uint8_t i = 0; i < keyLen; i++)
	{
		key.push_back(bytes[offset]);
		offset += steps;
	}

	return key;
}

Key WolfPro::findProtectionKey(const tString& filePath) const
{
#ifdef PRINT_DEBUG
	INFO_LOG << std::format(TEXT("Searching for protection key in: {} ..."), filePath) << std::endl;
#endif

	Key key;
	std::vector<uint8_t> bytes = decrypt(filePath);

	if (bytes.empty())
	{
		ERROR_LOG << TEXT("ERROR: Unable to decrypt protection key file") << std::endl;
		return key;
	}

	const uint32_t keyLen = *reinterpret_cast<uint32_t*>(&bytes[ProtKey::KEY_LEN_OFFSET]);

#ifdef PRINT_DEBUG
	INFO_LOG << std::format(TEXT("Key Length: {}"), keyLen) << std::endl;
#endif

	if (keyLen + ProtKey::KEY_OFFSET >= bytes.size())
	{
		ERROR_LOG << TEXT("ERROR: Invalid key length, exiting ...") << std::endl;
		return key;
	}

	srand(ProtKey::KEY_SEED);

	for (std::size_t i = 0; i < keyLen; i++)
		key.push_back(bytes[ProtKey::KEY_OFFSET + i] ^ static_cast<uint8_t>(rand()));

	return key;
}

bool WolfPro::validateProtectionKey(const Key& key) const
{
	if (key.empty()) return false;

	// The key can only contain alpha numerical characters so anything outside that range indicates an invalid key
	for (const uint8_t& k : key)
	{
		if (k < '0' || k > 'z') return false;
		if (k > '9' && k < 'A') return false;
		if (k > 'Z' && k < 'a') return false;
	}

	return true;
}

bool WolfPro::readFile(const tString& filePath, std::vector<uint8_t>& bytes, uint32_t& fileSize) const
{
	HANDLE hFile = CreateFile(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		ERROR_LOG << std::format(TEXT("ERROR: Unable to open file \"{}\"."), filePath) << std::endl;
		return false;
	}

	fileSize = GetFileSize(hFile, NULL);

	if (fileSize == INVALID_FILE_SIZE)
	{
		ERROR_LOG << std::format(TEXT("ERROR: Unable to get file size for \"{}\"."), filePath) << std::endl;
		CloseHandle(hFile);
		return false;
	}

	bytes.resize(fileSize, 0);

	DWORD dwBytesRead = 0;
	BOOL bResult      = ReadFile(hFile, bytes.data(), fileSize, &dwBytesRead, NULL);
	CloseHandle(hFile);

	if (!bResult)
	{
		ERROR_LOG << std::format(TEXT("ERROR: Unable to read file \"{}\"."), filePath) << std::endl;
		return false;
	}

	return true;
}

bool WolfPro::writeFile(const tString& filePath, std::vector<uint8_t>& bytes) const
{
	HANDLE hFile = CreateFile(filePath.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		ERROR_LOG << std::format(TEXT("ERROR: Unable to open file \"{}\"."), filePath) << std::endl;
		return false;
	}

	DWORD dwBytesWritten = 0;

	BOOL bResult = WriteFile(hFile, bytes.data(), bytes.size(), &dwBytesWritten, NULL);
	CloseHandle(hFile);

	if (!bResult)
	{
		ERROR_LOG << std::format(TEXT("ERROR: Unable to write file \"{}\"."), filePath) << std::endl;
		return false;
	}

	return true;
}

std::vector<uint8_t> WolfPro::decrypt(const tString& filePath, const std::array<uint8_t, 3> seedIdx) const
{
	std::array<uint8_t, ProtKey::SEED_COUNT> seeds;
	uint32_t fileSize;
	std::vector<uint8_t> bytes;

#ifdef PRINT_DEBUG
	INFO_LOG << std::format(TEXT("Decrypting: {} ..."), filePath) << std::endl;
#endif

	if (!readFile(filePath, bytes, fileSize)) return bytes;

	for (std::size_t i = 0; i < seedIdx.size(); i++)
		seeds[i] = bytes[seedIdx[i]];

#ifdef PRINT_DEBUG
	INFO_LOG << TEXT("Seeds: ") << std::flush;
	for (const uint8_t& seed : seeds)
		INFO_LOG << std::format(TEXT("{:#x} "), static_cast<uint32_t>(seed)) << std::flush;

	INFO_LOG << std::endl;
#endif

	for (std::size_t i = 0; i < seeds.size(); i++)
	{
		srand(seeds[i]);

		std::size_t inc = 1;

		if (i == 1) inc = 2;
		if (i == 2) inc = 5;

		if (ProtKey::START_OFFSET < bytes.size())
		{
			for (std::size_t j = ProtKey::START_OFFSET; j < bytes.size(); j += inc)
				bytes[j] ^= static_cast<uint8_t>(rand() >> ProtKey::SHIFT);
		}
	}

	return bytes;
}

void WolfPro::removeProtection(const tString& fileName, const BasicDataFiles& bdf) const
{
	INFO_LOG << std::format(TEXT("Removing protection from: {} ... "), fileName) << std::flush;
	std::vector<uint8_t> bytes;
	uint32_t projectSeed;
	const tString filePath = m_basicDataFolder + TEXT("/") + fileName;

	if (bdf == BasicDataFiles::GENERAL)
	{
		tString file = filePath + ProtKey::PROTECTED_FILES_EXT[0];
		if (fs::exists(file))
		{
			bytes = removeProtectionFromDat(file, bdf, projectSeed);

			writeFile(m_unprotectedFolder + TEXT("/") + fileName + ProtKey::PROTECTED_FILES_EXT[0], bytes);

			file = filePath + ProtKey::PROTECTED_FILES_EXT[1];
			if (fs::exists(file))
			{
				bytes = removeProtectionFromProject(file, projectSeed);
				writeFile(m_unprotectedFolder + TEXT("/") + fileName + ProtKey::PROTECTED_FILES_EXT[1], bytes);
			}
		}
		else
		{
			INFO_LOG << TEXT("Failed") << std::endl;
			ERROR_LOG << std::format(TEXT("ERROR: Unable to find file \"{}\"."), filePath) << std::endl;
			return;
		}
	}
	else if (bdf == BasicDataFiles::GAME_DAT || bdf == BasicDataFiles::COM_EVENT)
	{
		if (fs::exists(filePath))
		{
			bytes = removeProtectionFromDat(filePath, bdf, projectSeed);
			writeFile(m_unprotectedFolder + TEXT("/") + fileName, bytes);
		}
		else
		{
			INFO_LOG << TEXT("Failed") << std::endl;
			ERROR_LOG << std::format(TEXT("ERROR: Unable to find file \"{}\"."), filePath) << std::endl;
			return;
		}
	}

	INFO_LOG << TEXT("Done") << std::endl;
}

std::vector<uint8_t> WolfPro::removeProtectionFromProject(const tString& filePath, const uint32_t& seed) const
{
	std::vector<uint8_t> bytes;
	uint32_t fileSize;

#ifdef PRINT_DEBUG
	INFO_LOG << std::format(TEXT("Decrypting: {} ..."), filePath) << std::endl;
#endif

	if (!readFile(filePath, bytes, fileSize)) return bytes;

	srand(seed);

	for (uint8_t& byte : bytes)
		byte ^= static_cast<uint8_t>(rand());

	return bytes;
}

std::vector<uint8_t> WolfPro::removeProtectionFromDat(const tString& filePath, const BasicDataFiles& bdf, uint32_t& projectSeed) const
{
	std::array<uint8_t, 3> seedIdx = { 0, 3, 9 };

	if (bdf == BasicDataFiles::GAME_DAT)
		seedIdx = { 0, 8, 6 };

	std::vector<uint8_t> bytes = decrypt(filePath, seedIdx);

	if (bytes.empty())
	{
		ERROR_LOG << TEXT("ERROR: Unable to decrypt protected file") << std::endl;
		return std::vector<uint8_t>();
	}

	// Cast the value to an int8_t to preserve the sign and then write it to an uint32_t
	// to get a 32 bit value with the correct sign propagated to the upper bits
	projectSeed = static_cast<int8_t>(bytes[ProtKey::KEY_OFFSET]);

	const uint32_t keyLen = *reinterpret_cast<uint32_t*>(&bytes[ProtKey::KEY_LEN_OFFSET]);

	if (keyLen + ProtKey::KEY_OFFSET >= bytes.size())
	{
		ERROR_LOG << TEXT("ERROR: Invalid key length") << std::endl;
		return std::vector<uint8_t>();
	}

	const uint32_t oldSize = bytes.size();

	// Remove the first bytes until after the key
	bytes.erase(bytes.begin(), bytes.begin() + ProtKey::KEY_OFFSET + keyLen);

	// Insert the decrypted start bytes
	bytes.insert(bytes.begin(), ProtKey::DEC_START.begin(), ProtKey::DEC_START.end());

	if (bdf == BasicDataFiles::GAME_DAT)
	{
		// For Game.dat swap bytes 6 and 9
		std::swap(bytes[6], bytes[9]);

		gameDatUpdateSize(bytes, oldSize);
	}
	else if (bdf == BasicDataFiles::COM_EVENT)
	{
		// If the file is CommonEvents.dat, replace byte 8 with 0x43
		bytes[8] = 0x43;
	}

	return bytes;
}

void WolfPro::gameDatUpdateSize(std::vector<uint8_t>& bytes, const uint32_t& oldSize) const
{
	std::size_t offset = ProtKey::DEC_START.size();
	offset += *reinterpret_cast<uint32_t*>(&bytes[offset]) + 4; // Bytes
	offset += 4;                                                // DWORD
	offset += *reinterpret_cast<uint32_t*>(&bytes[offset]) + 4; // Title
	offset += *reinterpret_cast<uint32_t*>(&bytes[offset]) + 4; // Number (0000-0000)
	offset += *reinterpret_cast<uint32_t*>(&bytes[offset]) + 4; // Decrypt Key
	offset += *reinterpret_cast<uint32_t*>(&bytes[offset]) + 4; // Font

	while (*reinterpret_cast<uint32_t*>(&bytes[offset]) != (oldSize - 1))
		offset += *reinterpret_cast<uint32_t*>(&bytes[offset]) + 4; // ?

	*reinterpret_cast<uint32_t*>(&bytes[offset]) = bytes.size() - 1;
}
