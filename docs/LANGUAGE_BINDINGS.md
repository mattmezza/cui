# Python, Go, Rust, Node JavaScript and TypeScript with CUI

For UI trees authored directly in Python, JavaScript or TypeScript, use
[the component builders](BINDING_BUILDERS.md). Those builders expose rows,
columns, labels and styled buttons through the shared C bridge, with language
callbacks and mutable application state. This is the first component binding
slice; the other manifest components and Go/Rust builders are not exposed yet.

The examples below remain useful for embedding an application whose UI is
written in C.

These runnable examples call the actual CUI library through a small C host. Each
creates a styled **Increment** button, renders a counter, and calls your language
synchronously when the button changes it. With SDL3 and shaped text available,
the same clients open a native desktop window. Without a font argument they run
a deterministic headless interaction check instead.

This is an integration starting point, not a complete binding of CUI's 78
manifest entries. UI construction currently lives in `host.c`; application
callbacks live in Python, Go, Rust or Node. Extend that host ABI to expose the controls
and state your application needs. No Clay or SDL types cross this example ABI.

## Build and run

### Python only: standard-library ctypes

Use Python 3.9+, CMake 3.20+, Ninja and a C11 compiler. Python needs no pip
packages, Node, Go or Rust. Native windows additionally need installed SDL3
3.2+, FreeType, HarfBuzz and a TrueType/OpenType font. Build just the shared host:

```sh
cmake -S examples/foreign_languages -B examples/foreign_languages/build -G Ninja
cmake --build examples/foreign_languages/build --target cui_example_host
# Headless: two actual CUI button activations and Python callbacks.
python3 examples/foreign_languages/python/main.py
# Native: click Increment in the actual SDL3 window; close the window to return.
python3 examples/foreign_languages/python/main.py /usr/share/fonts/noto/NotoSans-Regular.ttf
```

For another build directory, pass `--library /absolute/path/libcui_example_host.so`.
Python imports the [small ctypes wrapper](../examples/foreign_languages/python/main.py):

```python
from main import run

def changed(count):
    print("Application callback:", count)

run(changed, font="/path/to/your/font.ttf")
```

Place this Python code beside the wrapper or import it from your application's
module path. The callback stays alive throughout the synchronous C call.
Python exceptions are captured in the callback and re-raised after the host
returns; they never unwind through C. The native loop blocks the calling Python
thread until close, just like the small Node host below. A production application
should expose a per-frame host API appropriate to its runtime.

### Node JavaScript and TypeScript only

Use the CMake/compiler and native dependencies above, plus Node 22.18+ and
Node development headers (`node_api.h`). Go and Rust are unnecessary for these
clients. No npm packages or node-gyp are required:

```sh
cmake -S examples/foreign_languages -B examples/foreign_languages/build -G Ninja
cmake --build examples/foreign_languages/build --target cui_node
node examples/foreign_languages/node/main.cjs
node examples/foreign_languages/typescript/main.ts
# Open the same native CUI window from either runtime:
node examples/foreign_languages/node/main.cjs /usr/share/fonts/noto/NotoSans-Regular.ttf
node examples/foreign_languages/typescript/main.ts /usr/share/fonts/noto/NotoSans-Regular.ttf
```

The TypeScript sample uses Node's built-in type stripping; it does not perform
static type checking. Both clients use the same N-API addon and C host.

### All five clients

From the repository root, install/use CMake 3.20+, Ninja, a C11 compiler,
Go 1.22+, Rust 1.85+ and Node 22.18+ (with Node development headers containing
`node_api.h`). TypeScript runs through Node's built-in type stripping; no npm,
Cargo or Go packages are downloaded. The examples add no third-party dependencies.
The repository pins Clay and optional fetched SDL3. This build uses installed
SDL3 3.2+, FreeType and HarfBuzz for native windows; if they are unavailable it
prints `headless only`.

```sh
examples/foreign_languages/build.sh
examples/foreign_languages/check.sh
```

The build compiles the C bridge and Node addon with `-Wall -Wextra -Wpedantic
-Werror`, Go's C trampoline with warnings as errors, and Rust with `-D warnings`.
It places all artifacts in `examples/foreign_languages/build/`, including the
shared C host used by all five clients. The CMake subproject builds CUI directly
from this checkout with position independent code. Run on Linux with GCC/Clang;
`build.sh` uses ELF rpath linker flags and needs adaptation for other platforms.

To force a portable headless build, pass `-DCUI_EXAMPLE_NATIVE=OFF` to
`build.sh`. Passing a font to that build returns status 4. To enable repository
pinned SDL fetching when system SDL3 is absent, pass `-DCUI_FETCH_SDL=ON`;
FreeType and HarfBuzz still need to be installed.

Choose an existing TrueType/OpenType font on your machine, then open a native
window with any client. The example below uses the font present on our Linux
validation machine:

```sh
FONT=/usr/share/fonts/noto/NotoSans-Regular.ttf
python3 examples/foreign_languages/python/main.py "$FONT"
examples/foreign_languages/build/go-demo "$FONT"
examples/foreign_languages/build/rust-demo "$FONT"
node examples/foreign_languages/node/main.cjs "$FONT"
node examples/foreign_languages/typescript/main.ts "$FONT"
```

Close each window before starting the next command. Add a positive frame count
(e.g. `"$FONT" 120`) to close automatically; zero runs until window close.
Click **Increment** to update the on-screen count and print the callback value.
Native SDL3 event dispatch, CUI layout, font shaping and rendering run on the
calling thread. Go locks its goroutine to that OS thread. Node runs this loop
synchronously: timers and other JS event-loop work wait until the window closes.
For a production app, design a per-frame host API integrated with your runtime's
main-thread event loop.

## C host contract

[host.h](../examples/foreign_languages/host.h) deliberately exposes a small ABI:

```c
typedef void (*cui_example_changed)(void *user, int count);
int cui_example_run(const char *font, unsigned frames,
                    cui_example_changed callback, void *user);
```

The callback is invoked on the calling thread and is never retained beyond the
call. Its `user` storage must remain alive through `run`. Headless mode lays out
the real button, obtains its hit bounds, queues pointer down/up events twice,
and verifies two state changes and nonempty draw lists with no CUI errors.
The native mode polls SDL3, renders CUI draw commands and optionally captures its
last bounded frame when `CUI_EXAMPLE_CAPTURE` names a BMP output path. Text backing
storage remains alive while the renderer consumes the draw list.

Status zero means success; 2 is font failure, 3 backend creation failure, 4 native
support unavailable, 5 CUI creation failure, 6–8 headless validation failure and
9 native layout/render failure. Native window initialization does not silently
fall back to a headless test.

## Language examples

[Go](../examples/foreign_languages/go/main.go) uses cgo and an exported Go callback:

```go
//export changed
func changed(count C.int) {
    observed = int(count)
    fmt.Println("Go count:", observed)
}
```

A C trampoline passes that callback to the host. No Go pointer is stored in C.
`runtime.LockOSThread()` keeps SDL operations on one OS thread. The build script
links the host shared library through `CGO_LDFLAGS`.

[Rust](../examples/foreign_languages/rust/main.rs) uses the C ABI directly:

```rust
extern "C" fn changed(user: *mut c_void, count: i32) {
    unsafe { *(user as *mut i32) = count; }
    println!("Rust count: {count}");
}
```

The `i32` lives on the caller's stack throughout the synchronous host call.
The narrow unsafe boundary covers the FFI call and pointer conversion. A
production callback must avoid panicking across the C boundary.

[JavaScript](../examples/foreign_languages/node/main.cjs) uses a locally built
[N-API addon](../examples/foreign_languages/node/addon.c), with no node-gyp:

```js
const { run } = require('../build/cui.node');
let observed = 0;
const callback = count => { observed = count; console.log(`Node count: ${count}`); };
const status = run(callback); // Omit font for the headless test.
```

The addon invokes JS only synchronously on Node's calling thread, propagates
callback exceptions and never retains a JavaScript callback after return.

[TypeScript](../examples/foreign_languages/typescript/main.ts) declares the addon
shape, then uses the same N-API implementation:

```ts
interface Host {
    run(callback: (count: number) => void, font?: string, frames?: number): number;
}
const host = require('../build/cui.node') as Host;
const status = host.run(count => console.log(`TypeScript count: ${count}`));
```

Node strips erasable TypeScript syntax; this run does not perform static type
checking. Use your project's pinned TypeScript compiler for type checking if
integrating the sample into an application.

## Validation and limits

Validated on Linux with Go 1.27.2, Rust 1.99.0, Node 25.8.0, SDL3 3.4.18,
FreeType and HarfBuzz. The C host check verifies callback order, repeated independent runs and optional
callbacks, in both native-enabled and core-only builds. Headless clients
verify counts 1 then 2. Python additionally
checks exact callback order, exception identity and a subsequent independent run.
JavaScript also checks rejected arguments, a thrown callback and a subsequent
successful run.

Native Linux/X11 checks use Xvfb, xdotool and xwininfo to click the actual window button,
verify that the foreign-language callback prints `count: 1`, and save a BMP of
the final frame. They exercise a native window and SDL event dispatch, rather
than merely generating headless commands:

```sh
FONT=/usr/share/fonts/noto/NotoSans-Regular.ttf
EXAMPLES=examples/foreign_languages
# Use TMPDIR pointing to a writable directory if your /tmp quota is constrained.
timeout 15 xvfb-run -a "$EXAMPLES/native-check.sh" "$FONT" python python3 "$EXAMPLES/python/main.py"
timeout 15 xvfb-run -a "$EXAMPLES/native-check.sh" "$FONT" go "$EXAMPLES/build/go-demo"
timeout 15 xvfb-run -a "$EXAMPLES/native-check.sh" "$FONT" rust "$EXAMPLES/build/rust-demo"
timeout 15 xvfb-run -a "$EXAMPLES/native-check.sh" "$FONT" node node "$EXAMPLES/node/main.cjs"
timeout 15 xvfb-run -a "$EXAMPLES/native-check.sh" "$FONT" typescript node "$EXAMPLES/typescript/main.ts"
```

Each check writes `build/<language>-native.log` and `build/<language>-native.bmp`.
The fixed click coordinates belong to this sample's 480×240 layout. Xvfb tests
use a virtual Linux display; physical-display behavior, Wayland, macOS, Windows
and performance across language boundaries have not been validated here.
The examples do not implement a browser renderer or promise complete bindings,
IME support or native operating-system widgets. The button is a CUI control
rendered into an SDL desktop window.
