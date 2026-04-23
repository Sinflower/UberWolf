/*
 *  File: FileCoder.hpp
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

#include "FileAccess.hpp"
#include "Types.hpp"
#include "WolfRPGException.hpp"
#include "WolfRPGUtils.hpp"

#include "../WolfCrypt/WolfCrypt.hpp"
#include "../WolfCrypt/WolfDataDecrypt.hpp"

#include <array>
#include <filesystem>
#include <iostream>
#include <lz4/lz4.h>
#include <string>

#ifndef _WIN32
#include <iconv.h>
#endif

// TODO:
// - Create Wrapper class for reader / writer to have mode independent access object
// - Remove static headers

class MagicNumber
{
public:
	MagicNumber(const Bytes& data, const int32_t& utf8Idx = -1) :
		m_data(data),
		m_utf8Idx(utf8Idx)
	{
	}

	bool operator==(const Bytes& check) const
	{
		if (std::equal(m_data.begin(), m_data.end(), check.begin()))
			return true;

		if (m_utf8Idx != -1)
		{
			Bytes utf8Data      = m_data;
			utf8Data[m_utf8Idx] = 0x55;
			return std::equal(utf8Data.begin(), utf8Data.end(), check.begin());
		}
		else
			return false;
	}

	const Bytes& GetData() const
	{
		return m_data;
	}

	Bytes GetUTF8Data() const
	{
		Bytes utf8Data      = m_data;
		utf8Data[m_utf8Idx] = 0x55;
		return utf8Data;
	}

	std::size_t Size() const
	{
		return m_data.size();
	}

	bool IsUTF8(const Bytes& data) const
	{
		if (m_utf8Idx == -1) return false;

		return (data[m_utf8Idx] == 0x55);
	}

private:
	Bytes m_data;
	int32_t m_utf8Idx = -1;
};

class FileCoder
{
public:
	static constexpr uint32_t CRYPT_HEADER_SIZE = 10;

	enum class Mode
	{
		READ,
		WRITE
	};

public:
	// Disable Copy/Move constructor
	DISABLE_COPY_MOVE(FileCoder)

	FileCoder(const std::filesystem::path& filePath, const Mode& mode, const WolfFileType& fileType, const SeedIncides& seedIndices = {}) :
		m_mode(mode),
		m_seedIndices(seedIndices),
		m_fileType(fileType)
	{
		if (mode == Mode::READ)
		{
			m_reader.Open(filePath);
			load();
		}
		else if (mode == Mode::WRITE)
		{
			if (s_createBackup)
				CreateBackup(filePath);

			m_writer.Open(filePath);

			if (fileType != WolfFileType::Project && fileType != WolfFileType::Map)
				WriteByte(0);
		}
	}

	FileCoder(const Bytes& buffer, const Mode& mode, const WolfFileType& fileType, const SeedIncides& seedIndices = {}) :
		m_mode(mode),
		m_seedIndices(seedIndices),
		m_fileType(fileType)
	{
		if (mode != Mode::READ)
			throw WolfRPGException(ERROR_TAG + "FileCoder: Only READ mode is supported for buffer input.");

		if (buffer.empty())
			throw WolfRPGException(ERROR_TAG + "FileCoder: Buffer is empty.");

		m_reader.InitData(buffer);
		load();
	}

	FileCoder(const Mode& mode, const WolfFileType& fileType) :
		m_mode(mode),
		m_fileType(fileType)
	{
		if (mode == Mode::READ)
			throw WolfRPGException(ERROR_TAG + "FileCoder: READ mode requires a filename or buffer.");
	}

	void Unpack(const bool& seekBack = false)
	{
		const uint32_t startOffset = m_reader.GetOffset();
		const uint32_t decDataSize = m_reader.ReadUInt32();
		const uint32_t encDataSize = m_reader.ReadUInt32();

		Bytes decData(decDataSize + startOffset, 0);

		// lz4Unpack(m_reader.Get(), &decData[startOffset], encDataSize);
		int32_t decSize = LZ4_decompress_safe(reinterpret_cast<const char*>(m_reader.Get()), reinterpret_cast<char*>(&decData[startOffset]), encDataSize, decDataSize);

		if (decSize < 0)
			throw WolfRPGException(ERROR_TAG + "LZ4 decompression failed.");

		m_reader.Seek(0);
		std::memcpy(decData.data(), m_reader.Get(), startOffset); // Copy header

		m_reader.InitData(decData);

		if (seekBack)
			m_reader.Seek(startOffset);
	}

	void Pack()
	{
		const uint32_t dataSize = static_cast<uint32_t>(m_writer.GetSize());

		// Use LZ4_compressBound to allocate the correct maximum buffer size
		const int32_t maxDstSize = LZ4_compressBound(dataSize);
		std::vector<uint8_t> encData(maxDstSize, 0);

		int32_t encSize = LZ4_compress_default(reinterpret_cast<const char*>(m_writer.Get()), reinterpret_cast<char*>(encData.data()), dataSize, maxDstSize);

		if (encSize <= 0)
			throw WolfRPGException(std::format("{}LZ4 compression failed. Data size: {}", ERROR_TAG, dataSize));

		encData.resize(encSize);
		m_writer.Clear();

		m_writer.Write(dataSize);
		m_writer.Write(encSize);
		m_writer.Write(encData);
	}

	~FileCoder()
	{
	}

	const uint32_t& GetSize() const
	{
		return m_reader.GetSize();
	}

	bool WasEncrypted() const
	{
		return m_wasEncrypted;
	}

	void Seek(const int32_t& pos)
	{
		if (m_mode == Mode::READ)
		{
			uint32_t o = m_reader.GetOffset() + pos;
			m_reader.Seek(o);
		}
	}

	bool IsEof() const
	{
		if (m_mode == Mode::READ)
			return m_reader.IsEoF();

		return false;
	}

	Bytes Read(const std::size_t& size = -1)
	{
		Bytes data;

		if (size != -1)
			data.resize(size);
		else
		{
			uint32_t remainingSize = m_reader.GetSize() - m_reader.GetOffset();
			data.resize(remainingSize);
		}

		m_reader.ReadBytesVec(data);

		return data;
	}

	uint8_t ReadByte()
	{
		return m_reader.ReadUInt8();
	}

	uint32_t ReadInt()
	{
		return m_reader.ReadUInt32();
	}

	tString ReadString()
	{
		uint32_t size = ReadInt();

		if (size == 0)
			throw WolfRPGException(ERROR_TAG + "Zero length string encountered.");

		Bytes data = Read(size);

		if (s_isUTF8)
		{
			std::string str = std::string(reinterpret_cast<const char*>(data.data()), data.size() - ((data.back() == 0x0) ? 1 : 0));
			return ToUTF16(str);
		}
		else
			return sjis2utf8(data);
	}

	Bytes ReadByteArray()
	{
		uint32_t size = ReadInt();
		Bytes data;

		for (uint32_t i = 0; i < size; i++)
			data.push_back(ReadByte());

		return data;
	}

	uInts ReadIntArray()
	{
		uint32_t size = ReadInt();
		uInts data;

		for (uint32_t i = 0; i < size; i++)
			data.push_back(ReadInt());

		return data;
	}

	tStrings ReadStringArray()
	{
		uint32_t size = ReadInt();
		tStrings data;

		for (uint32_t i = 0; i < size; i++)
			data.push_back(ReadString());

		return data;
	}

	bool Verify(const Bytes& vData)
	{
		Bytes data = Read(vData.size());
		if (std::equal(vData.begin(), vData.end(), data.begin()))
			return true;

		return false;
	}

	bool Verify(const MagicNumber& magicNumber)
	{
		Bytes data = Read(magicNumber.Size());
		if (magicNumber == data)
		{
			s_isUTF8 = magicNumber.IsUTF8(data);
			return true;
		}

		return false;
	}

	void SetUTF8(const bool& isUTF8)
	{
		s_isUTF8 = isUTF8;
	}

	void Skip(const uint32_t& size)
	{
		m_reader.Skip(size);
	}

	void Write(const Bytes& data)
	{
		m_writer.WriteBytesVec(data);
	}

	void Write(const MagicNumber& mn)
	{
		if (s_isUTF8)
			Write(mn.GetUTF8Data());
		else
			Write(mn.GetData());
	}

	void WriteByte(const uint8_t& data)
	{
		m_writer.Write(data);
	}

	void DumpReader(const std::filesystem::path& dumpFilePath)
	{
		m_reader.DumpToFile(dumpFilePath);
	}

#ifdef BIT_64
	// For 64 bit an additional method is required to properly handle size_t inputs
	void WriteInt(const std::size_t& data)
	{
		WriteInt(static_cast<uint32_t>(data));
	}
#endif

	void WriteInt(const uint32_t& data)
	{
		m_writer.Write(data);
	}

	void WriteString(const tString& wstr)
	{
		Bytes str;

		if (s_isUTF8)
		{
			std::string s = ToUTF8(wstr);
			str           = Bytes(s.begin(), s.end());
			str.push_back(0x0);
		}
		else
			str = utf82sjis(wstr);

		WriteInt(static_cast<uint32_t>(str.size()));
		Write(str);
	}

	void WriteByteArray(const Bytes& data)
	{
		WriteInt(static_cast<uint32_t>(data.size()));
		for (uint8_t byte : data)
			WriteByte(byte);
	}

	void WriteIntArray(const uInts& data)
	{
		WriteInt(static_cast<uint32_t>(data.size()));
		for (uint32_t uint : data)
			WriteInt(uint);
	}

	void WriteStringArray(const tStrings& strs)
	{
		WriteInt(static_cast<uint32_t>(strs.size()));
		for (tString str : strs)
			WriteString(str);
	}

	void WriteCoder(const FileCoder& coder)
	{
		m_writer.Write(coder.m_writer.GetBuffer());
	}

	static bool IsUTF8()
	{
		return s_isUTF8;
	}

	static std::size_t CalcStringSize(const tString& str)
	{
		if (s_isUTF8)
			return ToUTF8(str).size() + 1;
		else
			return utf82sjis(str).size();
	}

private:
	void cryptDatV1(Bytes& data, const SeedIncides& seeds)
	{
		wolf::crypt::datadecrypt::v2_0::decryptData(data, seeds);
	}

	void cryptDatV2(Bytes& data)
	{
		wolf::crypt::CryptData cd = wolf::crypt::datadecrypt::v3_3::decryptData(data, m_seedIndices);
		data.assign(cd.gameDatBytes.begin(), cd.gameDatBytes.end());
	}

	void cryptProj(Bytes& data)
	{
		wolf::crypt::rng::msvc_srand(s_projKey);

		for (uint8_t& byte : data)
			byte ^= static_cast<uint8_t>(wolf::crypt::rng::msvc_rand());
	}

#ifdef _WIN32
	static tString sjis2utf8(const Bytes& sjis)
	{
		const char* pSJIS = reinterpret_cast<const char*>(sjis.data());
		int sjisSize      = MultiByteToWideChar(932, 0, pSJIS, -1, NULL, 0);

		wchar_t* pUTF8 = new wchar_t[sjisSize + 1]();
		MultiByteToWideChar(932, 0, pSJIS, -1, pUTF8, sjisSize);
		tString utf8(pUTF8);
		delete[] pUTF8;
		return utf8;
	}

	static Bytes utf82sjis(const tString& utf8)
	{
		// Empty strings are length 1 with terminating 0
		if (utf8.empty()) return Bytes(1, 0);

		int utf8Size = WideCharToMultiByte(932, 0, &utf8[0], static_cast<int32_t>(utf8.size()), NULL, 0, NULL, NULL);
		Bytes sjis(utf8Size + 1, 0);
		WideCharToMultiByte(932, 0, &utf8[0], static_cast<int32_t>(utf8.size()), reinterpret_cast<char*>(sjis.data()), utf8Size, NULL, NULL);
		return sjis;
	}
#else
	static tString sjis2utf8(const Bytes& sjis)
	{
		iconv_t cd = iconv_open("WCHAR_T", "SHIFT-JIS");
		if (cd == (iconv_t)-1)
			throw WolfRPGException("iconv_open failed");

		std::size_t inBytesLeft  = sjis.size();
		std::size_t outBytesLeft = inBytesLeft * sizeof(wchar_t);

		std::wstring output(outBytesLeft / sizeof(wchar_t), L'\0');

		char* pInBuf  = const_cast<char*>(reinterpret_cast<const char*>(sjis.data()));
		char* pOutBuf = reinterpret_cast<char*>(output.data());

		size_t result = iconv(cd, &pInBuf, &inBytesLeft, &pOutBuf, &outBytesLeft);
		if (result == static_cast<std::size_t>(-1))
		{
			iconv_close(cd);
			throw WolfRPGException("iconv conversion failed");
		}

		// Resize to actual number of wchar_t elements present
		size_t bytesWritten = output.size() * sizeof(wchar_t) - outBytesLeft;
		output.resize(bytesWritten / sizeof(wchar_t));
		output.pop_back(); // Remove null terminator added by iconv

		iconv_close(cd);
		return output;
	}

	static Bytes utf82sjis(const tString& utf8)
	{
		iconv_t cd = iconv_open("SHIFT-JIS", "WCHAR_T");
		if (cd == (iconv_t)-1)
			throw WolfRPGException("iconv_open failed");

		std::size_t inBytesLeft  = utf8.size() * sizeof(wchar_t);
		std::size_t outBytesLeft = inBytesLeft * 4; // worst case size
		std::vector<char> output(outBytesLeft, 0);

		char* pInBuf  = reinterpret_cast<char*>(const_cast<wchar_t*>(utf8.data()));
		char* pOutBuf = output.data();

		while (inBytesLeft > 0)
		{
			std::size_t result = iconv(cd, &pInBuf, &inBytesLeft, &pOutBuf, &outBytesLeft);

			if (result == static_cast<std::size_t>(-1))
			{
				if (errno == E2BIG)
				{
					// Enlarge buffer and retry
					std::size_t used = output.size() - outBytesLeft;
					output.resize(output.size() * 2);
					pOutBuf      = output.data() + used;
					outBytesLeft = output.size() - used;
				}
				else
				{
					iconv_close(cd);
					throw WolfRPGException("iconv conversion failed");
				}
			}
		}

		Bytes converted(output.data(), output.data() + (output.size() - outBytesLeft));

		// Add null terminator
		converted.push_back(0x0);

		iconv_close(cd);
		return converted;
	}
#endif

	void decryptV2_0(const uint8_t& indicator)
	{
		if (indicator == 0x0)
			return;

		Bytes header(CRYPT_HEADER_SIZE);
		header[0] = indicator;

		for (uint32_t i = 1; i < CRYPT_HEADER_SIZE; i++)
			header[i] = ReadByte();

		SeedIncides seeds = { 0, 0, 0 };
		for (size_t i = 0; i < m_seedIndices.size(); i++)
			seeds[i] = header[m_seedIndices[i]];

		m_wasEncrypted = true;

		Bytes data = Read();
		cryptDatV1(data, seeds);

		m_reader.InitData(data);
	}

	void decryptV3_1()
	{
		uint8_t indicator = ReadByte();

		if (m_fileType == WolfFileType::DataBase)
		{
			if (m_reader.At(1) != 0x50 || m_reader.At(5) != 0x54 || m_reader.At(7) != 0x4B)
				return;
		}

		decryptV2_0(indicator);

		m_wasEncrypted = true;
		s_isUTF8       = true;

		// Skip 5 bytes to get to the key size
		m_reader.Skip(5);
		uint32_t keySize = m_reader.ReadUInt32();

		if (m_fileType == WolfFileType::GameDat)
		{
			// Skip over the key it since it's not needed here
			m_reader.Skip(keySize);
			return;
		}

		int8_t projKey = m_reader.ReadInt8();

		if (s_projKey == -1)
			s_projKey = projKey;

		m_reader.Skip(keySize - 1);
	}

	void decryptV3_3()
	{
		Bytes data = Read();
		cryptDatV2(data);

		m_wasEncrypted = true;
		s_isUTF8       = true;

		m_reader.InitData(data);
		m_reader.Skip(143);

		s_projKey = data[0x14];
	}

	void decryptV3_5()
	{
		m_reader.Seek(0);
		Bytes data = Read();
		if (!wolf::crypt::datadecrypt::v3_5::decryptData(data, m_fileType))
			throw WolfRPGException(ERROR_TAG + "Failed to decrypt ProV3 data.");

		// wasEncrypted is not set here because the decryption function adds the required headers
		s_isUTF8 = true;

		m_reader.InitData(data);
		// ¯\_(ツ)_/¯
		s_projKey = 0;
	}

	void load()
	{
		if (m_fileType == WolfFileType::Project)
		{
			if (s_projKey != -1)
			{
				Bytes data = Read();
				cryptProj(data);
				m_reader.InitData(data);
			}

			return;
		}

		if (m_seedIndices.empty() && (m_fileType != WolfFileType::Map)) return;

		if (m_reader.At(1) == 0x50)
		{
			uint8_t cryptVersion = m_reader.At(5);
			if (cryptVersion < 0x55)
			{
				decryptV3_1();
				return;
			}
			else if (cryptVersion < 0x57)
			{
				decryptV3_3();
				return;
			}
			else
				decryptV3_5();
		}

		if (m_fileType == WolfFileType::Map)
		{
			if (m_reader.At(20) < 0x65) return;

			// The first 25 bytes are the header -- TODO: remove magic numbers (here and elsewhere)
			m_reader.Seek(25);
			Unpack();
			return;
		}
		else
		{
			uint8_t indicator = ReadByte();

			if (indicator != 0x0 && m_fileType != WolfFileType::DataBase)
				decryptV2_0(indicator);
		}

		if (m_fileType == WolfFileType::DataBase)
		{
			if (m_reader.At(10) == 0xC4)
			{
				m_reader.Seek(11);
				Unpack();
				m_reader.Seek(1);
				return;
			}
		}
	}

private:
	bool m_wasEncrypted = false;
	Mode m_mode;
	SeedIncides m_seedIndices = {};
	WolfFileType m_fileType;

	FileReader m_reader = {};
	FileWriter m_writer = {};

	inline static bool s_isUTF8       = false;
	inline static uint32_t s_projKey  = -1;
	inline static bool s_createBackup = false;
};
