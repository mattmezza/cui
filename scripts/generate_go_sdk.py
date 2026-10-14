#!/usr/bin/env python3
"""Generate/check the minimal Go module SDK against canonical tracked CUI sources."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
ROOT = Path(__file__).resolve().parents[1]
SDKS = [ROOT / "bindings/go/sdk/source", ROOT / "bindings/rust/sdk/source"]
DEPENDENCIES = ("Clay", "NanoSVG", "GPUI Kit icon assets / Lucide", "GPUI Kit named palette data",
                "Unicode grapheme and word properties and conformance fixtures", "SheenBidi")

def snapshot():
    tracked = subprocess.check_output(["git", "ls-files", "--cached", "-z"], cwd=ROOT).decode().split("\0")
    files = {}
    for name in tracked:
        if name and (name.startswith(("src/", "include/", "backends/sdl3/", "backends/software/")) or name == "LICENSE"):
            files[name] = (ROOT / name).read_bytes()
    dependencies = [entry for entry in json.loads((ROOT / "third_party/dependencies.json").read_text())
                    if entry["name"] in DEPENDENCIES]
    for entry in dependencies:
        for name, expected in entry["sha256"].items():
            data = (ROOT / name).read_bytes()
            if hashlib.sha256(data).hexdigest() != expected:
                raise RuntimeError("Canonical pinned dependency checksum mismatch: " + name)
            files[name] = data
        name = entry["license_file"]
        files[name] = (ROOT / name).read_bytes()
    # Some generated icon/theme assets are not separately listed by the dependency manifest.
    for name in tracked:
        if name.startswith(("third_party/clay/", "third_party/nanosvg/", "third_party/gpui_icons/", "third_party/gpui_themes/", "third_party/sheenbidi/")):
            files[name] = (ROOT / name).read_bytes()
    files["third_party/dependencies.json"] = (json.dumps(dependencies, indent=2) + "\n").encode()
    cmake = (ROOT / "CMakeLists.txt").read_text()
    sources = re.search(r"add_library\(cui STATIC ([^)]+)\)", cmake).group(1).split()
    sources += re.search(r"target_sources\(cui PRIVATE ([^)]+)\)", cmake).group(1).split()
    files["CUI_SOURCES.cmake"] = ("# Generated from canonical CMake cui target; do not edit.\nset(CUI_SOURCES\n" +
        "\n".join(" " + source for source in sources) + "\n)\n").encode()
    files["CMakeLists.txt"] = CMAKE.encode()
    rows = [hashlib.sha256(data).hexdigest() + "  " + name for name, data in sorted(files.items())]
    files["SOURCE_FILES.sha256"] = ("\n".join(rows) + "\n").encode()
    return files

CMAKE = '''# Generated minimal SDK; changes belong in scripts/generate_go_sdk.py.
cmake_minimum_required(VERSION 3.20)
project(cui_binding_sdk LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
option(CUI_BUILD_NATIVE "Build optional SDL bridge backend" OFF)
include(CUI_SOURCES.cmake)
add_library(cui STATIC ${CUI_SOURCES})
target_include_directories(cui PUBLIC include PRIVATE third_party/clay)
target_link_libraries(cui PUBLIC m)
add_library(cui_bridge SHARED src/bridge/cui_bridge.c)
target_include_directories(cui_bridge PUBLIC include PRIVATE src/bridge)
target_compile_definitions(cui_bridge PRIVATE CUI_BRIDGE_BUILD=1)
target_link_libraries(cui_bridge PRIVATE cui)
if(CUI_BUILD_NATIVE)
 find_package(PkgConfig REQUIRED)
 pkg_check_modules(TEXT REQUIRED IMPORTED_TARGET freetype2 harfbuzz)
 pkg_check_modules(SDL REQUIRED IMPORTED_TARGET sdl3>=3.2)
 add_library(cui_sheenbidi_objects OBJECT third_party/sheenbidi/Source/SheenBidi.c)
 target_compile_definitions(cui_sheenbidi_objects PRIVATE SB_CONFIG_UNITY)
 target_include_directories(cui_sheenbidi_objects PRIVATE third_party/sheenbidi/Headers third_party/sheenbidi/Source)
 # Match canonical upstream-only diagnostics; CUI text/analyzer retain strict warnings.
 target_compile_options(cui_sheenbidi_objects PRIVATE -Wno-unused-parameter -Wno-unused-value -Wno-unused-function -Wno-implicit-fallthrough)
 add_library(cui_bidi STATIC src/text/cui_bidi.c $<TARGET_OBJECTS:cui_sheenbidi_objects>)
 target_include_directories(cui_bidi PUBLIC include PRIVATE third_party/sheenbidi/Headers)
 add_library(cui_text STATIC src/text/cui_text.c)
 target_compile_definitions(cui_text PRIVATE CUI_TEXT_BIDI)
 target_link_libraries(cui_text PUBLIC cui PRIVATE cui_bidi PkgConfig::TEXT)
 add_library(cui_sdl3 STATIC backends/sdl3/cui_sdl3.c)
 target_link_libraries(cui_sdl3 PUBLIC cui cui_text PRIVATE PkgConfig::SDL)
 target_sources(cui_bridge PRIVATE src/bridge/cui_bridge_sdl.c)
 target_compile_definitions(cui_bridge PRIVATE CUI_BRIDGE_WITH_SDL=1)
 target_link_libraries(cui_bridge PRIVATE cui_sdl3)
endif()
foreach(target cui cui_bridge cui_text cui_sdl3 cui_bidi cui_sheenbidi_objects)
 if(TARGET ${target})
  target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
 endif()
endforeach()
'''

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--go-only", action="store_true")
    args = parser.parse_args()
    files = snapshot()
    wrappers = {}
    sources = [name.strip() for name in files["CUI_SOURCES.cmake"].decode().splitlines()[2:-1]] + ["src/bridge/cui_bridge.c"]
    for source in sources:
        name = "z_cui_" + source.replace("/", "_").replace(".c", "") + ".c"
        wrappers[name] = ('//go:build linux && !cui_dynamic\n\n/* Generated separate translation unit; do not edit. */\n#include "../sdk/source/' + source + '"\n').encode()
    for source in ("src/text/cui_text.c", "src/text/cui_bidi.c",
                   "third_party/sheenbidi/Source/SheenBidi.c",
                   "backends/sdl3/cui_sdl3.c", "src/bridge/cui_bridge_sdl.c"):
        name = "z_cui_native_" + source.replace("/", "_").replace(".c", "") + ".c"
        prefix = '//go:build linux && cui_native && !cui_dynamic\n\n/* Generated separate translation unit; do not edit. */\n'
        if source == "third_party/sheenbidi/Source/SheenBidi.c":
            prefix += ('#define SB_CONFIG_UNITY\n'
                       '/* Exact canonical exceptions for unmodified pinned vendor code only. */\n'
                       '#if defined(__GNUC__)\n#pragma GCC diagnostic push\n'
                       '#pragma GCC diagnostic ignored "-Wunused-parameter"\n'
                       '#pragma GCC diagnostic ignored "-Wunused-value"\n'
                       '#pragma GCC diagnostic ignored "-Wunused-function"\n'
                       '#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"\n#endif\n')
        suffix = '\n#if defined(__GNUC__)\n#pragma GCC diagnostic pop\n#endif\n' if source == "third_party/sheenbidi/Source/SheenBidi.c" else ''
        wrappers[name] = (prefix + '#include "../sdk/source/' + source + '"\n' + suffix).encode()
    integration = ROOT / "bindings/go/cui"
    extra = [path.name for path in integration.glob("z_cui_*.c") if path.name not in wrappers]
    stale = [name for name, data in wrappers.items() if not (integration / name).is_file() or (integration / name).read_bytes() != data]
    if extra or (args.check and stale):
        raise SystemExit("Go translation units are stale: " + ", ".join(extra + stale))
    if not args.check:
        for name, data in wrappers.items(): (integration / name).write_bytes(data)
    for SDK in SDKS[:1] if args.go_only else SDKS:
        if args.check:
            try:
                receipt = json.loads((SDK.parent / "SOURCE_PROVENANCE.json").read_text())
                assert receipt["snapshot_manifest_sha256"] == hashlib.sha256(files["SOURCE_FILES.sha256"]).hexdigest()
            except (OSError, ValueError, KeyError, AssertionError) as error:
                raise SystemExit("Binding SDK provenance is missing or stale: " + str(SDK)) from error
            mismatches = [name for name, data in files.items() if not (SDK / name).is_file() or (SDK / name).read_bytes() != data]
            extra = [str(file.relative_to(SDK)) for file in SDK.rglob("*") if file.is_file() and str(file.relative_to(SDK)) not in files]
            if mismatches or extra:
                raise SystemExit("Binding SDK snapshot is stale; regenerate: " + ", ".join((mismatches + extra)[:12]))
            continue
        SDK.mkdir(parents=True, exist_ok=True)
        existing = [file for file in SDK.rglob("*") if file.is_file() and str(file.relative_to(SDK)) not in files]
        if existing:
            raise SystemExit("Obsolete SDK files require explicit review before removal: " + ", ".join(str(file) for file in existing[:12]))
        for name, data in files.items():
            target = SDK / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
        canonical = ["src", "include", "backends/sdl3", "backends/software", "third_party", "CMakeLists.txt", "LICENSE"]
        diff = subprocess.check_output(["git", "diff", "HEAD", "--", *canonical], cwd=ROOT)
        receipt = {"source_revision": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
                   "canonical_tracked_sources_dirty": bool(diff), "tracked_diff_sha256": hashlib.sha256(diff).hexdigest(),
                   "snapshot_manifest_sha256": hashlib.sha256(files["SOURCE_FILES.sha256"]).hexdigest()}
        (SDK.parent / "SOURCE_PROVENANCE.json").write_text(json.dumps(receipt, indent=2) + "\n")
        (SDK.parent / ".gitattributes").write_text("source/** -text\n")
    if args.check: print(("Go" if args.go_only else "Go/Rust") + " SDK source freshness and pinned checksums verified")
    else: print("Generated", len(files), "minimal SDK files per binding")

if __name__ == "__main__":
    main()
