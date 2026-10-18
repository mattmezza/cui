#!/usr/bin/env python3
"""Stage self-contained alpha source packages; never publishes to a registry."""
import argparse
import importlib.util
import hashlib
import json
import os
import platform
import zipfile
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE_DIRS = ("src", "include", "backends", "third_party", "cmake", "tests", "examples", "bindings", "scripts")

def copy_source(destination, *, omit_npm_ignored=False):
    destination.mkdir(parents=True, exist_ok=True)
    (destination / "SOURCE_REVISION").write_text(subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True))
    diff = subprocess.check_output(["git", "diff", "HEAD", "--", *SOURCE_DIRS, "CMakeLists.txt", "LICENSE"], cwd=ROOT)
    (destination / "SOURCE_DIFF.patch").write_bytes(diff)
    (destination / "SOURCE_PROVENANCE.json").write_text(json.dumps({
        "source_revision": (destination / "SOURCE_REVISION").read_text().strip(),
        "tracked_source_dirty": bool(diff), "tracked_diff_sha256": hashlib.sha256(diff).hexdigest(),
        "scope": list(SOURCE_DIRS), "untracked_files_included": False
    }, indent=2))
    tracked = subprocess.check_output(["git", "ls-files", "--cached", "-z"], cwd=ROOT).decode().split("\0")
    for name in tracked:
        if not name or not (name in ("CMakeLists.txt", "LICENSE") or name.split("/")[0] in SOURCE_DIRS):
            continue
        # npm pack omits .gitignore even inside the bundled source snapshot.
        if omit_npm_ignored and Path(name).name == ".gitignore":
            continue
        source = ROOT / name
        if not source.is_file():
            continue
        target = destination / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
    write_source_manifest(destination)

def write_source_manifest(destination):
    manifest = []
    for file in sorted(destination.rglob("*")):
        if file.is_file() and file.name != "SOURCE_FILES.sha256":
            manifest.append(hashlib.sha256(file.read_bytes()).hexdigest() + "  " + str(file.relative_to(destination)))
    (destination / "SOURCE_FILES.sha256").write_text("\n".join(manifest) + "\n")

def stage(language, destination):
    package = destination / language
    package.mkdir(parents=True)
    tracked = subprocess.check_output(["git", "ls-files", "--cached", "-z"], cwd=ROOT).decode().split("\0")
    prefix = f"bindings/{language}/"
    for name in tracked:
        if name.startswith(prefix) and (ROOT / name).is_file():
            target = package / name[len(prefix):]
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(ROOT / name, target)
    if language in ("go", "rust"):
        subprocess.run(["python", str(ROOT / "scripts/generate_go_sdk.py"), "--check"], check=True)
    else:
        source = package / ("cui_bridge/_source" if language == "python" else "_source")
        copy_source(source, omit_npm_ignored=(language == "node"))
    shutil.copy2(ROOT / "LICENSE", package / "LICENSE")
    if language == "node":
        for name in tracked:
            prefix = "bindings/typescript/"
            if name.startswith(prefix) and (ROOT / name).is_file():
                target = package / "types" / name[len(prefix):]
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(ROOT / name, target)
        (package / "index.d.cts").write_text("export * from './types/index';\n")
    return package

def build_bridge(source, build):
    native = os.environ.get("CUI_PACKAGE_NATIVE") == "1"
    if native:
        subprocess.run(["pkg-config", "--exists", "sdl3 >= 3.2", "freetype2", "harfbuzz"], check=True)
    subprocess.run(["cmake", "-S", str(source / "bindings/bridge"), "-B", str(build),
                    "-DCUI_BUILD_NATIVE=" + ("ON" if native else "OFF"),
                    "-DCMAKE_BUILD_TYPE=" + os.environ.get("CUI_PACKAGE_BUILD_TYPE", "Release")], check=True)
    subprocess.run(["cmake", "--build", str(build), "--target", "cui_bridge", "-j2"], check=True)
    return build / "libcui_bridge.so"

def package_go(package, output, temporary, source_only=True):
    if platform.system() != "Linux":
        raise SystemExit("Alpha Go binary source artifact currently validates Linux only")
    subprocess.run(["python", str(ROOT / "scripts/generate_go_sdk.py"), "--check"], check=True)
    if not source_only:
        source = package / "sdk/source"
        build = temporary / "go-build"
        subprocess.run(["cmake", "-S", str(source), "-B", str(build),
            "-DCUI_BUILD_NATIVE=" + ("ON" if os.environ.get("CUI_PACKAGE_NATIVE") == "1" else "OFF"),
            "-DCMAKE_BUILD_TYPE=" + os.environ.get("CUI_PACKAGE_BUILD_TYPE", "Release")], check=True)
        subprocess.run(["cmake", "--build", str(build), "--target", "cui_bridge", "-j2"], check=True)
        library = build / "libcui_bridge.so"
        native = package / "cui/_native"
        native.mkdir(parents=True, exist_ok=True)
        shutil.copy2(library, native / "libcui_bridge.so")
        (native / "build-info.json").write_text(json.dumps({"alpha": True,
            "native_requested": os.environ.get("CUI_PACKAGE_NATIVE") == "1",
            "platform": "linux", "architecture": {"x86_64": "amd64", "aarch64": "arm64"}.get(platform.machine(), platform.machine()),
            "source_digest": hashlib.sha256((source / "SOURCE_FILES.sha256").read_bytes()).hexdigest(),
            "abi_version": 1, "library_sha256": hashlib.sha256(library.read_bytes()).hexdigest(),
            "build_type": os.environ.get("CUI_PACKAGE_BUILD_TYPE", "Release"),
            "system_development_versions": subprocess.check_output(["pkg-config", "--modversion", "sdl3", "freetype2", "harfbuzz"], text=True).splitlines() if os.environ.get("CUI_PACKAGE_NATIVE") == "1" else [],
            "system_runtime_dependencies": [line.split()[0] for line in subprocess.check_output(["ldd", str(library)], text=True).splitlines() if line.split()]}, indent=2))
    version = "v0.1.0-alpha.2"
    module = "github.com/mattmezza/cui/bindings/go"
    proxy = output / module / "@v"
    proxy.mkdir(parents=True, exist_ok=True)
    shutil.copy2(package / "go.mod", proxy / (version + ".mod"))
    revision_time = subprocess.check_output(
        ["git", "show", "-s", "--format=%cI", "HEAD"], cwd=ROOT, text=True).strip()
    # A module proxy must describe the packaged commit, including its actual
    # timestamp, rather than the date of the original packaging implementation.
    (proxy / (version + ".info")).write_text(json.dumps({"Version": version, "Time": revision_time}))
    with zipfile.ZipFile(proxy / (version + ".zip"), "w", zipfile.ZIP_DEFLATED) as archive:
        for file in sorted(package.rglob("*")):
            if not file.is_file(): continue
            relative = file.relative_to(package)
            # Module ZIPs omit nested modules; these are C-source support copies.
            if relative.name in ("go.mod", "go.sum") and len(relative.parts) > 1: continue
            archive.write(file, module + "@" + version + "/" + str(relative))
    print(proxy / (version + ".zip"))

def package_rust(package, output, temporary):
    environment = dict(os.environ)
    environment["CARGO_TARGET_DIR"] = str(temporary / "cargo-target")
    subprocess.run(["cargo", "package", "--allow-dirty", "--no-verify", "--offline"],
                   cwd=package, env=environment, check=True)
    for artifact in (temporary / "cargo-target/package").glob("*.crate"):
        shutil.copy2(artifact, output / artifact.name)
        print(output / artifact.name)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("language", choices=("python", "node", "go", "rust"))
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--source-only", action="store_true", help="Package canonical Go build-time source SDK (the default)")
    parser.add_argument("--prebuilt", action="store_true", help="Build and bundle Linux addon/bridge before packing Node or Go")
    args = parser.parse_args()
    if args.source_only and args.prebuilt: parser.error("--source-only and --prebuilt are alternatives")
    if args.source_only and args.language != "go": parser.error("--source-only applies to go")
    if args.prebuilt and args.language not in ("node", "go"): parser.error("--prebuilt applies to node or go")
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="cui-package-stage-") as temporary:
        package = stage(args.language, Path(temporary))
        if args.language == "python":
            spec = importlib.util.spec_from_file_location("cui_wheel_backend", package / "packaging/wheel_backend.py")
            backend = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(backend)
            print(backend.build_wheel(str(output)))
        elif args.language == "node":
            if args.prebuilt:
                subprocess.run(["node", str(package / "packaging/install.cjs")], cwd=package, check=True)
            subprocess.run(["npm", "pack", "--pack-destination", str(output)], cwd=package, check=True)
        elif args.language == "go":
            package_go(package, output, Path(temporary), not args.prebuilt)
        elif args.language == "rust":
            package_rust(package, output, Path(temporary))
if __name__ == "__main__":
    main()
