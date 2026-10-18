"""Dependency-free PEP 517 backend for staged, host-platform alpha wheels."""
import base64
import csv
import hashlib
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import sysconfig
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
NAME = "cui_bridge-0.1.0a2"

def get_requires_for_build_wheel(config_settings=None):
    return []

def build_wheel(wheel_directory, config_settings=None, metadata_directory=None):
    if not sys.platform.startswith("linux"):
        raise RuntimeError("Alpha binary wheel builder currently validates Linux only")
    package = ROOT / "cui_bridge"
    source = package / "_source"
    if not (source / "bindings/bridge/CMakeLists.txt").is_file():
        raise RuntimeError("Stage sources with scripts/package_bindings.py python first")
    native = os.environ.get("CUI_PACKAGE_NATIVE", "0") == "1"
    if native:
        subprocess.run(["pkg-config", "--exists", "sdl3 >= 3.2", "freetype2", "harfbuzz"], check=True)
    with tempfile.TemporaryDirectory(prefix="cui-wheel-build-") as temporary:
        build = Path(temporary)
        subprocess.run(["cmake", "-S", str(source / "bindings/bridge"), "-B", str(build),
                        "-DCUI_BUILD_NATIVE=" + ("ON" if native else "OFF"),
                        "-DCMAKE_BUILD_TYPE=" + os.environ.get("CUI_PACKAGE_BUILD_TYPE", "Release"), "-DCMAKE_BUILD_RPATH_USE_ORIGIN=ON"], check=True)
        subprocess.run(["cmake", "--build", str(build), "--target", "cui_bridge", "-j2"], check=True)
        library = build / "libcui_bridge.so"
        if not library.is_file():
            raise RuntimeError("Expected Linux shared bridge output is missing")
        tag = "py3-none-" + sysconfig.get_platform().replace("-", "_").replace(".", "_")
        filename = f"{NAME}-{tag}.whl"
        info = f"{NAME}.dist-info"
        entries = {str(p.relative_to(ROOT)): p.read_bytes() for p in package.rglob("*")
                   if p.is_file() and "__pycache__" not in p.parts}
        entries["cui_bridge/_native/libcui_bridge.so"] = library.read_bytes()
        dependencies = subprocess.check_output(["ldd", str(library)], text=True)
        entries["cui_bridge/_native/build-info.json"] = json.dumps({
            "alpha": True, "native_requested": native, "platform": sysconfig.get_platform(),
            "system_development_versions": dict(zip(("sdl3", "freetype2", "harfbuzz"),
                subprocess.check_output(["pkg-config", "--modversion", "sdl3", "freetype2", "harfbuzz"], text=True).splitlines())) if native else {},
            "system_runtime_dependencies": [line.split()[0] for line in dependencies.splitlines() if line.split()],
            "source_revision": (source / "SOURCE_REVISION").read_text().strip()
        }, indent=2).encode()
        entries[f"{info}/METADATA"] = ("Metadata-Version: 2.4\nName: cui-bridge\nVersion: 0.1.0a2\n"
            "Summary: Alpha CUI bridge builders; tested subset, not full binding parity\n"
            "Requires-Python: >=3.10\nLicense-Expression: MIT\n").encode()
        entries[f"{info}/WHEEL"] = f"Wheel-Version: 1.0\nGenerator: cui-alpha\nRoot-Is-Purelib: false\nTag: {tag}\n".encode()
        entries[f"{info}/licenses/LICENSE"] = (source / "LICENSE").read_bytes()
        rows = []
        for path, data in entries.items():
            digest = base64.urlsafe_b64encode(hashlib.sha256(data).digest()).rstrip(b"=").decode()
            rows.append((path, "sha256=" + digest, len(data)))
        rows.append((f"{info}/RECORD", "", ""))
        record = io.StringIO(newline="")
        csv.writer(record).writerows(rows)
        entries[f"{info}/RECORD"] = record.getvalue().encode()
        output = Path(wheel_directory)
        output.mkdir(parents=True, exist_ok=True)
        with zipfile.ZipFile(output / filename, "w", zipfile.ZIP_DEFLATED) as archive:
            for path, data in entries.items():
                archive.writestr(path, data)
        return filename
