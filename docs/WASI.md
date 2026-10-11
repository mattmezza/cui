# Optional WASI verification

The portable C11 core, base models, styled controls, geometry-only software
renderer, and standalone header distributions can be compiled to `wasm32-wasi`
and executed under a WASI Preview 1 runtime. This is headless runtime evidence;
it does not establish browser, JavaScript host, canvas, WebGPU, DOM input,
clipboard, font shaping, or accessibility support.

The optional runner pins Zig **0.16.0** and Python wasmtime **49.0.0**. Zig is the
C compiler, linker, WASI libc, and archive tool. `uv` resolves the Python runtime
dependency from the script's PEP 723 metadata without a project environment or
privileged installation. No CMake changes or native graphics dependencies are
needed. Existing Zig and uv installations are sufficient:

```sh
uv run scripts/test_wasi.py
```

By default the script uses the repository containing it and preserves modules,
objects, and individual module stdout/stderr under `build/wasi`. `--root PATH`
selects another complete CUI checkout; `--build-dir PATH` selects the output
directory. `--zig PATH` locates a non-PATH Zig installation. Other Zig/wasmtime
versions are deliberately rejected until the pins are reviewed.

The runner builds an optimized portable static archive with C11,
`-Wall -Wextra -Wpedantic -Werror`, and enabled assertions. It then compiles and
executes 15 modules: bootstrap, core, base, components, render, edit, overlay,
virtual, dock, plot, implementation embedding, two examples, and single-/multi-TU
header distribution consumers. Each module gets an 8 MiB stack and a configurable
instruction fuel budget (10 billion by default); exceeding the budget fails the suite. Runtime exits,
traps, and compilation failures fail verification and are reported individually.
The runtime receives no filesystem preopens or inherited environment variables.
Host-side stdout/stderr files contain only the tested module's output.

WASI does not expose a process CPU clock. The virtual test contains a separately
invoked benchmark branch using `clock()`, so this runner explicitly enables
`_WASI_EMULATED_PROCESS_CLOCKS` and links `wasi-emulated-process-clocks`. That
compatibility layer measures wall time. The runner invokes only the normal
assertion path, not the optional benchmark. Per-suite times in the transcript
include compilation/runtime preparation and are not framework performance claims.

The transcript records the source Git revision, whether tracked modifications
were present, and a SHA256 fingerprint of all portable sources, relevant tests,
public/distribution headers, and vendored Clay. If those files change while the
runner executes, verification fails and must be repeated on a stable checkout.
Generated output files are preserved and no unrelated paths are cleaned.

Actual run evidence is in [validation/wasi.txt](validation/wasi.txt): **15/15
modules compiled and passed runtime assertions** against source revision
`8ff89cad797e8a5737532b7249323db26bbb72fb`, with tracked modifications
explicitly recorded and portable source fingerprint checked for stability. An earlier
run is preserved in [validation/wasi-initial.txt](validation/wasi-initial.txt):
14/15 passed, and the dock serializer's decimal roundtrip assertion failed.
That real target-specific failure prompted a parser correction; it was not
skipped or converted into a compilation-only claim. SDL/native renderer tests
and FreeType/HarfBuzz shaping are outside this runner's tested surface.
