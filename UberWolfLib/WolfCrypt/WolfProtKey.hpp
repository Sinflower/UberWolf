/*
 *  File: WolfProtKey.hpp
 *  Copyright (c) 2026 Sinflower
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
#include <random>
#include <stdexcept>
#include <vector>

#include "WolfAes.hpp"
#include "WolfCrypt.hpp"
#include "WolfCryptTypes.hpp"
#include "WolfCryptUtils.hpp"
#include "WolfRng.hpp"

namespace wolf::crypt::protkey
{
namespace v2
{
static constexpr uint32_t ENCRYPTED_KEY_SIZE = 128;

inline bool validateKey(const std::vector<uint8_t> &key, const std::array<uint8_t, ENCRYPTED_KEY_SIZE> &tarKey)
{
	if (key.empty()) return false;

	const uint32_t keyLen = static_cast<uint32_t>(key.size());

	if (keyLen > ENCRYPTED_KEY_SIZE)
		throw std::runtime_error("Key is too long");

	std::array<uint8_t, ENCRYPTED_KEY_SIZE> keyBytes;

	for (uint32_t i = 0; i < keyLen; i++)
		keyBytes[i] = key[i];

	for (uint32_t i = 0; i < ENCRYPTED_KEY_SIZE; i++)
		keyBytes[i] = i / keyLen + key[i % keyLen];

	// Compare the target key and the generated key
	return std::equal(keyBytes.begin(), keyBytes.end(), tarKey.begin());
}

inline std::vector<uint8_t> findKey(const std::array<uint8_t, ENCRYPTED_KEY_SIZE> &encKey)
{
	const uint32_t MIN_KEY_LEN = 4;
	for (uint32_t i = MIN_KEY_LEN; i < ENCRYPTED_KEY_SIZE; i++)
	{
		std::vector<uint8_t> key(i, 0);
		std::copy(encKey.begin(), encKey.begin() + i, key.begin());

		if (validateKey(key, encKey))
			return key;
	}

	return {};
}

inline std::vector<uint8_t> calcProtKey(const std::vector<uint8_t> &gameDatBytes)
{
	CryptData cd;
	rng::RngData rd;

	cd.gameDatBytes = gameDatBytes;
	datadecrypt::v3_3::initCrypt(cd, { 0, 8, 6 }); // Seed indeces for Game.dat

	rng::runRngChain(rd, cd.seed1, cd.seed2);

	aes::AesKey aesKey;
	aes::AesIV aesIv;

	aesKeyGen(cd, rd, aesKey, aesIv);

	aes::AesRoundKey roundKey;

	aes::keyExpansion(roundKey.data(), aesKey.data());
	std::copy(aesIv.begin(), aesIv.end(), roundKey.begin() + aes::KEY_EXP_SIZE);

	aes::aesCtrXCrypt(cd.gameDatBytes.data() + 20, roundKey.data(), cd.dataSize);

	rd.Reset();

	rng::runRngChain(rd, cd.keyBytes[3], cd.keyBytes[0]);

	cd.seedBytes = cd.keyBytes;

	aesKeyGen(cd, rd, aesKey, aesIv);

	aes::keyExpansion(roundKey.data(), aesKey.data());
	std::copy(aesIv.begin(), aesIv.end(), roundKey.begin() + aes::KEY_EXP_SIZE);

	std::array<uint8_t, ENCRYPTED_KEY_SIZE> encryptedKey;
	const auto &keyBegin = cd.gameDatBytes.begin() + 0xF;
	std::copy(keyBegin, keyBegin + ENCRYPTED_KEY_SIZE, encryptedKey.begin());

	aes::aesCtrXCrypt(encryptedKey.data(), roundKey.data(), ENCRYPTED_KEY_SIZE);

	return findKey(encryptedKey);
}

} // namespace v2

} // namespace wolf::crypt::protkey