#!/usr/bin/env python3
"""Install actual wheel/npm artifacts and smoke-test from clean temporary directories."""
import argparse
import os
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
    args = parser.parse_args()
    artifacts = args.artifacts.resolve()
    env = dict(os.environ)
    for key in ("PYTHONPATH", "CUI_BRIDGE_LIBRARY", "CUI_NODE_ADDON", "CUI_BRIDGE_LIB_DIR", "LD_LIBRARY_PATH"):
        env.pop(key, None)
    if args.native and env.get("CUI_PACKAGE_VIRTUAL") != "1":
        for key in ("DISPLAY", "WAYLAND_DISPLAY", "MIR_SOCKET", "SDL_VIDEODRIVER", "XDG_RUNTIME_DIR"):
            env.pop(key, None)
        env["CUI_PACKAGE_VIRTUAL"] = "1"
        font = args.font or Path(subprocess.check_output(["fc-match", "-f", "%{file}", "sans:charset=2713"], text=True))
        if not font.is_file():
            raise SystemExit("Native package validation requires an existing font file")
        env["CUI_PACKAGE_SMOKE_FONT"] = str(font.resolve())
        with tempfile.TemporaryDirectory(prefix="cui-package-runtime-") as runtime:
            env["XDG_RUNTIME_DIR"] = runtime
            command = ["xvfb-run", "-a", "-s", "-screen 0 1920x1080x24", sys.executable,
                       str(Path(__file__).resolve()), "--artifacts", str(artifacts), "--native"]
            raise SystemExit(subprocess.run(command, env=env).returncode)
    if args.native:
        if not env.get("DISPLAY") or env.get("DISPLAY") == ":0":
            raise SystemExit("Native validation requires a dedicated invisible Xvfb display")
        env["CUI_PACKAGE_SMOKE_BACKEND"] = "sdl"
        env["CUI_PACKAGE_NATIVE"] = "1"
    with tempfile.TemporaryDirectory(prefix="cui-package-install-") as temporary:
        clean = Path(temporary)
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
if __name__ == "__main__":
    main()
