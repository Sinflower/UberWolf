#include "WolfPro.h"

#include <windows.h>

#include <iostream>
#include <fstream>
#include <array>
#include <filesystem>

#include "Utils.h"
#include "UberLog.h"

namespace fs = std::filesystem;

namespace ProtKey
{
	static const uint32_t KEY_SEED = 0x5D93EBF;
	static const uint32_t SEED_COUNT = 3;
	static const std::size_t START_OFFSET = 0xA;
	static const std::size_t KEY_LEN_OFFSET = START_OFFSET + 5;
	static const std::size_t KEY_OFFSET = KEY_LEN_OFFSET + 4;
	static const uint32_t SHIFT = 12;

	static const tString PROTECTION_KEY_ARCHIVE = TEXT("BasicData");
	static const tString PROTECTION_KEY_FILE = PROTECTION_KEY_ARCHIVE + TEXT("/Game.dat");
}

namespace DxArcKey
{
	static const uint32_t SEED_OFFSET = 4;
	static const uint32_t KEY_LEN_OFFSET = 19;
	static const uint32_t KEY_START_OFFSET = 30;
	static const uint32_t STEP_DIVISOR = 0x58B1;
	static const uint32_t MIN_FILESIZE = 0x5CB8;
	static const uint32_t SHIFT = 12;
	static const std::size_t XOR_START_OFFSET = 20;

	static const tStrings DX_ARC_KEY_FILES = {
		TEXT("Game.wolf"),
		TEXT("List.wolf"),
		TEXT("Data2.wolf"),
		TEXT("GameFile.wolf"),
		TEXT("BasicData2.wolf")
	};
}

WolfPro::WolfPro(const tString& dataFolder) :
	m_dataFolder(dataFolder),
	m_protKeyFile(TEXT("")),
	m_dxArcKeyFile(TEXT(""))
{
	// Check if the data folder exists
	if (!fs::exists(m_dataFolder))
	{
		m_dataFolder = TEXT("");
		ERROR_LOG << TEXT("ERROR: Data folder \"") << m_dataFolder << TEXT("\" does not exist, exiting ...") << std::endl;
		return;
	}

	// Check if the data folder is a directory
	if (!fs::is_directory(m_dataFolder))
	{
		m_dataFolder = TEXT("");
		ERROR_LOG << TEXT("ERROR: Data folder \"") << m_dataFolder << TEXT("\" is not a directory, exiting ...") << std::endl;
		return;
	}

	// Check if the data folder contains any of the DxArc key files	
	for (const tString& file : DxArcKey::DX_ARC_KEY_FILES)
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
		ERROR_LOG << "Unable to find DxArc key file, this does not look like a WolfPro game" << std::endl;
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
	m_protKeyFile = TEXT("");
	m_needsUnpacking = false;

	// Check if the data folder contains the protection key file
	const tString protKeyFile = m_dataFolder + TEXT("/") + ProtKey::PROTECTION_KEY_FILE;
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

Key WolfPro::findDxArcKey(const tString& filePath)
{
	Key key;
	std::vector<uint8_t> bytes;
	uint32_t fileSize;

#ifdef PRINT_DEBUG
	INFO_LOG << TEXT("Searching for DxArc key in: ") << filePath << TEXT("... ") << std::endl;
#endif

	if (!readFile(filePath, bytes, fileSize)) return key;

	if (fileSize < DxArcKey::MIN_FILESIZE) return key;

	srand(bytes[DxArcKey::SEED_OFFSET]);

	for (std::size_t j = DxArcKey::XOR_START_OFFSET; j < bytes.size(); j++)
		bytes[j] ^= static_cast<uint8_t>(rand() >> DxArcKey::SHIFT);

	uint8_t keyLen = bytes[DxArcKey::KEY_LEN_OFFSET];
	uint32_t steps = DxArcKey::STEP_DIVISOR / keyLen;
	uint32_t offset = DxArcKey::KEY_START_OFFSET;

#ifdef PRINT_DEBUG
	std::cout << "Key Length: " << static_cast<uint32_t>(keyLen) << std::endl;
#endif

	for (uint8_t i = 0; i < keyLen; i++)
	{
		key.push_back(bytes[offset]);
		offset += steps;
	}

	return key;
}

Key WolfPro::findProtectionKey(const tString& filePath)
{
	std::array<uint8_t, ProtKey::SEED_COUNT> seeds;
	Key key;
	uint32_t fileSize;
	std::vector<uint8_t> bytes;

#ifdef PRINT_DEBUG
	INFO_LOG << TEXT("Searching for protection key in: ") << filePath << TEXT("... ") << std::endl;
#endif

	if (!readFile(filePath, bytes, fileSize)) return key;

	seeds[0] = bytes[0];
	seeds[1] = bytes[8];
	seeds[2] = bytes[6];

#ifdef PRINT_DEBUG
	std::cout << "Seeds: " << std::hex << std::flush;
	for (const uint8_t& seed : seeds)
		std::cout << "0x" << static_cast<uint32_t>(seed) << " " << std::flush;

	std::cout << std::dec << std::endl;
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

	const uint32_t keyLen = *reinterpret_cast<uint32_t*>(&bytes[ProtKey::KEY_LEN_OFFSET]);

#ifdef PRINT_DEBUG
	std::cout << "Key Length: " << keyLen << std::endl;
#endif

	if (keyLen + ProtKey::KEY_OFFSET >= fileSize)
	{
		ERROR_LOG << L"ERROR: Invalid key length, exiting ..." << std::endl;
		return key;
	}

	srand(ProtKey::KEY_SEED);

	for (std::size_t i = 0; i < keyLen; i++)
		key.push_back(bytes[ProtKey::KEY_OFFSET + i] ^ static_cast<uint8_t>(rand()));

	return key;
}

bool WolfPro::validateProtectionKey(const Key& key)
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

bool WolfPro::readFile(const tString& filePath, std::vector<uint8_t>& bytes, uint32_t& fileSize)
{
	HANDLE hFile = CreateFile(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		ERROR_LOG << TEXT("ERROR: Unable to open file \"") << filePath << TEXT("\".") << std::endl;
		return false;
	}

	fileSize = GetFileSize(hFile, NULL);

	if (fileSize == INVALID_FILE_SIZE)
	{
		ERROR_LOG << TEXT("ERROR: Unable to get file size for \"") << filePath << TEXT("\".") << std::endl;
		CloseHandle(hFile);
		return false;
	}

	bytes.resize(fileSize, 0);

	DWORD dwBytesRead = 0;
	BOOL bResult = ReadFile(hFile, bytes.data(), fileSize, &dwBytesRead, NULL);
	CloseHandle(hFile);

	if (!bResult)
	{
		ERROR_LOG << TEXT("ERROR: Unable to read file \"") << filePath << TEXT("\".") << std::endl;
		return false;
	}

	return true;

}
