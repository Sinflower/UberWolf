"""Run from an x64 VS developer prompt: python tests/test_pro1_key_length.py.

Requires clang-cl on PATH; --coverage DIR also requires llvm-profdata/llvm-cov.
The exact production function and constants are extracted to avoid unrelated backends.
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
    source = (root / "UberWolfLib/WolfPro.cpp").read_text(encoding="utf-8-sig")
    constant_start = source.index("namespace DxArcKey\n")
    constant_end = source.index("\n}", constant_start) + 2
    start = source.index("Key WolfPro::findDxArcKeyV1(")
    end = source.index("\n}", start) + 2
    body = "\n" * source.count("\n", 0, constant_start) + source[constant_start:constant_end]
    body += "\n" * source.count("\n", constant_end, start) + source[start:end]

    with tempfile.TemporaryDirectory(prefix="uberwolf-pro1-") as temporary:
        build = Path(temporary)
        include = build / "find_dxarc_key_v1.inc"
        include.write_text(body, encoding="utf-8")
        executable = build / "pro1_key_length.exe"
        command = [
            "clang-cl", "/nologo", "/std:c++20", "/EHsc", "/MT", "/DUNICODE",
            "/D_UNICODE", f"/I{root / 'UberWolfLib'}", f"/I{build}",
            str(root / "tests/pro1_key_length.cpp"), f"/Fe{executable}",
            f"/Fo{build / 'pro1_key_length.obj'}",
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
