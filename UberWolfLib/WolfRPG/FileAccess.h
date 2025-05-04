/*
 *  File: FileAccess.h
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
#include <windows.h>

#include <codecvt>
#include <exception>
#include <fstream>
#include <string>
#include <vector>

namespace fileAccessUtils
{
inline std::wstring s2ws(const std::string& str)
{
	return std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(str);
}

inline std::string ws2s(const std::wstring& wstr)
{
	return std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(wstr);
}
} // namespace fileAccessUtils

class FileWalkerException : public std::exception
{
public:
	explicit FileWalkerException(const std::string& what) :
		m_what(what) {}

	explicit FileWalkerException(const std::wstring& what) :
		m_what(fileAccessUtils::ws2s(what)) {}

	virtual ~FileWalkerException() noexcept {}

	virtual const char* what() const throw()
	{
		return m_what.c_str();
	}

private:
	std::string m_what;
};

class FileReader
{
public:
	FileReader() {}
	FileReader(const std::string& filename, const DWORD& startOffset = -1) :
		FileReader(fileAccessUtils::s2ws(filename), startOffset) {}

	FileReader(const std::wstring& filename, const DWORD& startOffset = -1)
	{
		Open(filename, startOffset);
	}

	FileReader(const std::vector<BYTE>& dataVec)
	{
		InitData(dataVec);
	}

	// Disable copy constructor and copy assignment operator
	FileReader(const FileReader&)            = delete;
	FileReader& operator=(const FileReader&) = delete;

	~FileReader()
	{
		close();
	}

	void InitData(const std::vector<BYTE>& dataVec)
	{
		close();

		m_offset  = 0;
		m_dataVec = dataVec;
		m_pData   = m_dataVec.data();
		m_size    = static_cast<DWORD>(m_dataVec.size());
		m_init    = true;
	}

	void Open(const std::wstring& filename, const DWORD& startOffset = -1)
	{
		m_pFile = CreateFileW(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (m_pFile == nullptr)
			throw(FileWalkerException(L"Failed to open file: " + filename));

		m_pFileMap = CreateFileMappingW(m_pFile, NULL, PAGE_READONLY, 0, 0, NULL);
		if (m_pFileMap == nullptr)
		{
			CloseHandle(m_pFile);
			throw(FileWalkerException(L"Failed to create file mapping for: " + filename));
		}

		m_pMapView = MapViewOfFile(m_pFileMap, FILE_MAP_READ, 0, 0, 0);
		if (m_pMapView == nullptr)
		{
			CloseHandle(m_pFileMap);
			CloseHandle(m_pFile);
			throw(FileWalkerException(L"Failed to create map view of file: " + filename));
		}

		m_pData = reinterpret_cast<PBYTE>(m_pMapView);

		m_size = GetFileSize(m_pFile, 0);

		if (startOffset != -1)
			m_offset = startOffset;

		m_init = true;
	}

	bool IsEoF() const
	{
		return m_offset >= m_size;
	}

	uint64_t ReadUInt64()
	{
		return read<uint64_t>();
	}

	uint32_t ReadUInt32()
	{
		return read<uint32_t>();
	}

	uint16_t ReadUInt16()
	{
		return read<uint16_t>();
	}

	uint8_t ReadUInt8()
	{
		return read<uint8_t>();
	}

	int64_t ReadInt64()
	{
		return read<int64_t>();
	}

	int32_t ReadInt32()
	{
		return read<int32_t>();
	}

	int16_t ReadInt16()
	{
		return read<int16_t>();
	}

	int8_t ReadInt8()
	{
		return read<int8_t>();
	}

	template<std::size_t S>
	void ReadBytesArr(std::array<BYTE, S>& buffer, const DWORD& size = -1)
	{
		if (size == -1)
			ReadBytes(buffer.data(), static_cast<DWORD>(buffer.size()));
		else if (size <= buffer.size())
			ReadBytes(buffer.data(), size);
		else
			throw(FileWalkerException("ReadBytesArr: size is larger than buffer size"));
	}

	void ReadBytesVec(std::vector<BYTE>& buffer, const DWORD& size = -1)
	{
		if (size == -1)
			ReadBytes(buffer.data(), static_cast<DWORD>(buffer.size()));
		else if (size <= buffer.size())
			ReadBytes(buffer.data(), size);
		else
			throw(FileWalkerException("ReadBytesVec: size is larger than buffer size"));
	}

	void ReadBytes(LPVOID pBuffer, std::size_t& size)
	{
		ReadBytes(pBuffer, static_cast<DWORD>(size));
	}

	void ReadBytes(LPVOID pBuffer, const DWORD& size)
	{
		if (!m_init)
			throw(FileWalkerException("FileWalker not initialized"));

		if (m_offset + size > m_size)
			throw(FileWalkerException("ReadBytes: Attempted to read past end of file"));

		std::memcpy(pBuffer, m_pData + m_offset, size);
		m_offset += size;
	}

	template<typename T>
	void ReadVec(std::vector<T>& buffer, const DWORD& dwordCnt = -1)
	{
		if (dwordCnt == -1)
			ReadBytes(buffer.data(), buffer.size() * sizeof(T));
		else if (dwordCnt <= buffer.size())
			ReadBytes(buffer.data(), dwordCnt * sizeof(T));
		else
			throw(FileWalkerException("ReadVec: size is larger than buffer size"));
	}

	void Seek(const DWORD& offset)
	{
		if (!m_init)
			throw(FileWalkerException("FileWalker not initialized"));

		if (offset > m_size)
			throw(FileWalkerException("Seek: Attempted to seek past end of file"));

		m_offset = offset;
	}

	void Skip(const DWORD& size)
	{
		if (!m_init)
			throw(FileWalkerException("FileWalker not initialized"));

		if (m_offset + size > m_size)
			throw(FileWalkerException("Skip: Attempted to skip past end of file"));

		m_offset += size;
	}

	const PBYTE Get() const
	{
		return m_pData + m_offset;
	}

	const DWORD& GetOffset() const
	{
		return m_offset;
	}

	const DWORD& GetSize() const
	{
		return m_size;
	}

	bool IsEndOfFile()
	{
		return m_offset >= m_size;
	}

	BYTE At(const DWORD& offset) const
	{
		if (!m_init)
			throw(FileWalkerException("FileWalker not initialized"));

		if (offset >= m_size)
			throw(FileWalkerException("At: Attempted to read past end of file"));

		return *(m_pData + offset);
	}

private:
	void close()
	{
		if (m_pMapView != nullptr)
		{
			UnmapViewOfFile(m_pMapView);
			m_pMapView = nullptr;
		}

		if (m_pFileMap != nullptr)
		{
			CloseHandle(m_pFileMap);
			m_pFileMap = nullptr;
		}

		if (m_pFile != nullptr)
		{
			CloseHandle(m_pFile);
			m_pFile = nullptr;
		}

		m_offset = 0;
	}

	template<typename T>
	T read()
	{
		if (!m_init)
			throw(FileWalkerException("FileWalker not initialized"));

		if (m_offset + sizeof(T) > m_size)
			throw(FileWalkerException("read: End of file reached"));

		T value = *(reinterpret_cast<T*>(m_pData + m_offset));
		m_offset += sizeof(T);
		return value;
	}

private:
	bool m_init       = false;
	HANDLE m_pFile    = nullptr;
	LPVOID m_pMapView = nullptr;
	HANDLE m_pFileMap = nullptr;

	PBYTE m_pData = nullptr;

	DWORD m_offset = 0;
	DWORD m_size   = 0;

	std::vector<BYTE> m_dataVec = {};
};

class FileWriterException : public std::exception
{
public:
	explicit FileWriterException(const std::string& what) :
		m_what(what) {}

	explicit FileWriterException(const std::wstring& what) :
		m_what(fileAccessUtils::ws2s(what)) {}

	virtual ~FileWriterException() noexcept {}

	virtual const char* what() const throw()
	{
		return m_what.c_str();
	}

private:
	std::string m_what;
};

class FileWriter
{
public:
	FileWriter() {}
	FileWriter(const std::string& filename) :
		FileWriter(fileAccessUtils::s2ws(filename)) {}

	FileWriter(const std::wstring& filename)
	{
		Open(filename);
	}

	// Disable copy constructor and copy assignment operator
	FileWriter(const FileWriter&)            = delete;
	FileWriter& operator=(const FileWriter&) = delete;

	void Open(const std::wstring& filename)
	{
		m_file = std::fstream(filename, std::ios::out | std::ios::binary);
		if (!m_file.is_open())
			throw(FileWriterException("Failed to open file"));
		m_bufferMode = false;
	}

	~FileWriter()
	{
		if (m_file.is_open())
			m_file.close();
	}

	PBYTE Get()
	{
		return m_buffer.data();
	}

	const std::vector<BYTE>& GetBuffer() const
	{
		return m_buffer;
	}

	void SetAt(const uint64_t& offset, const BYTE& value)
	{
		if (offset < m_buffer.size())
			m_buffer[offset] = value;
		else
			throw(FileWriterException("SetAt: offset is larger than buffer size"));
	}

	const uint64_t& GetSize() const
	{
		return m_size;
	}

	void Clear()
	{
		if (!m_bufferMode)
			throw(FileWriterException("Clear: FileWriter not in buffer mode"));

		m_buffer.clear();
		m_size = 0;
	}

	void WriteToFile(const std::string& filename)
	{
		WriteToFile(fileAccessUtils::s2ws(filename));
	}

	void WriteToFile(const std::wstring& filename)
	{
		if (m_bufferMode)
		{
			std::ofstream file(filename, std::ios::out | std::ios::binary);
			file.write(reinterpret_cast<const char*>(m_buffer.data()), m_buffer.size());
		}
	}

	template<typename T>
	void Write(const T& data)
	{
		write<T>(data);
	}

	template<typename T>
	void Write(const std::vector<T>& data)
	{
		for (const T& d : data)
			write<T>(d);
	}

	template<std::size_t S>
	void WriteBytesArr(const std::array<BYTE, S>& buffer, const DWORD& size = -1)
	{
		if (size == -1)
			WriteBytes(buffer.data(), buffer.size());
		else if (size <= buffer.size())
			WriteBytes(buffer.data(), size);
		else
			throw(FileWriterException("WriteBytesArr: size is larger than buffer size"));
	}

	void WriteBytesVec(const std::vector<BYTE>& buffer, const DWORD& size = -1)
	{
		if (size == -1)
			WriteBytes(buffer.data(), buffer.size());
		else if (size <= buffer.size())
			WriteBytes(buffer.data(), size);
		else
			throw(FileWriterException("WriteBytesVec: size is larger than buffer size"));
	}

	void WriteBytes(LPCVOID pBuffer, const int& size)
	{
		WriteBytes(pBuffer, static_cast<DWORD>(size));
	}

	void WriteBytes(LPCVOID pBuffer, const std::size_t& size)
	{
		WriteBytes(pBuffer, static_cast<DWORD>(size));
	}

	void WriteBytes(LPCVOID pBuffer, const DWORD& size)
	{
		m_size += size;
		if (m_bufferMode)
			m_buffer.insert(m_buffer.end(), static_cast<const BYTE*>(pBuffer), static_cast<const BYTE*>(pBuffer) + size);
		else if (m_file.is_open())
			m_file.write(static_cast<const char*>(pBuffer), size);
		else
			throw(FileWriterException("FileWriter not initialized"));
	}

private:
	template<typename T>
	void write(const T& data)
	{
		m_size += sizeof(T);
		if (m_bufferMode)
			m_buffer.insert(m_buffer.end(), reinterpret_cast<const BYTE*>(&data), reinterpret_cast<const BYTE*>(&data) + sizeof(T));
		else if (m_file.is_open())
			m_file.write(reinterpret_cast<const char*>(&data), sizeof(T));
		else
			throw(FileWriterException("FileWriter not initialized"));
	}

private:
	bool m_bufferMode          = true;
	uint64_t m_size            = 0;
	std::fstream m_file        = {};
	std::vector<BYTE> m_buffer = {};
};