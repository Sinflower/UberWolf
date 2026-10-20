// Run from an x64 Native Tools prompt:
// clang-cl /std:c++20 /EHsc /DNOMINMAX /clang:-fsanitize=alignment /clang:-fno-sanitize-recover=alignment tests/file-reader-alignment.cpp /Fe:reader-alignment.exe
#include <array>
#include <cstdint>
#include <cstring>
#include <format>
#include <iostream>
#include <limits>
#include <stdexcept>

#include "../UberWolfLib/WolfRPG/FileAccess.hpp"

template<typename T>
static void Check(T (FileReader::*read)(), T expected)
{
	for (uint32_t offset = 0; offset < 8; ++offset)
	{
		std::vector<uint8_t> bytes(offset + sizeof(T));
		std::memcpy(bytes.data() + offset, &expected, sizeof(T));
		FileReader reader(bytes);
		reader.Seek(offset);
		if ((reader.*read)() != expected || reader.GetOffset() != bytes.size())
			throw std::runtime_error("Incorrect value or offset after reading integer");

		bool rejected = false;
		try
		{
			(reader.*read)();
		}
		catch (const FileWalkerException&)
		{
			rejected = true;
		}
		if (!rejected || reader.GetOffset() != bytes.size())
			throw std::runtime_error("Read past end was not rejected");
	}

	FileReader unopened;
	try
	{
		(unopened.*read)();
	}
	catch (const FileWalkerException&)
	{
		return;
	}
	throw std::runtime_error("Uninitialized reader was not rejected");
}

int main()
{
	try
	{
		Check(&FileReader::ReadUInt64, uint64_t{0xFEDCBA9876543210});
		Check(&FileReader::ReadUInt32, uint32_t{0xFEDCBA98});
		Check(&FileReader::ReadUInt16, uint16_t{0xFEDC});
		Check(&FileReader::ReadUInt8, uint8_t{0xFE});
		Check(&FileReader::ReadInt64, int64_t{-0x123456789ABCDE});
		Check(&FileReader::ReadInt32, int32_t{-0x12345678});
		Check(&FileReader::ReadInt16, int16_t{-0x1234});
		Check(&FileReader::ReadInt8, int8_t{-0x12});
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << error.what() << '\n';
		return 1;
	}
}
