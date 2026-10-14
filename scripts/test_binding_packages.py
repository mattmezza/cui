#!/usr/bin/env python3
"""Install actual wheel/npm artifacts and smoke-test from clean temporary directories."""
import argparse
import os
import secrets
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--artifacts", type=Path, required=True)
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--font", type=Path)
    parser.add_argument("--require-prebuilt", action="store_true", help="Fail if installation tries a native compiler/build tool")
    parser.add_argument("--virtual-token", help=argparse.SUPPRESS)
    args = parser.parse_args()
    artifacts = args.artifacts.resolve()
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
