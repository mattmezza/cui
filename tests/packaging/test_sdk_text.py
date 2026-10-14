#!/usr/bin/env python3
"""Compare staged Go/Rust SDK contextual font behavior with canonical native text."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import tempfile
import sys
import subprocess
ROOT = Path(__file__).resolve().parents[2]

def run(command, cwd, env):
    subprocess.run([str(item) for item in command], cwd=cwd, env=env, check=True)

def binding_consumers(build, env, font, rtl_font, build_type):
    # Only the invisible-display runner may launch the generated SDL consumers.
    with tempfile.TemporaryDirectory(prefix="sdk-text-consumers-", dir=build) as temporary:
        stage = Path(temporary)
        go, rust = stage / "go", stage / "rust"
        for language, destination in (("go", go), ("rust", rust)):
            destination.mkdir()
            shutil.copytree(ROOT / "bindings" / language, destination / "module",
                            ignore=shutil.ignore_patterns("build", "target", "__pycache__"))
        (go / "go.mod").write_text('module sdk-text-consumer\n\ngo 1.23\n\n'
            'require github.com/mattmezza/cui/bindings/go v0.0.0\n'
            'replace github.com/mattmezza/cui/bindings/go => ./module\n')
        go_source = (ROOT / "tests/packaging/go_smoke.go").read_text()
        go_source = go_source.replace('Native: native, FontPath:',
            'FontFallbacks: []string{' + json.dumps(str(rtl_font), ensure_ascii=False) + '}, Native: native, FontPath:')
        (go / "main.go").write_text(go_source.replace("Save ✓", "Save אב"))
        (rust / "src").mkdir()
        (rust / "Cargo.toml").write_text('[package]\nname="sdk-text-consumer"\n'
            'version="0.0.0"\nedition="2021"\n[dependencies]\n'
            'cui-builders={path="module",features=["native"]}\n')
        rust_source = (ROOT / "tests/packaging/rust_smoke.rs").read_text()
        rust_source = rust_source.replace('    let root =',
            '    app.font_fallbacks(&[' + json.dumps(str(rtl_font), ensure_ascii=False) + '])?;\n    let root =')
        (rust / "src/main.rs").write_text(rust_source.replace("Save ✓", "Save אב"))
        # CTest inherits the sanitized environment; the wrapper replaces display
        # variables with its private Xvfb display and runtime directory.
        (stage / "CTestTestfile.cmake").write_text(
            'add_test("go_native_sdk_text_callback" "go" "run" "-tags=cui_native" ".")\n'
            'set_tests_properties(go_native_sdk_text_callback PROPERTIES WORKING_DIRECTORY "' + str(go) + '")\n'
            'add_test("rust_native_sdk_text_callback" "cargo" "run" "--offline")\n'
            'set_tests_properties(rust_native_sdk_text_callback PROPERTIES WORKING_DIRECTORY "' + str(rust) + '")\n')
        consumer_env = {**env, "CUI_PACKAGE_SMOKE_BACKEND": "sdl",
            "CUI_PACKAGE_SMOKE_FONT": str(font), "CUI_PACKAGE_BUILD_TYPE": build_type,
            "GOCACHE": str(stage / "go-cache"), "CARGO_TARGET_DIR": str(stage / "rust-target")}
        run([sys.executable, ROOT / "scripts/test_local.py", "--build-dir", stage], ROOT, consumer_env)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--font", type=Path, required=True)
    parser.add_argument("--rtl-font", type=Path, required=True)
    parser.add_argument("--build-type", choices=("Debug", "Release"), default="Debug")
    parser.add_argument("--bindings", action="store_true", help="Also run native Go/Rust RTL callback consumers under invisible Xvfb")
    args = parser.parse_args()
    font, rtl_font = args.font.resolve(strict=True), args.rtl_font.resolve(strict=True)
    build = args.build_dir.resolve(); build.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ)
    for key in ("DISPLAY", "WAYLAND_DISPLAY", "MIR_SOCKET", "SDL_VIDEODRIVER", "XDG_RUNTIME_DIR",
                "CUI_BRIDGE_LIBRARY", "CUI_BRIDGE_LIB_DIR", "LD_LIBRARY_PATH"):
        env.pop(key, None)
    flags = shlex.split(subprocess.check_output(["pkg-config", "--libs", "freetype2", "harfbuzz"], text=True))
    run([sys.executable, ROOT / "scripts/generate_go_sdk.py", "--check"], ROOT, env)
    for name, path in (("primary-font", font), ("rtl-font", rtl_font)):
        print(f"{name} SHA256: {hashlib.sha256(path.read_bytes()).hexdigest()}", flush=True)
    outputs = []
    for name, source, cmake_source, target, subdir in (
        ("canonical", ROOT, ROOT, "cui_text", ""),
        ("go-sdk", ROOT / "bindings/go/sdk/source", ROOT / "bindings/go/sdk/source", "cui_text", ""),
        ("rust-sdk", ROOT / "bindings/rust/sdk/source", ROOT / "bindings/rust/packaging", "cui_rust_bridge", "cui_source"),
    ):
        dependencies = json.loads((source / "third_party/dependencies.json").read_text())
        vendor = next(entry for entry in dependencies if entry["name"] == "SheenBidi")
        text_inputs = sorted({"src/text/cui_text.c", "src/text/cui_bidi.c", *vendor["sha256"]})
        fingerprint = hashlib.sha256()
        for item in text_inputs:
            fingerprint.update(item.encode()); fingerprint.update((source / item).read_bytes())
        print(f"{name} text-source SHA256: {fingerprint.hexdigest()}", flush=True)
        destination = build / name
        configuration = ["cmake", "-S", cmake_source, "-B", destination, "-G", "Ninja",
             "-DCUI_BUILD_NATIVE=ON", f"-DCMAKE_BUILD_TYPE={args.build_type}"]
        if name == "canonical":
            configuration.append("-DCUI_BUILD_BIDI=ON")
        elif name == "rust-sdk":
            configuration.append(f"-DCUI_PACKAGE_ROOT={source}")
        run(configuration, ROOT, env)
        run(["cmake", "--build", destination, "--target", target, "-j2"], ROOT, env)
        archives = destination / subdir
        executable = destination / "sdk-text-fixture"
        run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-UNDEBUG",
             "-I" + str(source / "include"), ROOT / "tests/packaging/sdk_text_fixture.c",
             archives / "libcui_text.a", archives / "libcui.a", archives / "libcui_bidi.a",
             *flags, "-lm", "-o", executable], ROOT, env)
        output = subprocess.check_output([str(executable), str(font), str(rtl_font)], cwd=build, env=env, text=True).strip()
        outputs.append(output); print(name + ": " + output, flush=True)
    if len(set(outputs)) != 1:
        raise RuntimeError("Staged SDK contextual geometry/raster differs from canonical native text")
    print("Canonical, Go SDK and Rust SDK contextual font parity fixture PASS", flush=True)
    if args.bindings:
        binding_consumers(build, env, font, rtl_font, args.build_type)

if __name__ == "__main__":
    main()
