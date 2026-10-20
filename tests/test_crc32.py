"""Run `python tests/test_crc32.py --tsan` with g++ on Linux.

On Windows, use an x64 Visual Studio developer prompt with clang-cl on PATH.
Add --coverage --build-dir <directory> to retain LLVM coverage profiles.
The race detector check requires Linux.
"""

import argparse
import os
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
PARSER = argparse.ArgumentParser()
PARSER.add_argument("--build-dir", type=Path)
PARSER.add_argument("--coverage", action="store_true")
PARSER.add_argument("--tsan", action="store_true")
OPTIONS, UNITTEST_ARGS = PARSER.parse_known_args()

PREAMBLE = r'''
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <thread>
using u32 = std::uint32_t;
using DWORD = std::uint32_t;
using BYTE = unsigned char;
struct DXArchive { static u32 HashCRC32(const void *, std::size_t); };
'''

DRIVER = r'''
int main() {
    constexpr unsigned threadCount = 32;
    std::atomic<unsigned> ready{0};
    std::atomic<bool> go{false};
    std::atomic<bool> correct{true};
    std::array<std::thread, threadCount> threads;
    for (auto &thread : threads) {
        thread = std::thread([&] {
            ready.fetch_add(1);
            while (!go.load()) std::this_thread::yield();
            if (DXArchive::HashCRC32("123456789", 9) != 0xCBF43926u)
                correct.store(false);
        });
    }
    while (ready.load() != threadCount) std::this_thread::yield();
    go.store(true);
    for (auto &thread : threads) thread.join();
    if (!correct.load()) return 1;
    if (DXArchive::HashCRC32(nullptr, 0) != 0) return 2;
    unsigned char binary[256];
    for (unsigned i = 0; i < 256; ++i) binary[i] = static_cast<unsigned char>(i);
    if (DXArchive::HashCRC32(binary, sizeof(binary)) != 0x29058C73u) return 3;
    if (DXArchive::HashCRC32("123456789", 9) != 0xCBF43926u) return 4;
    return 0;
}
'''


class CRC32Tests(unittest.TestCase):
    def test_concurrent_first_calls_and_known_vectors(self):
        with tempfile.TemporaryDirectory(prefix="crc32-") as temporary:
            build = (OPTIONS.build_dir or Path(temporary)).resolve()
            build.mkdir(parents=True, exist_ok=True)
            source = ROOT / "3rdParty/DXLib/DXArchive.cpp"
            text = source.read_text(encoding="utf-8")
            start = text.index("u32 DXArchive::HashCRC32(")
            stop = text.index("int DXArchive::EncodeArchiveOneDirectoryWolf(", start)
            end = text.rindex("\n}", start, stop) + 2
            subject = (PREAMBLE + f'#line {text[:start].count(chr(10)) + 1} "{source.as_posix()}"\n'
                       + text[start:end] + '\n#line 1 "test_driver.cpp"\n' + DRIVER)
            (build / "subject.cpp").write_text(subject, encoding="utf-8")
            executable = build / ("subject.exe" if os.name == "nt" else "subject")
            if os.name == "nt":
                if OPTIONS.tsan:
                    self.fail("ThreadSanitizer requires a Linux compiler")
                command = ["clang-cl", "/nologo", "/std:c++17", "/EHsc", "/MT", "/W4", "/WX",
                           "subject.cpp", f"/Fe:{executable}"]
                if OPTIONS.coverage:
                    command += ["/clang:-fprofile-instr-generate", "/clang:-fcoverage-mapping"]
            else:
                command = ["g++", "-std=c++17", "-pthread", "-Wall", "-Wextra", "-Werror",
                           "-g", "-O1", "subject.cpp", "-o", str(executable)]
                if OPTIONS.tsan:
                    command += ["-fsanitize=thread", "-fno-omit-frame-pointer", "-no-pie"]
            subprocess.run(command, cwd=build, check=True)
            environment = dict(os.environ, TSAN_OPTIONS="halt_on_error=1:exitcode=66",
                               LLVM_PROFILE_FILE=str(build / "crc-%p.profraw"))
            result = subprocess.run([str(executable)], env=environment, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main(argv=[__file__, *UNITTEST_ARGS])
