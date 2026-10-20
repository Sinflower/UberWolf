"""Run from an x64 VS developer prompt: python tests/test_short_header.py.

Requires clang-cl on PATH; --coverage DIR also requires llvm-profdata/llvm-cov.
The exact production function is extracted to avoid linking unrelated backends.
"""

import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--coverage", type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    source_path = root / "UberWolfLib/WolfDec.cpp"
    source = source_path.read_text(encoding="utf-8-sig")
    start = source.index("uint16_t WolfDec::getCryptVersion(")
    end = source.index("\n}", start) + 2
    line = source.count("\n", 0, start) + 1

    with tempfile.TemporaryDirectory(prefix="uberwolf-header-") as temporary:
        build = Path(temporary)
        include = build / "get_crypt_version.inc"
        include.write_text(
            "\n" * (line - 1) + source[start:end],
            encoding="utf-8",
        )
        executable = build / "short_header.exe"
        command = [
            "clang-cl", "/nologo", "/std:c++20", "/EHsc", "/MT", "/DUNICODE",
            "/D_UNICODE", f"/I{root / '3rdParty'}", f"/I{root / 'UberWolfLib'}",
            f"/I{build}", str(root / "tests/short_header.cpp"), f"/Fe{executable}",
            f"/Fo{build / 'short_header.obj'}",
        ]
        if args.coverage:
            command += ["/clang:-fprofile-instr-generate", "/clang:-fcoverage-mapping"]
        subprocess.run(command, cwd=build, check=True)
        environment = os.environ.copy()
        environment["LLVM_PROFILE_FILE"] = str(build / "coverage.profraw")
        subprocess.run([str(executable)], cwd=build, env=environment, check=True)
        if args.coverage:
            destination = args.coverage.resolve()
            destination.mkdir(parents=True, exist_ok=True)
            profile = destination / "coverage.profdata"
            subprocess.run([
                "llvm-profdata", "merge", "-sparse", str(build / "coverage.profraw"),
                "-o", str(profile),
            ], check=True)
            options = [str(executable), f"-instr-profile={profile}", str(include)]
            subprocess.run(["llvm-cov", "report", *options], check=True)
            result = subprocess.run(
                ["llvm-cov", "export", *options], check=True, capture_output=True, text=True,
            )
            (destination / "coverage.json").write_text(result.stdout, encoding="utf-8")
            files = json.loads(result.stdout)["data"][0]["files"]
            totals = next(file["summary"] for file in files if Path(file["filename"]) == include)
            for kind in ("lines", "branches"):
                if totals[kind]["covered"] != totals[kind]["count"]:
                    raise RuntimeError(f"Incomplete {kind} coverage: {totals[kind]}")


if __name__ == "__main__":
    main()
