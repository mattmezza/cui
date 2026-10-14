#!/usr/bin/env python3
"""Consume Go proxy ZIP and Rust crate artifacts with clean module/build caches."""
import argparse
import os
import secrets
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import tempfile
ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--go-artifacts", type=Path, required=True)
    parser.add_argument("--rust-artifacts", type=Path, required=True)
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--allow-runtime-build", action="store_true", help="Validate the source-only Go alpha fallback")
    parser.add_argument("--font", type=Path)
    parser.add_argument("--virtual-token", help=argparse.SUPPRESS)
    args = parser.parse_args()
    env = dict(os.environ)
    for key in ("CUI_BRIDGE_LIBRARY", "CUI_BRIDGE_LIB_DIR", "LD_LIBRARY_PATH", "GOPRIVATE", "GONOPROXY", "GONOSUMDB"):
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
        if not font.is_file(): raise SystemExit("Native validation requires an existing font")
        env["CUI_PACKAGE_SMOKE_FONT"] = str(font.resolve())
        with tempfile.TemporaryDirectory(prefix="cui-compiled-runtime-") as runtime:
            env["XDG_RUNTIME_DIR"] = runtime
            command = ["xvfb-run", "-a", "-s", "-screen 0 1920x1080x24", sys.executable,
                str(Path(__file__).resolve()), "--go-artifacts", str(args.go_artifacts.resolve()),
                "--rust-artifacts", str(args.rust_artifacts.resolve()), "--native"]
            command += ["--virtual-token", token]
            if getattr(args, "allow_runtime_build", False): command.append("--allow-runtime-build")
            raise SystemExit(subprocess.run(command, env=env).returncode)
    if args.native:
        if not child or not env.get("DISPLAY") or env["DISPLAY"].split(".")[0] == ":0" or env.get("WAYLAND_DISPLAY"):
            raise SystemExit("Native validation requires a dedicated invisible Xvfb display")
        env["CUI_PACKAGE_SMOKE_BACKEND"] = "sdl"
    with tempfile.TemporaryDirectory(prefix="cui-compiled-install-") as temporary:
        clean = Path(temporary)
        consumer = clean / "consumer"
        consumer.mkdir()
        env.update(GOMODCACHE=str(clean / "go-modules"), GOCACHE=str(clean / "go-cache"),
                   GOPROXY=args.go_artifacts.resolve().as_uri(), GOSUMDB="off", CGO_ENABLED="1",
                   XDG_CACHE_HOME=str(clean / "runtime-cache"))
        (consumer / "go.mod").write_text("module cui-package-smoke\n\ngo 1.23\n\nrequire github.com/mattmezza/cui/bindings/go v0.1.0-alpha.1\n")
        shutil.copy2(ROOT / "tests/packaging/go_smoke.go", consumer / "main.go")
        subprocess.run(["go", "mod", "download", "all"], cwd=consumer, env=env, check=True)
        binary = clean / "go-smoke"
        subprocess.run(["go", "build", "-o", str(binary), "."], cwd=consumer, env=env, check=True)
        # Both artifact modes embed their inputs; neither may read the module checkout.
        (clean / "go-modules").rename(clean / "unused-go-modules")
        blocked = clean / "blocked-toolchain"
        blocked.mkdir()
        for tool in ("cmake", "cc", "gcc", "clang", "ninja", "make", "pkg-config"):
            executable = blocked / tool
            executable.write_text("#!/bin/sh\necho compiler forbidden at runtime >&2\nexit 99\n")
            executable.chmod(0o755)
        runtime_env = dict(env, PATH=str(blocked) + os.pathsep + env["PATH"])
        subprocess.run([str(binary)], cwd=clean, env=env if args.allow_runtime_build else runtime_env, check=True)
        # Repeated consumers share a cold cache; process locking must serialize writes.
        shutil.rmtree(clean / "runtime-cache")
        go_runtime = env if args.allow_runtime_build else runtime_env
        concurrent = [subprocess.Popen([str(binary)], cwd=clean, env=go_runtime) for _ in range(2)]
        if any(process.wait() != 0 for process in concurrent):
            raise SystemExit("Concurrent Go package cache initialization failed")
        libraries = list((clean / "runtime-cache").rglob("libcui_bridge.so"))
        if len(libraries) != 1:
            raise SystemExit("Expected one verified Go cached bridge")
        libraries[0].write_bytes(b"invalid cached library")
        subprocess.run([str(binary)], cwd=clean, env=go_runtime, check=True)
        invalid_env = dict(go_runtime, CUI_BRIDGE_LIBRARY=str(clean / "missing-override.so"))
        failed = subprocess.run([str(binary)], cwd=clean, env=invalid_env, capture_output=True)
        if failed.returncode == 0 or b"missing-override.so" not in failed.stderr:
            raise SystemExit("Go invalid override was hidden by default package discovery")
        crate = next(args.rust_artifacts.resolve().glob("*.crate"))
        with tarfile.open(crate) as archive:
            archive.extractall(clean / "crates", filter="data")
        unpacked = next((clean / "crates").iterdir())
        rust = clean / "rust-consumer"
        (rust / "src").mkdir(parents=True)
        features = ', features = ["native"]' if args.native else ''
        (rust / "Cargo.toml").write_text('[package]\nname="cui-package-smoke"\nversion="0.0.0"\nedition="2021"\n'
            + '[dependencies]\ncui-builders = {path="' + str(unpacked) + '"' + features + '}\n')
        shutil.copy2(ROOT / "tests/packaging/rust_smoke.rs", rust / "src/main.rs")
        env["CARGO_TARGET_DIR"] = str(clean / "cargo-target")
        subprocess.run(["cargo", "build", "--offline"], cwd=rust, env=env, check=True)
        rust_binary = clean / "rust-smoke"
        shutil.copy2(clean / "cargo-target/debug/cui-package-smoke", rust_binary)
        (clean / "cargo-target").rename(clean / "unused-cargo-target")
        (clean / "crates").rename(clean / "unused-crates")
        subprocess.run([str(rust_binary)], cwd=clean, env=runtime_env, check=True)
if __name__ == "__main__":
    main()
