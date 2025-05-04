/*
 *  File: GameDat.h
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

// TODO: When integrating the decryption the filesize diff override might not be required anymore

#include "FileCoder.h"
#include "WolfDataBase.h"
#include "WolfRPGUtils.h"

#include <iostream>
#include <nlohmann\json.hpp>

class GameDat : public WolfDataBase
{
public:
	explicit GameDat(const tString& fileName = L"") :
		WolfDataBase(fileName, MAGIC_NUMBER, WolfFileType::GameDat, SEED_INDICES)
	{
		if (!fileName.empty())
			Load(fileName);
	}

	explicit GameDat(const Bytes& buffer, const bool& ignoreFilesizeDiff = false) :
		WolfDataBase(L"Game.dat", MAGIC_NUMBER, WolfFileType::GameDat, SEED_INDICES),
		m_ignoreFilesizeDiff(ignoreFilesizeDiff)
	{
		Load(buffer);
	}

	const tString& GetTitle() const
	{
		return m_title;
	}

	const tString& GetTitlePlus() const
	{
		return m_titlePlus;
	}

	const tString& GetFont() const
	{
		return m_font;
	}

	const tStrings& GetSubFonts() const
	{
		return m_subFonts;
	}

protected:
	bool load(FileCoder& coder) override
	{
		m_oldSize = coder.GetSize() + static_cast<uint32_t>(m_cryptHeader.size()) - 1;

		m_unknown1    = coder.ReadByteArray();
		m_stringCount = coder.ReadInt();

		// String 0
		m_title = coder.ReadString();

		// String 1
		m_magicString = coder.ReadString();

		if (m_magicString != MAGIC_STRING)
			throw WolfRPGException(ERROR_TAGW + L"Invalid magic string: \"" + m_magicString + L"\" expected: \"" + MAGIC_STRING + L"\"");

		// String 2
		m_decryptKey = coder.ReadByteArray();
		// String 3
		m_font = coder.ReadString();

		// Strings 4-6
		for (uint32_t i = 0; i < 3; i++)
			m_subFonts.push_back(coder.ReadString());

		// String 7
		m_defaultPCGraphic = coder.ReadString();

		// String 8
		if (m_stringCount >= 9)
			m_titlePlus = coder.ReadString();

		// Strings 9-13
		if (m_stringCount > 9)
		{
			m_roadImg    = coder.ReadString();
			m_gaugeImg   = coder.ReadString();
			m_startUpMsg = coder.ReadString();
			m_titleMsg   = coder.ReadString();
		}

		if (m_stringCount > 13)
			m_unknownString14 = coder.ReadString();

		m_fileSize = coder.ReadInt();

		if (!m_ignoreFilesizeDiff && (m_fileSize != m_oldSize))
			throw WolfRPGException(ERROR_TAG + std::format("Game.dat has different size than expected - expected: {} - got: {}", m_fileSize, m_oldSize));

		m_unknown2 = coder.Read();

		if (!coder.IsEof())
			throw WolfRPGException(ERROR_TAG + "Game.dat has more data than expected");

		return true;
	}

	void dump(FileCoder& coder) const
	{
		coder.Write(MAGIC_NUMBER);

		coder.WriteByteArray(m_unknown1);
		coder.WriteInt(m_stringCount);
		coder.WriteString(m_title);
		coder.WriteString(MAGIC_STRING);
		coder.WriteByteArray(m_decryptKey);
		coder.WriteString(m_font);
		for (const tString& font : m_subFonts)
			coder.WriteString(font);
		coder.WriteString(m_defaultPCGraphic);
		if (m_stringCount >= 9) coder.WriteString(m_titlePlus);

		if (m_stringCount > 9)
		{
			coder.WriteString(m_roadImg);
			coder.WriteString(m_gaugeImg);
			coder.WriteString(m_startUpMsg);
			coder.WriteString(m_titleMsg);
		}

		if (m_stringCount > 13)
			coder.WriteString(m_unknownString14);

		coder.WriteInt(calcNewSize());
		coder.Write(m_unknown2);
	}

	nlohmann::ordered_json toJson() const
	{
		nlohmann::ordered_json j;

		j["Title"]     = ToUTF8(m_title);
		j["TitlePlus"] = ToUTF8(m_titlePlus);

		if (m_stringCount > 9)
		{
			j["StartUpMsg"] = ToUTF8(m_startUpMsg);
			j["TitleMsg"]   = ToUTF8(m_titleMsg);
		}

		return j;
	}

	void patch(const nlohmann::ordered_json& j)
	{
		m_title     = ToUTF16(j["Title"]);
		m_titlePlus = ToUTF16(j["TitlePlus"]);

		if (m_stringCount > 9)
		{
			m_startUpMsg = ToUTF16(j["StartUpMsg"]);
			m_titleMsg   = ToUTF16(j["TitleMsg"]);
		}
	}

private:
	std::size_t calcNewSize() const
	{
		std::size_t size = 0;
		size += MAGIC_NUMBER.Size();
		size += m_unknown1.size() + 4;
		size += sizeof(m_stringCount);
		size += FileCoder::CalcStringSize(m_title) + 4;
		size += FileCoder::CalcStringSize(MAGIC_STRING) + 4;
		size += m_decryptKey.size() + 4;
		size += FileCoder::CalcStringSize(m_font) + 4;

		for (const tString& font : m_subFonts)
			size += FileCoder::CalcStringSize(font) + 4;

		size += FileCoder::CalcStringSize(m_defaultPCGraphic) + 4;

		if (m_stringCount >= 9) size += FileCoder::CalcStringSize(m_titlePlus) + 4;

		if (m_stringCount > 9)
		{
			size += FileCoder::CalcStringSize(m_roadImg) + 4;
			size += FileCoder::CalcStringSize(m_gaugeImg) + 4;
			size += FileCoder::CalcStringSize(m_startUpMsg) + 4;
			size += FileCoder::CalcStringSize(m_titleMsg) + 4;
		}

		if (m_stringCount > 13)
			size += FileCoder::CalcStringSize(m_unknownString14) + 4;

		size += sizeof(m_fileSize);

		size += m_unknown2.size();

		return size;
	}

private:
	Bytes m_unknown1           = {};
	uint32_t m_stringCount     = 0;
	tString m_title            = TEXT("");
	tString m_magicString      = TEXT("");
	Bytes m_decryptKey         = {};
	tString m_font             = TEXT("");
	tStrings m_subFonts        = {};
	tString m_defaultPCGraphic = TEXT("");
	tString m_titlePlus        = TEXT("");
	tString m_roadImg          = TEXT("");
	tString m_gaugeImg         = TEXT("");
	tString m_startUpMsg       = TEXT("");
	tString m_titleMsg         = TEXT("");
	tString m_unknownString14  = TEXT("");
	uint32_t m_fileSize        = 0;
	Bytes m_unknown2           = {};

	uint32_t m_oldSize = 0;

	bool m_ignoreFilesizeDiff = false;

	inline static const uInts SEED_INDICES{ 0, 8, 6 };
	inline static const MagicNumber MAGIC_NUMBER{ { 0x57, 0x00, 0x00, 0x4f, 0x4c, 0x00, 0x46, 0x4d, 0x00 }, 8 };
	inline static const tString MAGIC_STRING = L"0000-0000";
};