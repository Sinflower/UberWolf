/*
 *  File: WolfPro.h
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

	bool IsProV2() const
	{
		return m_proVersion == 2;
	}

	bool RemoveProtection();

private:
	Key findProtectionKey(const tString& filePath);
	Key findProtectionKeyV1(std::vector<uint8_t>& byteData) const;
	Key findProtectionKeyV2(std::vector<uint8_t>& byteData) const;
	Key findDxArcKey(const tString& filePath);
	Key findDxArcKeyV1(std::vector<uint8_t>& byteData, const uint32_t& fileSize) const;
	Key findDxArcKeyV2(std::vector<uint8_t>& byteData) const;
	bool validateProtectionKey(const Key& key) const;
	bool readFile(const tString& filePath, std::vector<uint8_t>& bytes, uint32_t& fileSize) const;
	bool writeFile(const tString& filePath, std::vector<uint8_t>& bytes) const;

	std::vector<uint8_t> decrypt(const tString& filePath, const std::array<uint8_t, 3> seedIdx = { 0, 8, 6 }) const;
	std::vector<uint8_t> decrypt(std::vector<uint8_t>& bytes, const std::array<uint8_t, 3> seedIdx = { 0, 8, 6 }) const;
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
	uint32_t m_proVersion   = 1;
};
