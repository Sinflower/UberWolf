#pragma once

#include <vector>
#include <cstdint>

#include "Types.h"

class WolfPro
{
public:
	WolfPro() = default;
	WolfPro(const tString& dataFolder);
	~WolfPro() = default;

	const bool& NeedsUnpacking() const { return m_needsUnpacking; }
	const bool& IsWolfPro() const { return m_isWolfPro; }

	Key GetProtectionKey();
	Key GetDxArcKey();
	bool RecheckProtFileState();
	tString GetProtKeyArchiveName() const;

private:
	Key findProtectionKey(const tString& filePath);
	Key findDxArcKey(const tString& filePath);
	bool validateProtectionKey(const Key& key);
	bool readFile(const tString& filePath, std::vector<uint8_t>& bytes, uint32_t& fileSize);

private:
	tString m_dataFolder;
	tString m_protKeyFile;
	tString m_dxArcKeyFile;
	bool m_needsUnpacking = false;
	bool m_isWolfPro = false;
};

