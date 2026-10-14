# Local alpha binding artifacts

These packages contain the tested binding subset, not full GPUI or all CUI API
parity. They are local alpha artifacts. No registry account ownership is assumed
and these scripts never publish. npm retains `private: true`; the Rust crate has
`publish = false`; the Go module retains the existing local module identity.

## Python wheel and npm tarball

```sh
python scripts/package_bindings.py python --output /tmp/cui-artifacts
python scripts/package_bindings.py node --output /tmp/cui-artifacts
python scripts/test_binding_packages.py --artifacts /tmp/cui-artifacts
```

The Python artifact is a platform wheel (`py3-none-linux_<architecture>`), not a
pure Python wheel or a manylinux compatibility claim. It contains the compiled
portable bridge, source roots for core/bridge/backends, the dependency manifest,
and vendored dependency licenses. Its in-tree PEP 517 backend needs no Python
build dependencies. A staged directory also supports `pip wheel --no-deps`.
Installed `App()` discovers the package-local bridge. Explicit `library=` and
`CUI_BRIDGE_LIBRARY` still take precedence; a broken override reports an error.

The npm tarball contains CUI source and licenses and builds its addon/bridge in
an install hook. The addon build explicitly selects Node-API version 8 and
rejects older runtimes. Installation needs Node >=18 with N-API headers, CMake >=3.20,
a C11 compiler and its build tool. It does not download native build dependencies.
The addon and bridge are placed together with an `$ORIGIN` lookup. TypeScript
exports point inside the package. Existing `CUI_NODE_ADDON` overrides remain
supported. Missing packaged binaries fail directly instead of silently using a
checkout's build directory. Install scripts must be enabled, or run `npm run build`
inside the installed package before importing it.

The validation script installs the actual artifacts in a temporary virtualenv and
npm project, clears checkout/module/library overrides, copies only smoke scripts,
and executes real component frames and click callbacks. The installed Python
SDL smoke also captures the invisible window and checks rendered Button pixels. It also checks explicit
overrides, broken overrides, and missing packaged binary diagnostics. No source
checkout or PYTHONPATH is used by the installed applications.

## Optional SDL packages and runtime limits

```sh
CUI_PACKAGE_NATIVE=1 python scripts/package_bindings.py python --output /tmp/cui-native-artifacts
python scripts/package_bindings.py node --output /tmp/cui-native-artifacts
python scripts/test_binding_packages.py --artifacts /tmp/cui-native-artifacts --native
```

`--native` launches a dedicated invisible Xvfb session and uses an existing font
selected by fontconfig, or `--font /path/to/font.ttf`. The glyph smoke includes
`✓`; the font must cover the submitted text. Local full binding checks use
`/usr/share/fonts/Adwaita/AdwaitaMono-Regular.ttf`, since plain NotoSans lacks
that glyph here; missing glyph errors are preserved, not suppressed. It never uses the desktop
Wayland session or DISPLAY=:0. npm's validation install requests an SDL build.
For a separate manual npm install, set `CUI_PACKAGE_NATIVE=1` during installation.
Python's selected native/portable backend is fixed when its wheel is built.
Keep portable and SDL artifacts in separate output directories: they currently
have the same package versions/filenames and represent alternative local builds.

SDL builds require system SDL3 >=3.2, FreeType, HarfBuzz and their development
headers/pkg-config metadata. These system libraries are **not bundled or pinned**
by this alpha packaging. Their transitive runtime libraries and an explicit font
must exist on the target host. Therefore SDL artifacts are source-complete but
are not self-contained native runtime distributions. `_native/build-info.json`
(Python) and `native/build-info.json` (npm) record the actual built shared-library
runtime dependency names, development versions and source revision. Source
file hashes are recorded in `SOURCE_FILES.sha256`, alongside the pinned
third-party manifest and license texts. Portable builds need the host's
standard C/math runtime; native builds add the precise dependencies in that file.
Linux x86_64 is locally validated. macOS/Windows, manylinux repair, dependency
runtime bundling, cross compilation and registry publication remain unvalidated.

C/CMake warnings stay enabled and errors remain fatal. Artifact builders default
to Release; `CUI_PACKAGE_BUILD_TYPE=Debug` explicitly selects a diagnostic build.
Vendored source versions/licenses remain in `third_party/dependencies.json`.
The configured SDL fetch pin remains in the bundled source, but packaging does
not enable network fetching or silently replace system development dependencies.

Authoritative packaging references: [Python packaging flow](https://packaging.python.org/en/latest/flow/),
[npm pack](https://docs.npmjs.com/cli/v12/commands/npm-pack/),
[npm lifecycle scripts](https://docs.npmjs.com/misc/scripts/),
[Cargo native build scripts](https://doc.rust-lang.org/cargo/reference/build-scripts.html),
[Go module ZIP rules](https://go.dev/ref/mod#zip-files), and
[Go embedded files](https://pkg.go.dev/embed).
