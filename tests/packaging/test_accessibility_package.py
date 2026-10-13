#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Install, relocate, link and run optional AccessKit consumers; no Rust invocation."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
def run(command, **kwargs):
    result = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, **kwargs)
    if result.returncode:
        raise AssertionError(f"{command[0]} failed:\n{result.stdout}")
    return result.stdout

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, required=True)
    args = parser.parse_args()
    build = args.build.resolve()
    with tempfile.TemporaryDirectory(prefix="accessibility-package-", dir=build) as directory:
        work = Path(directory)
        stage = work / "stage"
        run(["cmake", "--install", str(build), "--prefix", str(stage)])
        relocated = work / "relocated"
        stage.rename(relocated)
        notices = relocated / "share/cui/licenses/accesskit"
        for source in (ROOT / "third_party/accesskit").glob("LICENSE*"):
            assert (notices / source.name).read_bytes() == source.read_bytes(), source.name
        inventory = (notices / "LICENSES.dependencies.txt").read_text()
        assert len(re.findall(r"^[a-z0-9_-]+ [0-9][^\n]* — ", inventory, re.M)) == 90
        pin = json.loads((notices / "pin.json").read_text())
        receipt = json.loads((notices / "accesskit-build.json").read_text())
        assert receipt["commit"] == pin["commit"]
        assert receipt["library_sha256"] == hashlib.sha256((relocated / "lib/libaccesskit.a").read_bytes()).hexdigest()
        assert (notices / "extension.json").read_bytes() == (ROOT / "backends/accesskit/extension.json").read_bytes()
        symbols = run(["nm", "-g", "--defined-only", str(relocated / "lib/libcui_accessibility.a")])
        assert "cui_accessibility_test_" not in symbols
        consumer = work / "consumer"
        consumer.mkdir()
        (consumer / "CMakeLists.txt").write_text(
            'cmake_minimum_required(VERSION 3.20)\nproject(installed C)\n'
            'find_package(CUIAccessibility CONFIG REQUIRED)\n'
            f'add_executable(consumer "{ROOT}/tests/packaging/accessibility_consumer.c")\n'
            'target_compile_options(consumer PRIVATE -std=c11 -Wall -Wextra -Wpedantic -Werror -UNDEBUG)\n'
            'target_link_libraries(consumer PRIVATE CUI::cui_accessibility)\n')
        run(["cmake", "-S", str(consumer), "-B", str(consumer / "build"), "-G", "Ninja",
             f"-DCMAKE_PREFIX_PATH={relocated}"])
        run(["cmake", "--build", str(consumer / "build"), "--parallel", "2"])
        run([str(consumer / "build/consumer")])
        env = dict(os.environ, PKG_CONFIG_LIBDIR=str(relocated / "lib/pkgconfig"))
        flags = shlex.split(run(["pkg-config", "--static", "--cflags", "--libs", "cui-accessibility"], env=env))
        assert "-laccesskit" in flags and "-lcui" in flags and "-pthread" in flags
        run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-UNDEBUG",
             str(ROOT / "tests/packaging/accessibility_consumer.c"), "-o", str(work / "pc-consumer"), *flags])
        run([str(work / "pc-consumer")])
        # A nonexistent cache must not matter when the option is OFF.
        missing = work / "missing-cache"
        off = work / "off"
        base = ["cmake", "-S", str(ROOT), "-G", "Ninja", "-DCUI_BUILD_NATIVE=OFF",
                "-DCUI_BUILD_BIDI=OFF", "-DCUI_BUILD_CPP_EXAMPLE=OFF", f"-DACCESSKIT_CACHE={missing}"]
        run([*base, "-B", str(off), "-DCUI_BUILD_ACCESSIBILITY=OFF"])
        graph = (off / "build.ninja").read_text()
        assert "libaccesskit.a" not in graph and "cui_accessibility.c" not in graph
        run(["cmake", "--build", str(off), "--target", "cui", "--parallel", "2"])
        bad = subprocess.run([*base, "-B", str(work / "bad"), "-DCUI_BUILD_ACCESSIBILITY=ON"],
                             text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        assert bad.returncode and "Missing optional AccessKit cache file" in bad.stdout
        missing.mkdir()
        (missing / (pin["commit"] + ".tar.gz")).write_text("corrupt")
        bad = subprocess.run([*base, "-B", str(work / "corrupt"), "-DCUI_BUILD_ACCESSIBILITY=ON"],
                             text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        assert bad.returncode and "Pinned AccessKit checksum mismatch" in bad.stdout
    print("PASS: relocated CMake/pkg-config consumers, actual Focus/Activate,90 notices, production ABI, OFF Rust-free and cache failure guards")

if __name__ == "__main__":
    main()
