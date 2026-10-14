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

def copy_source(destination):
    destination.mkdir(parents=True, exist_ok=True)
    (destination / "SOURCE_REVISION").write_text(subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True))
    tracked = subprocess.check_output(["git", "ls-files", "--cached", "--others", "--exclude-standard", "-z"], cwd=ROOT).decode().split("\0")
    for name in tracked:
        if not name or not (name in ("CMakeLists.txt", "LICENSE") or name.split("/")[0] in SOURCE_DIRS):
            continue
        source = ROOT / name
        if not source.is_file():
            continue
        target = destination / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
    manifest = []
    for file in sorted(destination.rglob("*")):
        if file.is_file():
            manifest.append(hashlib.sha256(file.read_bytes()).hexdigest() + "  " + str(file.relative_to(destination)))
    (destination / "SOURCE_FILES.sha256").write_text("\n".join(manifest) + "\n")

def stage(language, destination):
    package = destination / language
    shutil.copytree(ROOT / "bindings" / language, package,
                    ignore=shutil.ignore_patterns("__pycache__", "target", "build", "node_modules", "*.tgz"))
    source = package / ("cui_bridge/_source" if language == "python" else "_source")
    copy_source(source)
    shutil.copy2(ROOT / "LICENSE", package / "LICENSE")
    if language == "node":
        shutil.copytree(ROOT / "bindings/typescript", package / "types")
        (package / "index.d.cts").write_text("export * from './types/index';\n")
    return package

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("language", choices=("python", "node"))
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
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
            subprocess.run(["npm", "pack", "--pack-destination", str(output)], cwd=package, check=True)

if __name__ == "__main__":
    main()
