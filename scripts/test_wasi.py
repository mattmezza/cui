#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["wasmtime==49.0.0"]
# ///
"""Compile portable CUI suites for WASI Preview 1 and execute their assertions."""
from __future__ import annotations

import argparse
import importlib.metadata
import hashlib
import pathlib
import subprocess
import sys
import time

ZIG_VERSION = "0.16.0"
WASMTIME_VERSION = "49.0.0"
SOURCES = (
    "src/core/cui.c",
    "src/base/cui_base.c",
    "src/base/cui_models.c",
    "src/base/cui_edit.c",
    "src/base/cui_overlay.c",
    "src/base/cui_virtual.c",
    "src/base/cui_dock.c",
    "src/base/cui_plot.c",
    "src/theme/cui_theme.c",
    "src/components/cui_components.c",
    "src/base/cui_content.c",
    "src/components/cui_content.c",
    "src/base/cui_disclosure.c",
    "src/components/cui_disclosure.c",
    "src/base/cui_choice.c",
    "src/components/cui_choice.c",
    "backends/software/cui_software.c",
)
SUITES = ("bootstrap", "core", "base", "components", "render", "edit", "overlay", "virtual", "dock", "plot", "content", "disclosure", "choice")


def source_fingerprint(root: pathlib.Path) -> str:
    paths = list(SOURCES)
    paths += [f"tests/unit/test_{name}.c" for name in SUITES]
    paths += ["tests/unit/test_core_embed.c", "tests/distribution/single.c",
              "tests/distribution/implementation.c", "tests/distribution/consumer.c"]
    paths += [f"examples/{name}/main.c" for name in ("unstyled", "styled_settings")]
    paths += [str(path.relative_to(root)) for folder in ("include/cui", "dist")
              for path in (root / folder).glob("*.h")]
    paths += ["third_party/clay/clay.h"]
    digest = hashlib.sha256()
    for path in sorted(set(paths)):
        digest.update(path.encode() + b"\0")
        digest.update((root / path).read_bytes())
    return digest.hexdigest()


def command(args: list[str], root: pathlib.Path) -> None:
    result = subprocess.run(args, cwd=root, capture_output=True, text=True, timeout=180)
    if result.returncode:
        raise RuntimeError(
            f"Command failed ({result.returncode}): {' '.join(args)}\n"
            f"{result.stdout}{result.stderr}"
        )
    if result.stdout or result.stderr:
        print(result.stdout + result.stderr, end="", flush=True)


def run_module(path: pathlib.Path, engine: object, fuel: int) -> None:
    import wasmtime

    stdout = path.with_suffix(".stdout.txt")
    stderr = path.with_suffix(".stderr.txt")
    status: Exception | None = None
    with wasmtime.Store(engine) as store:
        store.set_fuel(fuel)
        wasi = wasmtime.WasiConfig()
        wasi.argv = [path.stem]
        wasi.stdout_file = str(stdout)
        wasi.stderr_file = str(stderr)
        # Deliberately grant no filesystem preopens or environment variables.
        store.set_wasi(wasi)
        with wasmtime.Linker(engine) as linker:
            linker.define_wasi()
            with wasmtime.Module.from_file(engine, str(path)) as module:
                instance = linker.instantiate(store, module)
                try:
                    instance.exports(store)["_start"](store)
                except wasmtime.ExitTrap as exc:
                    if exc.code:
                        status = exc
                except wasmtime.Trap as exc:
                    status = exc
    for output in (stdout, stderr):
        if output.exists():
            text = output.read_text(errors="replace").strip()
            if text:
                print(text, flush=True)
    if status:
        raise RuntimeError(f"{path.stem} trapped or exited unsuccessfully: {status}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path(__file__).resolve().parents[1])
    parser.add_argument("--build-dir", type=pathlib.Path, help="defaults to ROOT/build/wasi")
    parser.add_argument("--zig", default="zig", help="path to Zig 0.16.0")
    parser.add_argument("--fuel", type=int, default=10_000_000_000, help="per-suite instruction budget")
    args = parser.parse_args()
    root = args.root.resolve()
    build = (args.build_dir or root / "build/wasi").resolve()
    if args.fuel <= 0:
        parser.error("--fuel must be positive")
    build.mkdir(parents=True, exist_ok=True)
    actual_zig = subprocess.check_output([args.zig, "version"], text=True).strip()
    actual_wasmtime = importlib.metadata.version("wasmtime")
    if actual_zig != ZIG_VERSION or actual_wasmtime != WASMTIME_VERSION:
        raise RuntimeError(
            f"Pinned tools required: Zig {ZIG_VERSION}, wasmtime {WASMTIME_VERSION}; "
            f"found {actual_zig}, {actual_wasmtime}"
        )
    import wasmtime

    config = wasmtime.Config()
    config.consume_fuel = True
    with wasmtime.Engine(config) as engine:
        revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root, text=True).strip()
        dirty = subprocess.run(["git", "diff", "--quiet", "HEAD"], cwd=root).returncode != 0
        fingerprint = source_fingerprint(root)
        print(f"CUI source revision: {revision}", flush=True)
        print(f"Portable source SHA256: {fingerprint}", flush=True)
        print("Tracked source modifications: " + ("present" if dirty else "none"), flush=True)
        print(f"Target: wasm32-wasi (WASI Preview 1), Zig {actual_zig}, wasmtime {actual_wasmtime}", flush=True)
        print("Optional adapters disabled: SDL, GPU, FreeType, HarfBuzz; no browser host tested.", flush=True)
        print("Assertions enabled. Virtual clock() compatibility uses wall time; benchmarks not invoked.", flush=True)
        flags = [args.zig, "cc", "-target", "wasm32-wasi", "-std=c11", "-O2", "-UNDEBUG",
                 "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-I", str(root / "include"),
                 "-I", str(root / "third_party/clay")]
        objects = []
        for source in SOURCES:
            obj = build / (source.replace("/", "_").replace(".c", ".o"))
            command(flags + ["-c", str(root / source), "-o", str(obj)], root)
            objects.append(str(obj))
        archive = build / "libcui-portable.a"
        command([args.zig, "ar", "rcs", str(archive), *objects], root)
        # WASI has no process CPU clock. The virtual suite contains an optional
        # benchmark branch using clock(), so compile its explicit wall-clock
        # compatibility library even though normal assertions do not run it.
        virtual_clock = ["-D_WASI_EMULATED_PROCESS_CLOCKS", "-lwasi-emulated-process-clocks"]
        targets = [(name, [f"tests/unit/test_{name}.c"], True,
                    virtual_clock if name == "virtual" else []) for name in SUITES]
        targets += [("implementation_embedding", ["tests/unit/test_core_embed.c"], False, [])]
        targets += [(f"example_{name}", [f"examples/{name}/main.c"], True, [])
                    for name in ("unstyled", "styled_settings")]
        distribution_flags = ["-I", str(root / "dist"), "-DCUI_DISTRIBUTION_CONTROLS=1"]
        targets += [("distribution_single", ["tests/distribution/single.c"], False, distribution_flags),
                    ("distribution_multi", ["tests/distribution/implementation.c", "tests/distribution/consumer.c"], False, distribution_flags)]
        failures = []
        for name, sources, link_library, extra in targets:
            started = time.monotonic()
            module = build / f"{name}.wasm"
            try:
                command(flags + extra + [str(root / source) for source in sources] +
                        ([str(archive)] if link_library else []) +
                        ["-lm", "-Wl,-z,stack-size=8388608", "-o", str(module)], root)
                run_module(module, engine, args.fuel)
                print(f"PASS {name} ({time.monotonic() - started:.2f}s)", flush=True)
            except (RuntimeError, subprocess.SubprocessError, OSError, wasmtime.WasmtimeError) as exc:
                failures.append(name)
                print(f"FAIL {name}: {exc}", flush=True)
        if source_fingerprint(root) != fingerprint:
            raise RuntimeError("Portable source files changed during verification; rerun on a stable checkout")
        print(f"WASI runtime suites: {len(targets) - len(failures)}/{len(targets)} passed", flush=True)
        if failures:
            print("Failed suites: " + ", ".join(failures), flush=True)
        return int(bool(failures))


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (RuntimeError, subprocess.SubprocessError, OSError) as error:
        print(f"WASI runner failed: {error}", file=sys.stderr)
        sys.exit(1)
