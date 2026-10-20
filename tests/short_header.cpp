#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <windows.h>

#include "DXLib/DXArchive.h"
#include "Types.h"

// Compile the unchanged function body without the unrelated archive backends.
class WolfDec
{
public:
	uint16_t getCryptVersion(const tString& filePath) const;
};

#define ERROR_LOG tcerr
#include "get_crypt_version.inc"

int main()
{
	const auto path = std::filesystem::path("header.wolf");
	const WolfDec decoder;
	auto require = [](bool condition, const char* message) {
		if (!condition)
		{
			std::cerr << message << '\n';
			std::exit(1);
		}
	};
	auto write = [&](const DARC_HEAD& header, std::size_t size) {
		std::ofstream file(path, std::ios::binary | std::ios::trunc);
		file.write(reinterpret_cast<const char*>(&header), size);
		require(static_cast<bool>(file), "fixture write failed");
	};

	std::filesystem::remove(path);
	require(decoder.getCryptVersion(path.wstring()) == 0, "missing file accepted");

	DARC_HEAD header{};
	header.Head = DXA_HEAD;
	header.Flags = 0x012C0000;
	write(header, sizeof(header));
	require(decoder.getCryptVersion(path.wstring()) == 0x012C, "complete header rejected");

	header.Head = 0;
	write(header, sizeof(header));
	require(decoder.getCryptVersion(path.wstring()) == 0, "invalid signature accepted");
	header.Head = DXA_HEAD;

	// Flags are present: the old code incorrectly accepts this truncated header.
	write(header, sizeof(header) - 1);
	require(decoder.getCryptVersion(path.wstring()) == 0, "truncated header accepted");

	for (std::size_t size = 0; size < sizeof(header); ++size)
	{
		write(header, size);
		require(decoder.getCryptVersion(path.wstring()) == 0, "short header accepted");
	}
	std::filesystem::remove(path);
	std::cout << "short header tests passed\n";
}
