#!/usr/bin/env python3
"""Install actual wheel/npm artifacts and smoke-test from clean temporary directories."""
import argparse
import hashlib
import re
import tarfile
import zipfile
import os
import secrets
from pathlib import Path, PurePosixPath
import shutil
import subprocess
import sys
import tempfile
ROOT = Path(__file__).resolve().parents[1]

def archive_path(name):
    """Require a canonical relative POSIX file name, without traversal."""
    if (not name or "\\" in name or "\0" in name or name.startswith("/") or
            re.match(r"^[A-Za-z]:", name) or
            any(part in ("", ".", "..") for part in name.split("/")) or
            str(PurePosixPath(name)) != name):
        raise ValueError("Unsafe archive/source manifest path: " + repr(name))
    return name


def verify_source_manifest(artifact):
    """Hash the actual archived source files before trusting package installation."""
    artifact = Path(artifact)
    if artifact.suffix == ".whl":
        archive = zipfile.ZipFile(artifact)
        prefix = "cui_bridge/_source/"
        members = {}
        for member in archive.infolist():
            name = archive_path(member.filename.removesuffix("/") if member.is_dir() else member.filename)
            if name in members:
                archive.close()
                raise ValueError("Duplicate archive entry: " + name)
            # Unix symlinks and special files must never be treated as source bytes.
            kind = (member.external_attr >> 16) & 0o170000
            if kind not in (0, 0o100000, 0o040000):
                archive.close()
                raise ValueError("Nonregular archive entry: " + name)
            members[name] = member
        read = archive.read
        regular = lambda member: not member.is_dir()
    else:
        archive = tarfile.open(artifact, "r:gz")
        prefix = "package/_source/"
        members = {}
        for member in archive.getmembers():
            name = archive_path(member.name.removesuffix("/") if member.isdir() else member.name)
            if name in members:
                archive.close()
                raise ValueError("Duplicate archive entry: " + name)
            if not (member.isfile() or member.isdir()):
                archive.close()
                raise ValueError("Nonregular archive entry: " + name)
            members[name] = member
        read = lambda member: archive.extractfile(member).read()
        regular = lambda member: member.isfile()
    try:
        manifest = members.get(prefix + "SOURCE_FILES.sha256")
        if manifest is None or not regular(manifest):
            raise ValueError("Missing source manifest in " + artifact.name)
        seen = set()
        for line in read(manifest).decode("utf-8").splitlines():
            match = re.fullmatch(r"([0-9a-f]{64})  (.+)", line)
            if match is None:
                raise ValueError("Malformed source manifest entry: " + repr(line))
            digest, relative = match.groups()
            archive_path(relative)
            if relative in seen:
                raise ValueError("Duplicate source manifest entry: " + relative)
            seen.add(relative)
            member = members.get(prefix + relative)
            if member is None or not regular(member):
                raise ValueError("Missing archived source file: " + relative)
            if hashlib.sha256(read(member)).hexdigest() != digest:
                raise ValueError("Archived source hash mismatch: " + relative)
        # Existing producer excludes nested SDK hash receipts from recursive hashing.
        receipt_exclusions = {"SOURCE_FILES.sha256",
                              "bindings/go/sdk/source/SOURCE_FILES.sha256",
                              "bindings/rust/sdk/source/SOURCE_FILES.sha256"}
        actual = {name[len(prefix):] for name, member in members.items()
                  if name.startswith(prefix) and regular(member)
                  and name[len(prefix):] not in receipt_exclusions}
        if actual != seen:
            raise ValueError("Unlisted archived source files: " + ", ".join(sorted(actual - seen)))
        if not seen:
            raise ValueError("Empty source manifest in " + artifact.name)
        print(f"{artifact.name}: verified {len(seen)} archived source files", flush=True)
        return len(seen)
    finally:
        archive.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--artifacts", type=Path, required=True)
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--font", type=Path)
    parser.add_argument("--require-prebuilt", action="store_true", help="Fail if installation tries a native compiler/build tool")
    parser.add_argument("--verify-only", action="store_true", help="Verify actual wheel/npm source archives without installing or running consumers")
    parser.add_argument("--virtual-token", help=argparse.SUPPRESS)
    args = parser.parse_args()
    artifacts = args.artifacts.resolve()
    if args.verify_only:
        verify_source_manifest(next(artifacts.glob("*.whl")))
        verify_source_manifest(next(artifacts.glob("*.tgz")))
        return
    env = dict(os.environ)
    for key in ("PYTHONPATH", "CUI_BRIDGE_LIBRARY", "CUI_NODE_ADDON", "CUI_BRIDGE_LIB_DIR", "LD_LIBRARY_PATH"):
        env.pop(key, None)
    env["CUI_PACKAGE_SMOKE_BACKEND"] = "none"
    child = bool(args.virtual_token and secrets.compare_digest(args.virtual_token, env.get("CUI_PACKAGE_VIRTUAL_TOKEN", "")))
    if args.virtual_token and not child:
        raise SystemExit("Invalid invisible-display child token")
    if args.native and not child:
        for key in ("DISPLAY", "WAYLAND_DISPLAY", "MIR_SOCKET", "SDL_VIDEODRIVER", "XDG_RUNTIME_DIR"):
            env.pop(key, None)
        token = secrets.token_hex(32)
        env["CUI_PACKAGE_VIRTUAL_TOKEN"] = token
        font = args.font or Path(subprocess.check_output(["fc-match", "-f", "%{file}", "sans:charset=2713"], text=True))
        if not font.is_file():
            raise SystemExit("Native package validation requires an existing font file")
        env["CUI_PACKAGE_SMOKE_FONT"] = str(font.resolve())
        with tempfile.TemporaryDirectory(prefix="cui-package-runtime-") as runtime:
            env["XDG_RUNTIME_DIR"] = runtime
            command = ["xvfb-run", "-a", "-s", "-screen 0 1920x1080x24", sys.executable,
                       str(Path(__file__).resolve()), "--artifacts", str(artifacts), "--native"]
            if args.require_prebuilt: command.append("--require-prebuilt")
            command += ["--virtual-token", token]
            if getattr(args, "allow_runtime_build", False): command.append("--allow-runtime-build")
            raise SystemExit(subprocess.run(command, env=env).returncode)
    if args.native:
        if not child or not env.get("DISPLAY") or env["DISPLAY"].split(".")[0] == ":0" or env.get("WAYLAND_DISPLAY"):
            raise SystemExit("Native validation requires a dedicated invisible Xvfb display")
        env["CUI_PACKAGE_SMOKE_BACKEND"] = "sdl"
        env["CUI_PACKAGE_NATIVE"] = "1"
    verify_source_manifest(next(artifacts.glob("*.whl")))
    verify_source_manifest(next(artifacts.glob("*.tgz")))
    with tempfile.TemporaryDirectory(prefix="cui-package-install-") as temporary:
        clean = Path(temporary)
        if args.require_prebuilt:
            blocked = clean / "blocked-toolchain"
            blocked.mkdir()
            for tool in ("cmake", "cc", "gcc", "clang", "make", "ninja", "pkg-config"):
                stub = blocked / tool
                stub.write_text("#!/bin/sh\necho 'package install attempted blocked toolchain' >&2\nexit 98\n")
                stub.chmod(0o755)
            env["PATH"] = str(blocked) + os.pathsep + env["PATH"]
        venv = clean / "venv"
        subprocess.run([sys.executable, "-m", "venv", str(venv)], check=True, env=env)
        python = venv / "bin/python"
        wheel = next(artifacts.glob("*.whl"))
        subprocess.run([str(python), "-m", "pip", "install", "--no-index", str(wheel)], cwd=clean, env=env, check=True)
        shutil.copy2(ROOT / "tests/packaging/python_smoke.py", clean / "python_smoke.py")
        subprocess.run([str(python), "python_smoke.py"], cwd=clean, env=env, check=True)
        (clean / "package.json").write_text('{"name":"cui-local-package-smoke","version":"0.0.0","private":true}')
        tarball = next(artifacts.glob("*.tgz"))
        subprocess.run(["npm", "install", "--no-audit", "--no-fund", str(tarball)], cwd=clean, env=env, check=True)
        shutil.copy2(ROOT / "tests/packaging/node_smoke.cjs", clean / "node_smoke.cjs")
        subprocess.run(["node", "node_smoke.cjs"], cwd=clean, env=env, check=True)
        subprocess.run(["npm", "install", "--save-dev", "--no-audit", "--no-fund", "--ignore-scripts", "typescript@5.8.3"], cwd=clean, env=env, check=True)
        shutil.copy2(ROOT / "tests/packaging/typescript_smoke.cts", clean / "typescript_smoke.cts")
        subprocess.run(["node", "node_modules/typescript/bin/tsc", "--strict", "--noEmit", "--module", "NodeNext",
                        "--moduleResolution", "NodeNext", "--target", "ES2022", "typescript_smoke.cts"], cwd=clean, env=env, check=True)
if __name__ == "__main__":
    main()
