# Native callback runners in language bindings

Python `App.run()`, Go `App.Run(dt)`, Rust `App::run()` / `run_with(...)` and
Node/TypeScript `App.run()` delegate to `cui_bridge_run`. The SDL callback loop
owns event pumping and frame scheduling so native modal resizing can continue
layout and paint. Call from the app's creating UI thread. Mount a tree first.
Python/Go retain and validate the legacy positive finite `dt` argument for source
compatibility; the native host supplies actual frame timing.

Application handlers execute after a completed native frame. They may update
nodes or request close. Nested run and step calls are rejected. Python and Node
retain the original exception, Go recovers a panic, and Rust uses `catch_unwind`;
all stop the C runner before rethrowing/resuming in the language caller. Rust
builds using `panic=abort` retain the toolchain's abort behavior. No language may
unwind through a C callback.

A normal window close returns normally. After the runner exits, its native host
is released and the app is terminal: release the app owner as usual. Python
`close()`, Node `dispose()` and Go `Close()` may also destroy the owner inside a
handler; the bridge defers actual destruction until the runner exits. These
wrappers invalidate their handles immediately and avoid using them afterward.
Rust ownership prevents dropping a mutably borrowed app while `run` is active;
use `run_with(|app| app.close())` for a safe close request/update hook.

Manual APIs remain: Python `step`/`dispatch`, Go `Step`/`Dispatch`, Rust
`step`/`dispatch`/`frame`, and Node `step`. Node `start`/`stop` still use timers and
keep the JavaScript event loop responsive. The synchronous native `run()` blocks
Node timers/I/O until closure; choose it when native modal-resize behavior is
needed. It cannot be combined with `start()` on the same app.

Dynamic wrappers resolve the new optional runner lazily. Older bridges can still
use manual APIs; runner use fails clearly when unavailable. Portable/headless
backends do not support the native runner. Rust's existing dynamic packaging
support is Linux; static Go/Rust builds require refreshed canonical SDK sources
containing the runner. No generated SDK snapshot is changed by this wrapper patch.

Validation lives in Python `test_run_native_virtual.py`, Node
`test/run-native.test.cjs`, Go `TestNativeRunLifecycle`, and Rust
`tests/run_native.rs`. Native tests require the isolated-display marker and font
path and must be launched through `scripts/test_local.py`; never point them at
the desktop display. They cover callback delivery, normal close, foreign failure
containment, terminal lifecycle and reentrancy; Python additionally exercises a
real X11 resize through the native runner. Underlying SDL live-resize behavior
and C deferred destruction are separately covered by native bridge/runner tests.
