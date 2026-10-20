#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "Types.h"

// Compile the unchanged function body without unrelated protection backends.
class WolfPro
{
public:
	Key findDxArcKeyV1(std::vector<uint8_t>& byteData, const uint32_t& fileSize) const;
};

#include "find_dxarc_key_v1.inc"

static void require(bool condition, const char* message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		std::exit(1);
	}
}

int main()
{
	const WolfPro decoder;
	std::vector<uint8_t> shortFile;
	require(decoder.findDxArcKeyV1(shortFile, 0).empty(), "short file accepted");
	require(shortFile.empty(), "short file modified");

	for (uint8_t length : { 1, 2, 254, 255 })
	{
		std::vector<uint8_t> plain(DxArcKey::MIN_FILESIZE, 0xA5);
		plain[DxArcKey::SEED_OFFSET] = 0x26;
		plain[DxArcKey::KEY_LEN_OFFSET] = length;
		Key expected;
		for (uint32_t i = 0; i < length; ++i)
		{
			const uint8_t value = static_cast<uint8_t>(i + 37);
			expected.push_back(value);
			plain[DxArcKey::KEY_START_OFFSET + i * (DxArcKey::STEP_DIVISOR / length)] = value;
		}

		auto encrypted = plain;
		std::srand(plain[DxArcKey::SEED_OFFSET]);
		for (std::size_t i = DxArcKey::XOR_START_OFFSET; i < encrypted.size(); ++i)
			encrypted[i] ^= static_cast<uint8_t>(std::rand() >> DxArcKey::SHIFT);

		require(decoder.findDxArcKeyV1(encrypted, static_cast<uint32_t>(encrypted.size())) == expected,
		        "valid key changed");
		require(encrypted == plain, "valid data was not decrypted");
	}

	std::vector<uint8_t> zeroLength(DxArcKey::MIN_FILESIZE, 0x13);
	zeroLength[DxArcKey::KEY_LEN_OFFSET] = 0;
	const auto original = zeroLength;
	std::srand(4567);
	const int expectedNext = std::rand();
	std::srand(4567);
	std::cout << "checking zero key length\n" << std::flush;
	require(decoder.findDxArcKeyV1(zeroLength, static_cast<uint32_t>(zeroLength.size())).empty(),
	        "zero key length accepted");
	require(zeroLength == original, "invalid data modified");
	require(std::rand() == expectedNext, "invalid data changed random state");
	std::cout << "Pro1 key length tests passed\n";
}
