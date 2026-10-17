---
name: cui
description: Build and debug CUI user interfaces in C11, Python, JavaScript, TypeScript, Go or Rust, for native, browser/WASM, headless or embedded hosts. Use for CUI setup, component/API discovery, layout, fonts, themes, responsive composition and backend integration.
---

# Build with CUI

Use the Python 3.9+ standard-library helper beside this file. Resolve its absolute
path from this skill's location; do not assume the skill is in the app checkout.
In commands below, `HELPER` means that absolute `scripts/cui.py` path. No pip
packages are required. The skill includes offline documentation, headers and
examples; it does not include the CUI implementation or compiler toolchain.

## Start with the project's actual integration

Inspect the existing app before choosing a language, host or renderer. Preserve
its stack and dependency strategy. For a new app, infer the intended target from
the request; ask only when the choice materially affects the result and is unclear.
Read `docs/START_HERE.md` through the helper, then only relevant guides below.

```sh
python "$HELPER" read docs/START_HERE.md
python "$HELPER" doctor --profile portable
python "$HELPER" search "searchable dropdown" --kind component
python "$HELPER" component Select --show usage
python "$HELPER" component Select --show limitations
python "$HELPER" component Select --show api
python "$HELPER" component Select --show examples
```

`search` supports component, api and guide filters, weighted literal words, and
labelled partial matches when no entry matches every word. Try related terms or
`list` if a concept is not found. `component --show all` includes everything.
`api SYMBOL` returns exact matching function declarations and complete headers,
including types, callbacks, macros and comments. `read PATH` prints an exact
bundled document or example; `info` lists available paths and content hashes.
Use `--json` for machine-readable search, component and list results.

Run `doctor --profile TARGET` for the chosen path. For a native window, also run
`doctor --profile native --font /absolute/font.ttf`. These are read-only probes,
not proof that headers, ABI versions or GPU drivers work: configure and build to
verify those. Explain missing dependencies and obtain any necessary authorization
before installing system packages. Do not ask users to manually confirm things
the probe can check. Native Windows prerequisites may need manual/CMake discovery
instead of pkg-config; do not infer unsupported platform parity.

## Pin setup and keep references aligned

For a new source workspace, resolve the desired upstream branch/tag once, then
use its explicit 40-character commit SHA:

```sh
python "$HELPER" resolve --ref main
python "$HELPER" vendor --revision COMMIT_SHA --dest vendor/cui
python "$HELPER" init --template web --revision COMMIT_SHA --dest my-app
```

`init` supports native, portable, web, python, node, typescript, go, rust,
embedding, sdl3, sdl-gpu, sokol and software. `native` creates the existing
standalone SDL3 source bundle, with fonts and Makefile. Other templates create a
complete pinned source workspace and identify an existing example/adapter to
adapt, retaining its build context. They do not synthesize a new production host.
Read the printed entry point and its guide before changing it.

For offline setup, pass `--source /path/to/cui` to vendor/init. This archives
committed Git files only, defaulting to local HEAD; uncommitted edits are excluded.
Alternatively supply `--archive FILE --revision COMMIT_SHA --sha256 HASH` for an
upstream source tarball. Downloads use an immutable GitHub commit URL; the archive
hash is recorded and an optional `--sha256` enforces a previously known checksum.
Existing destinations are refused. No dependency installs or app launches occur.

The helper records `.cui-lock.json` (revision, archive hash and original file
hashes) and `.cui-resources.json` (matching discovery data). Run
`verify --source PROJECT` to detect changes to the original snapshot; application
edits are reported too. New files are not tracked by this verification.

**After setup, pass `--source /path/to/project-or-vendored-cui` on discovery
commands.** For full trees this reads current local headers/docs; standalone
bundles use their matching packaged references. Without `--source`, lookup uses
the installed skill snapshot, which may differ from the app's CUI revision.
Never silently upgrade an existing project to match a newer skill.

## Choose only the references needed

Read these with `read PATH --source CUI_SOURCE`:

| Work | References |
| --- | --- |
| Native C11 app and distribution | `docs/NATIVE_APP_STARTER.md`, `examples/native_bundle/README.md`, `examples/native_starter/README.md` |
| Browser/WASM and Canvas host | `docs/WEB_GALLERY.md`, `docs/WASI.md`; inspect `web/gallery.c` and `web/gallery.js` |
| Python, Node and TypeScript authoring | `docs/BINDING_BUILDERS.md`, `docs/BRIDGE_COMPONENTS.md`, `docs/binding-coverage.json` |
| Go or Rust authoring | `docs/GO_BUILDERS.md` or `docs/RUST_BUILDERS.md`, then bridge coverage |
| Hosting C-authored UI from another language | `docs/LANGUAGE_BINDINGS.md` (different from language component builders) |
| Embedding/custom host or renderer | `docs/INTERFACES.md`, `docs/PLATFORM_MATRIX.md`, relevant `backends/*/README.md` |
| Layout and responsive composition | `docs/LAYOUT_GUIDE.md`, `docs/APP_RECIPES.md`, `docs/FORMS.md`, `docs/COMPOSED_APPS.md` |
| Fonts, metrics, fallback and shaping | `docs/TYPOGRAPHY.md`, `docs/TEXT.md`, `docs/BIDI_SHAPING.md`, `docs/IME.md` |
| Theme, density, colors and icons | `docs/DESIGN_SYSTEM.md`, `docs/THEME_GUIDE.md`, `docs/THEME_REGISTRY_GUIDE.md`, `docs/ICON.md` |
| State, identity, ownership and failures | `docs/IMMEDIATE_AND_STATE.md`, `docs/IDS_AND_LIFETIMES.md`, `docs/DEBUGGING.md` |
| Accessibility and touch | `docs/ACCESSIBILITY.md`, `docs/NATIVE_ACCESSIBILITY.md`, `docs/TOUCH_COMPONENT_COVERAGE.md` |
| Optional regex provider | `backends/regex/README.md`, `docs/INPUT_PATTERN_FORMATTING.md` |
| Platform prerequisites and evidence | `docs/DEPENDENCIES.md`, `docs/PLATFORM_MATRIX.md`, `docs/V1_READINESS.md` |

Configure a real font and assets for the selected host. Keep measuring and
painting font configuration consistent; arrange fallback/shaping for the scripts
actually displayed. Read the exact text APIs before implementing font families.
For responsive UI, derive available space from the host and use the documented
layout constraints. Test narrow and wide viewports, long content and overflow;
do not transplant CSS/browser assumptions into the C layout model.

## Compose and verify

CUI's C API composes visible UI every frame; application data and explicit models
persist. Language builders have their own retained authoring facade over this
engine. Follow that binding's event and ownership contract.

- Use stable IDs for interactive elements, including reordered rows. Retain values,
  edit buffers and explicit models for their documented lifetimes.
- Balance successful begin/end calls. Paint borrowed draw data before the next
  frame; keep borrowed inputs alive for the documented interval.
- Use real declared APIs, types and callbacks. Read each chosen component's
  scope, feature status and limitations; 78 catalog families do not imply full
  behavior, binding-option or platform parity.
- Keep portable UI separate from host and renderer code. Build the unchanged
  chosen example first, then implement the requested UI and its interactions.
- Compile with warnings. Test triggering interactions and expected state changes,
  keyboard/focus behavior and responsive bounds where relevant. A screenshot or
  successful compilation alone does not establish interaction correctness.

For repository builds:

```sh
cmake -S . -B build -G Ninja
cmake --build build
python scripts/test_local.py --build-dir build
```

For portable builds set `-DCUI_BUILD_NATIVE=OFF`; binding and standalone-bundle
builds use their own guide commands. On Linux, native automated tests must use
invisible Xvfb through the wrapper, never DISPLAY=:0 or a user's Wayland session.
Browser tests run headless. GPU observations unsupported by the virtual display
remain explicitly deferred. Never auto-launch a native example on the desktop
as an automated check. Report what actually ran and which host/backend it used.
