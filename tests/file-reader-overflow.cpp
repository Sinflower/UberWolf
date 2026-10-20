// Run from an x64 Native Tools prompt:
// clang-cl /std:c++20 /EHsc /DNOMINMAX tests/file-reader-overflow.cpp /Fe:reader-overflow.exe
#include <array>
#include <cstdint>
#include <cstring>
#include <format>
#include <iostream>
#include <limits>
#include <stdexcept>

#include "../UberWolfLib/WolfRPG/FileAccess.hpp"

static void Check(bool value)
{
	if (!value)
		throw std::runtime_error("FileReader bounds check failed");
}

template<typename F>
static void Reject(FileReader& reader, F operation)
{
	const auto offset = reader.GetOffset();
	bool rejected = false;
	try
	{
		operation();
	}
	catch (const FileWalkerException&)
	{
		rejected = true;
	}
	Check(rejected);
	Check(reader.GetOffset() == offset);
}

int main(int argc, char** argv)
{
	try
	{
		FileReader reader(std::vector<uint8_t>{1, 2, 3, 4});
		reader.Seek(1);
		const auto maximum = std::numeric_limits<uint32_t>::max();
		if (argc == 1 || std::string_view(argv[1]) == "skip")
			Reject(reader, [&] { reader.Skip(maximum); });
		if (argc == 1 || std::string_view(argv[1]) == "read")
		{
			uint8_t byte = 0xA5;
			Reject(reader, [&] { reader.ReadBytes(&byte, maximum); });
			Check(byte == 0xA5);
		}
		if (argc == 1 || std::string_view(argv[1]) == "size")
		{
			uint8_t byte = 0xA5;
			const std::size_t tooLarge = static_cast<std::size_t>(maximum) + 1;
			Reject(reader, [&] { reader.ReadBytes(&byte, tooLarge); });
			Check(byte == 0xA5);
		}
		if (argc > 1)
			return 0;

		FileReader unopened;
		uint8_t byte = 0xA5;
		Reject(unopened, [&] { unopened.ReadBytes(&byte, uint32_t{1}); });
		Reject(unopened, [&] { unopened.Skip(1); });

		reader.Skip(3);
		Check(reader.IsEndOfFile());
		reader.Skip(0);
		Reject(reader, [&] { reader.Skip(1); });
		Reject(reader, [&] { reader.ReadBytes(&byte, uint32_t{1}); });
		Check(byte == 0xA5);
		reader.Seek(1);
		std::array<uint8_t, 3> bytes{};
		reader.ReadBytes(bytes.data(), bytes.size());
		Check((bytes == std::array<uint8_t, 3>{2, 3, 4}));
		Check(reader.IsEndOfFile());
		reader.ReadBytes(&byte, std::size_t{0});
		Check(byte == 0xA5);
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << error.what() << '\n';
		return 1;
	}
}
