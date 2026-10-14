# Install local alpha binding artifacts

The packages expose the [tested binding subset](BRIDGE_COMPONENTS.md), not full
GPUI feature parity. Commands below build and consume local files; no registry
publication or public Go tag is assumed. npm is `private: true` and Cargo has
`publish = false`. Linux x86_64 is validated; registry ownership, public release
versions, other platforms and native runtime bundling remain release work.

Run artifact build commands from the CUI repository root. The artifact producer scripts require Python 3, CMake
3.20+, a C11 compiler and build tools. Node builds also require Node >=18 and
Node-API headers; Go requires Go 1.23+ with cgo, and Rust requires Cargo/Rust.
Builds default to Release with strict warnings. Use
`CUI_PACKAGE_BUILD_TYPE=Debug` explicitly for diagnostic builds.

**Portable is the default:** an App computes layout and processes input but
opens no window. Build with `CUI_PACKAGE_NATIVE=1`, then select the SDL backend
in the application for a native window. Keep portable and native artifacts in
separate output directories; their versions and filenames currently coincide.
Native builds need SDL3 >=3.2, FreeType and HarfBuzz development packages.

## Python: wheel, virtualenv and native counter

```sh
CUI_PACKAGE_NATIVE=1 python scripts/package_bindings.py python --output /tmp/cui-python-native
python -m venv /tmp/cui-python-consumer
/tmp/cui-python-consumer/bin/python -m pip install --no-deps /tmp/cui-python-native/*.whl
```

Save this as `/tmp/cui-python-consumer/counter.py`. It creates a real native UI;
the callback updates mounted label state. Closing the window ends `app.run()`.

```python
import os
from cui_bridge import App, button, column, label

count = 0
status = label("Clicks: 0")
def increment(event):
    global count
    count += 1
    status.label(f"Clicks: {count}")

click = button("increment").label("Click me").on_click(increment)
with App(backend="sdl", title="CUI Counter", font_path=os.environ["CUI_FONT"]) as app:
    app.mount(column().children(status, click))
    if os.environ.get("CUI_EXAMPLE_SMOKE") == "1":
        app.step()
        x, y, w, h = app.bounds(click)
        app.queue_pointer("down", x + w / 2, y + h / 2)
        app.queue_pointer("up", x + w / 2, y + h / 2)
        app.step()
        app.dispatch()
        assert count == 1
        app.step()
    else:
        app.run()
```

Choose an existing font covering your application's text, then run:

```sh
CUI_FONT=/path/to/font.ttf /tmp/cui-python-consumer/bin/python /tmp/cui-python-consumer/counter.py
```

For portable layout/input only, omit `CUI_PACKAGE_NATIVE=1` when building and
use `App()` instead of the SDL options. The platform wheel includes its compiled
bridge, source roots, pinned dependency manifest and licenses. It is not a pure
Python wheel or a manylinux claim. No checkout/PYTHONPATH or separate library path
is needed. Explicit `library=` and `CUI_BRIDGE_LIBRARY` overrides retain precedence;
a broken override fails rather than hiding the error with a default library.

## JavaScript: prebuilt tarball and native counter

```sh
CUI_PACKAGE_NATIVE=1 python scripts/package_bindings.py node --prebuilt --output /tmp/cui-node-native
mkdir -p /tmp/cui-node-consumer
cd /tmp/cui-node-consumer
npm init -y
CUI_PACKAGE_NATIVE=1 npm install /tmp/cui-node-native/cui-native-0.1.0-alpha.1.tgz
```

Save this as `/tmp/cui-node-consumer/counter.cjs`:

```javascript
const {App, button, column, label} = require('@cui/native');
let count = 0;
const status = label('Clicks: 0');
const click = button('increment').label('Click me').on_click(() => {
  status.label(`Clicks: ${++count}`);
});
const app = new App({backend: 'sdl', title: 'CUI Counter', fontPath: process.env.CUI_FONT});
app.mount(column().children(status, click));
if (process.env.CUI_EXAMPLE_SMOKE === '1') {
  app.step();
  const {x, y, width, height} = click.bounds();
  app.pointer('down', x + width / 2, y + height / 2);
  app.pointer('up', x + width / 2, y + height / 2);
  app.step();
  if (count !== 1) throw new Error('counter callback failed');
  app.step();
  app.dispose();
} else {
  app.start();
}
```

```sh
CUI_FONT=/path/to/font.ttf node /tmp/cui-node-consumer/counter.cjs
```

For portable behavior build without `CUI_PACKAGE_NATIVE=1` and use `new App()`.
`--prebuilt` carries a hashed Node-API 8 addon and bridge; the consumer needs
Node >=18 and system runtime libraries, without Python or a C compiler. Omit `--prebuilt` for a source tarball whose
install hook requires Python 3, CMake, a C11 compiler, build tools and Node headers. Install scripts must be enabled.
`CUI_PACKAGE_BUILD_FROM_SOURCE=1` explicitly rebuilds bundled source. TypeScript
declarations are inside the package and validated using strict TypeScript 5.8.3.
`CUI_NODE_ADDON` remains an explicit override; missing installed binaries fail
without a checkout fallback.

## Go: local module proxy and executable

Build a portable artifact at the repository root:

```sh
python scripts/package_bindings.py go --output /tmp/cui-go-artifacts
mkdir -p /tmp/cui-go-consumer
cp tests/packaging/go_smoke.go /tmp/cui-go-consumer/main.go
cd /tmp/cui-go-consumer
go mod init example.local/cui-counter
go mod edit -require=github.com/mattmezza/cui/bindings/go@v0.1.0-alpha.1
GOPROXY=file:///tmp/cui-go-artifacts GOSUMDB=off go mod tidy
GOPROXY=file:///tmp/cui-go-artifacts GOSUMDB=off go build -o counter .
./counter
```

This example renders a portable frame, sends a click and asserts the callback.
The canonical module path describes this repository; these commands use a local
proxy ZIP, not an unpublished registry/tag. The default artifact embeds a compiled
Linux bridge into the Go executable, extracted into a verified private cache.
It needs no CMake/compiler at runtime. Native system libraries remain required.

To validate native rendering, build the Go artifact with `CUI_PACKAGE_NATIVE=1`
in a different output directory, use that directory as `GOPROXY`, then run the
example with `CUI_PACKAGE_SMOKE_BACKEND=sdl CUI_PACKAGE_SMOKE_FONT=/path/to/font.ttf`.
For an interactive application, see the [Go workspace UI](GO_BUILDERS.md).

`--source-only` instead creates a module embedding SDK source. Its first App
creation needs CMake and a C11 compiler; this is an alpha development fallback.
A public build-time cgo/static or explicit artifact embedding release path is
still needed for production Go distribution. Cache receipts validate source,
backend, ABI, OS, architecture and library hashes; writes are locked and atomic.
Existing explicit library overrides remain supported.

## Rust: local crate and statically linked executable

At the repository root:

```sh
python scripts/package_bindings.py rust --output /tmp/cui-rust-artifacts
mkdir -p /tmp/cui-rust-consumer/vendor
cp tests/packaging/rust_smoke.rs /tmp/cui-rust-main.rs
tar -xzf /tmp/cui-rust-artifacts/cui-builders-0.1.0-alpha.1.crate -C /tmp/cui-rust-consumer/vendor
cd /tmp/cui-rust-consumer
cargo init --name cui-counter
cp /tmp/cui-rust-main.rs src/main.rs
cat >> Cargo.toml <<'TOML'
cui-builders = { path = "vendor/cui-builders-0.1.0-alpha.1" }
TOML
cargo build --offline
./target/debug/cui-counter
```

Cargo compiles the bundled SDK and statically links CUI during the developer build.
The copied executable needs no CUI checkout, CMake or compiler at runtime.
The example asserts portable geometry and a callback. For native rendering,
add `features = ["native"]` to the dependency entry, then run it with
`CUI_PACKAGE_SMOKE_BACKEND=sdl CUI_PACKAGE_SMOKE_FONT=/path/to/font.ttf`.
Native system libraries are still dynamically required. The existing
`CUI_BRIDGE_LIB_DIR` shared-library override remains available. See the
[Rust workspace UI](RUST_BUILDERS.md) for a complete interactive application.

## Headless checks and runtime requirements

Automated native checks must use a dedicated invisible Xvfb session, never the
maintainer desktop. These runners create it automatically:

```sh
python scripts/test_binding_packages.py --artifacts /tmp/cui-native-artifacts --native --require-prebuilt --font /path/to/font.ttf
python scripts/test_compiled_binding_packages.py --go-artifacts /tmp/cui-go-native --rust-artifacts /tmp/cui-rust-artifacts --native --font /path/to/font.ttf
```

The first directory must contain both the native wheel and prebuilt npm tarball;
build both there using the earlier commands. The Go directory must contain a
native Go artifact. Rust's consumer selects its native feature. For the standalone
counter examples, `CUI_EXAMPLE_SMOKE=1` sends and checks a synthetic click and
exits; wrap their invocation in `xvfb-run -a env -u WAYLAND_DISPLAY`.

The installed artifact tests use clean temporary consumers without checkout or
library-path overrides, exercise real frames/callbacks, and block compiler tools
at runtime. Python native validation also checks captured Button pixels.
The text smoke includes `✓`; fonts must cover submitted text. This host validates
`/usr/share/fonts/Adwaita/AdwaitaMono-Regular.ttf`; plain NotoSans lacks that glyph
here. Missing glyph errors remain visible. Full workspace examples and supported
options are in [Python/JavaScript/TypeScript builders](BINDING_BUILDERS.md),
[Go builders](GO_BUILDERS.md), and [Rust builders](RUST_BUILDERS.md).

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


## Source receipts and release work

Artifacts include tracked source only, file hashes, pinned dependency manifests
and license texts. Revision receipts include dirty-source information so a
revision is not mistaken for an exact clean snapshot. Go and Rust use minimal
SDK snapshots: after canonical changes run `python scripts/generate_go_sdk.py`,
then `python scripts/generate_go_sdk.py --check` to validate freshness/provenance.
No packaging command publishes. Public tag/registry release, Linux runtime
bundling, manylinux compatibility and cross-platform binaries are pending.

Authoritative packaging references: [Python packaging flow](https://packaging.python.org/en/latest/flow/),
[npm pack](https://docs.npmjs.com/cli/v12/commands/npm-pack/),
[npm lifecycle scripts](https://docs.npmjs.com/misc/scripts/),
[Cargo native build scripts](https://doc.rust-lang.org/cargo/reference/build-scripts.html),
[Go module ZIP rules](https://go.dev/ref/mod#zip-files), and
[Go embedded files](https://pkg.go.dev/embed).
