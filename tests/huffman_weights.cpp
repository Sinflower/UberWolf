// Run from an x64 Native Tools prompt:
// clang-cl /std:c++20 /EHsc /clang:-fsanitize=bounds /clang:-fno-sanitize-recover=all tests/huffman_weights.cpp 3rdParty/DXLib/Huffman.cpp /Fe:huffman_weights.exe

#include <cassert>
#include <cstddef>
#include <vector>

#include "../3rdParty/DXLib/Huffman.h"

int main()
{
    const std::vector<std::vector<unsigned char>> inputs = {
        {0x41},
        std::vector<unsigned char>(1024, 0x41),
        [] {
            std::vector<unsigned char> bytes(1024);
            for (std::size_t i = 0; i < bytes.size(); ++i)
                bytes[i] = static_cast<unsigned char>(i);
            return bytes;
        }(),
    };

    for (auto input : inputs)
    {
        // The legacy codec reads ahead; padding keeps this regression about the weight table.
        std::vector<unsigned char> encoded(input.size() * 2 + 4096);
        const auto encodedSize = Huffman_Encode(input.data(), input.size(), encoded.data());
        assert(encodedSize > 0 && encodedSize < encoded.size());
        assert(Huffman_Decode(encoded.data(), nullptr) == input.size());

        std::vector<unsigned char> decoded(input.size());
        assert(Huffman_Decode(encoded.data(), decoded.data()) == input.size());
        assert(decoded == input);
    }
}
