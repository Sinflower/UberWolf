#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "Types.h"

enum class BasicDataFiles;

class WolfPro
{
public:
	WolfPro() = default;
	WolfPro(const tString& dataFolder, const bool& dataInBaseFolder = false);
	~WolfPro() = default;

	const bool& NeedsUnpacking() const
	{
		return m_needsUnpacking;
	}

	const bool& IsWolfPro() const
	{
		return m_isWolfPro;
	}

	Key GetProtectionKey();
	Key GetDxArcKey();
	bool RecheckProtFileState();
	tString GetProtKeyArchiveName() const;

	bool RemoveProtection();

private:
	Key findProtectionKey(const tString& filePath) const;
	Key findDxArcKey(const tString& filePath) const;
	bool validateProtectionKey(const Key& key) const;
	bool readFile(const tString& filePath, std::vector<uint8_t>& bytes, uint32_t& fileSize) const;
	bool writeFile(const tString& filePath, std::vector<uint8_t>& bytes) const;

	std::vector<uint8_t> decrypt(const tString& filePath, const std::array<uint8_t, 3> seedIdx = { 0, 8, 6 }) const;
	void removeProtection(const tString& fileName, const BasicDataFiles& bdf) const;
	std::vector<uint8_t> removeProtectionFromProject(const tString& filePath, const uint32_t& seed) const;
	std::vector<uint8_t> removeProtectionFromDat(const tString& filePath, const BasicDataFiles& bdf, uint32_t& projectSeed) const;
	void gameDatUpdateSize(std::vector<uint8_t>& bytes, const uint32_t& oldSize) const;

private:
	tString m_dataFolder;
	tString m_unprotectedFolder;
	tString m_basicDataFolder;
	tString m_protKeyFile;
	tString m_dxArcKeyFile;
	bool m_needsUnpacking   = false;
	bool m_isWolfPro        = false;
	bool m_dataInBaseFolder = false;
};
