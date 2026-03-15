/*
 *  File: WolfDataBase.hpp
 *  Copyright (c) 2024 Sinflower
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

#include <format>
#include <fstream>
#include <nlohmann/json.hpp>

#include "FileCoder.hpp"
#include "Types.hpp"

class WolfDataBase
{
public:
	WolfDataBase(const std::filesystem::path& filePath, const MagicNumber& magic, const WolfFileType& fileType, const bool& saveUncompressed = false, const uInts& seedIndices = {}) :
		m_filePath(filePath),
		m_magic(magic),
		m_fileType(fileType),
		m_saveUncompressed(saveUncompressed),
		m_seedIndices(seedIndices)
	{
	}

	virtual ~WolfDataBase() = default;

	bool Load(const std::filesystem::path& filePath)
	{
		m_filePath = filePath;

		if (m_filePath.empty())
			throw WolfRPGException(ERROR_TAG + "Trying to load with empty file path");

		g_activeFile = ::GetFileName(m_filePath);

		// Reset the static variable for Command
		Command::Command::s_v35 = false;

		FileCoder coder(m_filePath, FileCoder::Mode::READ, m_fileType, m_seedIndices);

		if (coder.IsEncrypted())
		{
			m_cryptHeader = coder.GetCryptHeader();
			coder.SetUTF8(m_magic.IsUTF8(m_cryptHeader));
		}
		else
			VERIFY_MAGIC(coder, m_magic);

		if (m_saveUncompressed)
			coder.DumpReader(getUncompressedPath());

		return load(coder);
	}

	bool Load(const Bytes& buffer)
	{
		if (buffer.empty())
			throw WolfRPGException(ERROR_TAG + "Trying to load with empty buffer");

		FileCoder coder(buffer, FileCoder::Mode::READ, m_fileType, m_seedIndices);

		if (coder.IsEncrypted())
		{
			m_cryptHeader = coder.GetCryptHeader();
			coder.SetUTF8(m_magic.IsUTF8(m_cryptHeader));
		}
		else
			VERIFY_MAGIC(coder, m_magic);

		return load(coder);
	}

	void Dump(const std::filesystem::path& outputPath, const std::filesystem::path& dataPath) const
	{
		// Reset the static variable for Command
		Command::Command::s_v35 = false;

		const std::filesystem::path fileName = ::GetFileName(m_filePath);

		g_activeFile           = fileName;

		// Get the relative path of the dataPath (absolute path to the data folder) and the parent path of the file, i.e., the difference between the two paths.
		// This results in the subfolder structure which are required to produce the correct output path.
		std::filesystem::path relativePath = std::filesystem::relative(std::filesystem::absolute(m_filePath).parent_path(), std::filesystem::absolute(dataPath));
		std::filesystem::path fullOutPath  = outputPath / relativePath / fileName;

		// Make sure the target folder exists
		CheckAndCreateDir(fullOutPath.parent_path());

		FileCoder coder(fullOutPath.wstring(), FileCoder::Mode::WRITE, m_fileType, m_seedIndices);
		dump(coder);
	}

	virtual void ToJson(const std::filesystem::path& outputPath) const
	{
		const std::filesystem::path fileName = ::GetFileNameNoExt(m_filePath);

		g_activeFile           = fileName;

		std::filesystem::path outputFilePath = outputPath / fileName;
		outputFilePath += ".json"; // Don't use replace_extension here in case the filename contains a dot

		std::ofstream out(outputFilePath);
		out << toJson().dump(4);

		out.close();
	}

	virtual void Patch(const std::filesystem::path& patchPath)
	{
		const std::filesystem::path fileName = ::GetFileNameNoExt(m_filePath);

		g_activeFile           = fileName;

		std::filesystem::path patchFilePath = patchPath / fileName;
		patchFilePath += ".json"; // Don't use replace_extension here in case the filename contains a dot

		if (!std::filesystem::exists(patchFilePath))
			throw WolfRPGException(ERROR_TAGW + L"Patch file not found: " + patchFilePath.wstring());

		nlohmann::ordered_json j;
		std::ifstream in(patchFilePath);
		in >> j;
		in.close();

		patch(j);
	}

	const std::filesystem::path& FileName() const
	{
		return m_filePath;
	}

	static void SetUncompressedPath(const std::filesystem::path& path)
	{
		s_uncompressedPath = path;
	}

protected:
	virtual bool load(FileCoder& coder)                 = 0;
	virtual void dump(FileCoder& coder) const           = 0;
	virtual nlohmann::ordered_json toJson() const       = 0;
	virtual void patch(const nlohmann::ordered_json& j) = 0;

private:
	std::filesystem::path getUncompressedPath() const
	{
		if (!s_uncompressedPath.empty())
		{
			// Make sure the target folder exists
			if (!std::filesystem::exists(s_uncompressedPath))
				std::filesystem::create_directories(s_uncompressedPath);
		}

		return s_uncompressedPath / ::GetFileName(m_filePath);
	}

protected:
	std::filesystem::path m_filePath;
	MagicNumber m_magic;
	bool m_saveUncompressed;
	WolfFileType m_fileType;
	uInts m_seedIndices = {};

	Bytes m_cryptHeader = {};

private:
	static inline std::filesystem::path s_uncompressedPath = "";
};