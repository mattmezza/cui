#!/usr/bin/env python3
"""Exercise an installed prebuilt addon under privately fetched, pinned Node runtimes."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
RUNTIMES = ("18.20.8", "22.15.0")
NPM_VERSION = "10.8.2"
NODE_BIN_SETUP_VERSION = "1.1.4"
SUITES = (
    ("test/binding.test.cjs", ("--expose-gc",), ()),
    ("test/addon.test.cjs", ("--expose-gc",), ()),
    ("test/components.test.cjs", (), ()),
    ("test/extended.test.cjs", (), ()),
    ("test/models.test.cjs", (), ()),
    ("examples/workspace.cjs", (), ("--headless",)),
    ("examples/sidebar-workspace.cjs", (), ("--headless",)),
)


def command(args, *, cwd, env):
    subprocess.run([str(a) for a in args], cwd=cwd, env=env, check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tarball", type=Path, required=True)
    parser.add_argument("--cache-dir", type=Path, required=True,
                        help="Private cache for pinned node/npm downloads and temporary installs")
    args = parser.parse_args()
    tarball = args.tarball.resolve(strict=True)
    print(f"Artifact SHA256: {hashlib.sha256(tarball.read_bytes()).hexdigest()}", flush=True)
    cache = args.cache_dir.resolve()
    if cache == ROOT or ROOT in cache.parents:
        parser.error("cache-dir must be outside the checkout")
    cache.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ)
    for key in ("DISPLAY", "WAYLAND_DISPLAY", "MIR_SOCKET", "SDL_VIDEODRIVER", "XDG_RUNTIME_DIR",
                "NODE_PATH", "NODE_OPTIONS", "CUI_NODE_ADDON", "CUI_BRIDGE_LIBRARY",
                "CUI_BRIDGE_LIB_DIR", "LD_LIBRARY_PATH", "CUI_PACKAGE_NATIVE",
                "CUI_TEST_VIRTUAL_DISPLAY", "CUI_PACKAGE_SMOKE_FONT"):
        env.pop(key, None)
    env["CUI_PACKAGE_SMOKE_BACKEND"] = "none"
    env["npm_config_cache"] = str(cache / "npm-cache")
    npm = shutil.which("npm")
    if not npm:
        parser.error("npm is required to privately fetch pinned runtimes")
    for version in RUNTIMES:
        result = subprocess.check_output(
            [npm, "exec", "--yes", f"--package=node@{version}", f"--package=npm@{NPM_VERSION}",
             f"--package=node-bin-setup@{NODE_BIN_SETUP_VERSION}",
             "--", "node", "-p", "JSON.stringify({version:process.version,napi:process.versions.napi,node:process.execPath,path:process.env.PATH})"],
            cwd=cache, env=env, text=True)
        runtime = json.loads(result)
        if runtime["version"] != "v" + version:
            raise RuntimeError("npm exec did not select the pinned Node runtime")
        node = Path(runtime["node"]).resolve(strict=True)
        pinned_npm = Path(shutil.which("npm", path=runtime["path"])).resolve(strict=True)
        npm_version = subprocess.check_output([str(node), str(pinned_npm), "--version"], env=env, text=True).strip()
        if npm_version != NPM_VERSION:
            raise RuntimeError("npm exec did not select the pinned npm")
        with tempfile.TemporaryDirectory(prefix=f"node-{version}-", dir=cache) as temporary:
            clean = Path(temporary)
            toolbin = clean / "blocked-toolchain"
            toolbin.mkdir()
            (toolbin / "node").symlink_to(node)
            marker = clean / "compiler-attempts.txt"
            for tool in ("cmake", "cc", "gcc", "clang", "c++", "g++", "clang++", "make", "ninja", "pkg-config", "zig"):
                stub = toolbin / tool
                stub.write_text(f"#!{sys.executable}\nfrom pathlib import Path\nPath({str(marker)!r}).write_text({tool!r})\nraise SystemExit(98)\n")
                stub.chmod(0o755)
            installed_env = {**env, "PATH": str(toolbin) + os.pathsep + "/usr/bin:/bin"}
            (clean / "package.json").write_text('{"name":"cui-private-runtime-check","version":"0.0.0","private":true,"main":"index.cjs"}')
            command([node, pinned_npm, "install", "--no-audit", "--no-fund", tarball], cwd=clean, env=installed_env)
            if marker.exists():
                raise RuntimeError("prebuilt addon install attempted a blocked compiler")
            package = clean / "node_modules/@cui/native"
            addon = package / "native/cui.node"
            if not addon.is_file():
                raise RuntimeError("installed prebuilt addon missing")
            # Unmodified copied suites resolve their relative wrapper imports here,
            # then resolve the actual installed package, never the checkout wrapper.
            (clean / "index.cjs").write_text("module.exports = require('@cui/native');\n")
            tests = clean / "validation"
            tests.mkdir()
            shutil.copy2(ROOT / "tests/packaging/node_smoke.cjs", clean / "node_smoke.cjs")
            command([node, clean / "node_smoke.cjs"], cwd=clean, env=installed_env)
            for source, flags, arguments in SUITES:
                target = tests / Path(source).name
                shutil.copy2(ROOT / "bindings/node" / source, target)
                suite_env = dict(installed_env)
                if source == "test/addon.test.cjs":
                    suite_env["CUI_NODE_ADDON"] = str(addon)
                command([node, *flags, target, *arguments], cwd=clean, env=suite_env)
            if marker.exists():
                raise RuntimeError("runtime validation attempted a blocked compiler")
            print(f"Node {version} (runtime Node-API {runtime['napi']}), npm {NPM_VERSION}: "
                  "installed smoke + 7 headless suites PASS", flush=True)
    print("SDL-only native and clipboard suites excluded; no desktop display or publication used.")


if __name__ == "__main__":
    main()
