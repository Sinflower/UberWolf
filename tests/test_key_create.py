"""Run `python tests/test_key_create.py` in an x64 Visual Studio developer prompt
with clang-cl on PATH. Add --coverage --build-dir <directory> for LLVM profiles.
The test compiles the production KeyCreate and HashCRC32 bodies.
"""

import argparse
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
import zlib


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "3rdParty/DXLib/DXArchive.cpp"
PARSER = argparse.ArgumentParser()
PARSER.add_argument("--build-dir", type=Path)
PARSER.add_argument("--coverage", action="store_true")
OPTIONS, UNITTEST_ARGS = PARSER.parse_known_args()


def function(signature, terminator):
    text = SOURCE.read_text(encoding="utf-8")
    start = text.index(signature)
    stop = text.index(terminator, start)
    end = text.rindex("\n}", start, stop) + 2
    return f'#line {text[:start].count(chr(10)) + 1} "{SOURCE.as_posix()}"\n' + text[start:end] + "\n"


PREAMBLE = r'''
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <vector>
using u8 = unsigned char;
using u32 = std::uint32_t;
using DWORD = std::uint32_t;
using BYTE = unsigned char;
constexpr int CHARCODEFORMAT_ASCII = 32765;
const char DefaultKeyString[] = "DXBDXARC";
void CL_strcpy(int, char *dest, const char *src) { std::strcpy(dest, src); }
int CL_strlen(int, const char *s) { return static_cast<int>(std::strlen(s)); }
struct DXArchive {
    static void KeyCreate(const char *, std::size_t, u8 *);
    static u32 HashCRC32(const void *, std::size_t);
};
bool failAllocation = false;
void *operator new(std::size_t size) {
    if (failAllocation) throw std::bad_alloc();
    if (void *p = std::malloc(size ? size : 1)) return p;
    throw std::bad_alloc();
}
void operator delete(void *p) noexcept { std::free(p); }
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
void *operator new[](std::size_t size) { return operator new(size); }
void operator delete[](void *p) noexcept { std::free(p); }
void operator delete[](void *p, std::size_t) noexcept { std::free(p); }
'''

DRIVER = r'''
int main(int argc, char **argv) {
    if (argc != 3) return 2;
    const long requested = std::strtol(argv[2], nullptr, 10);
    const std::size_t length = requested < 0 ? 4 : static_cast<std::size_t>(requested);
    std::vector<char> source(length + 1, '\0');
    for (std::size_t i = 0; i < length; ++i)
        source[i] = static_cast<char>((i * 7 + 3) % 255 + 1);
    u8 key[7] = {0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5};
    failAllocation = std::strcmp(argv[1], "fail") == 0;
    int result = 0;
    try {
        DXArchive::KeyCreate(source.data(), requested < 0 ? 0 : length, key);
    } catch (const std::bad_alloc &) {
        result = 42;
    }
    for (auto byte : key) std::printf("%02X", byte);
    std::printf("\n");
    return result;
}
'''


class KeyCreateTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="key-create-")
        cls.build = (OPTIONS.build_dir or Path(cls.temporary.name)).resolve()
        cls.build.mkdir(parents=True, exist_ok=True)
        subject = PREAMBLE + function(
            "u32 DXArchive::HashCRC32(", "int DXArchive::EncodeArchiveOneDirectoryWolf(")
        # Fault injection changes only the scratch allocator, after standard headers.
        subject += "#define malloc(n) (failAllocation ? nullptr : std::malloc(n))\n"
        subject += function("void DXArchive::KeyCreate(", "void DXArchive::KeyConv(")
        subject += '#undef malloc\n#line 1 "test_driver.cpp"\n' + DRIVER
        (cls.build / "subject.cpp").write_text(subject, encoding="utf-8")
        cls.executable = cls.build / ("subject.exe" if os.name == "nt" else "subject")
        if os.name == "nt":
            command = ["clang-cl", "/nologo", "/std:c++17", "/EHsc", "/MT", "/W4", "/WX",
                       "/D_CRT_SECURE_NO_WARNINGS", "subject.cpp", f"/Fe:{cls.executable}"]
            if OPTIONS.coverage:
                command += ["/clang:-fprofile-instr-generate", "/clang:-fcoverage-mapping"]
        else:
            command = ["g++", "-std=c++17", "-Wall", "-Wextra", "-Werror", "subject.cpp",
                       "-o", str(cls.executable)]
        subprocess.run(command, cwd=cls.build, check=True)

    @classmethod
    def tearDownClass(cls):
        cls.temporary.cleanup()

    def run_subject(self, mode, length):
        environment = dict(os.environ, LLVM_PROFILE_FILE=str(self.build / "key-%p.profraw"))
        return subprocess.run([str(self.executable), mode, str(length)], capture_output=True,
                              text=True, env=environment)

    def test_keys_match_independent_crc32(self):
        for length in (0, 1, 3, 4, -1, 1024, 1025, 2039):
            with self.subTest(length=length):
                material = bytes((i * 7 + 3) % 255 + 1 for i in range(4 if length < 0 else length))
                if len(material) < 4:
                    material += b"DXBDXARC"
                expected = (zlib.crc32(material[::2]).to_bytes(4, "little")
                            + zlib.crc32(material[1::2]).to_bytes(4, "little")[:3])
                result = self.run_subject("key", length)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(result.stdout.strip(), expected.hex().upper())

    def test_heap_failure_throws_before_writing_the_key(self):
        for length in (1025, 2039):
            with self.subTest(length=length):
                result = self.run_subject("fail", length)
                self.assertEqual(result.returncode, 42, result.stdout + result.stderr)
                self.assertEqual(result.stdout.strip(), "A5" * 7)

    def test_stack_scratch_does_not_allocate(self):
        result = self.run_subject("fail", 1024)
        reference = self.run_subject("key", 1024)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, reference.stdout)


if __name__ == "__main__":
    unittest.main(argv=[__file__, *UNITTEST_ARGS])
