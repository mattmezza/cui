# Rust owning value notifications

`App::poll_value_notification_v1(&mut self) -> Result<Option<ValueNotificationV1>>`
is a separate opt-in owning view over the existing queue. The original
`NotificationV1` enum and `poll_notification_v1` method remain unchanged, including
exhaustive matches. This adds no listeners, foreign synchronous requests or dispatch
hooks. Polling and `dispatch`/`run` divide the same FIFO; they do not broadcast copies.

```rust
use cui_builders::{App, Kind, ValueNotificationV1};
fn main() -> Result<(), cui_builders::Error> {
let mut app = App::headless(640., 480.)?;
let root = app.root(Kind::Column, "root")?;
app.child(root, Kind::Input, "name")?;
app.step(1. / 60.)?;
while let Some(event) = app.poll_value_notification_v1()? {
    match event {
        ValueNotificationV1::ScalarChange(_, value) => println!("{value}"),
        ValueNotificationV1::TextChange(_, text) => println!("{text}"),
        ValueNotificationV1::SelectionChange(_, selected) => println!("{:?}", selected.handle),
        _ => {}
    }
}
Ok(())
}
```

Call `step` after host input as needed.
`step` does not dispatch Rust callbacks. Choose manual draining or the existing
callback dispatch loop deliberately.

The separate enum retains activation, boolean, table, surface-close and legacy
alternatives, and adds `ScalarChange(MetadataV1, f64)`,
`TextChange(MetadataV1, String)` and
`SelectionChange(MetadataV1, SelectionChangeV1)`. The latter contains
`handle: RawHandle` and `node: Option<Node>`. Full legacy snapshots remain available
in common metadata. Strings/numeric values/identities are owned Rust data and survive
later mutation, removal, queue reuse and app drop. Rust callers can mutate their own
returned values; this is not an immutable model snapshot.

Frames are exact `u64`, slots/generations/indices exact `u32`, scalar values binary64.
NumberInput keeps its double precision; Slider/Rating cannot recover precision
already lost in float models. Optional Node capabilities are checked against the
current full C slot/generation using a read-only getter before being returned; a
removed identity does not resolve to a replacement generation. Node values remain
live capabilities, not retained C objects; later operations still validate them.

See [the C allowlist](BRIDGE_VALUE_NOTIFICATIONS.md) for the four scalar, four plain
text and five selection producers. Combobox/menu and unsupported producers remain
LEGACY. Nothing parses their JSON or invents input source, activation or index
metadata. No bridge secure-text configuration or inferred redaction is provided.
This does not establish uniform direct-C component-event-facade coverage.

Dynamic builds resolve optional size/version/category/poll exports, verify layout,
and request only advertised known category bits. Unknown capability bits are ignored.
Missing exports, unsupported version or no known categories return `Error(9)` before
polling, without fallback to another drain. A layout mismatch returns
`NOTIFICATION_PROTOCOL_ERROR`. Older dynamic libraries remain usable through their
existing APIs. **Static builds link strong symbols from the paired bundled SDK**, as
the existing Rust V1 implementation does; rebuild the crate with its matching bundled
bridge. Compatibility with mismatched older static archives is not promised. Rust
stable provides no portable weak-symbol mechanism here.

The shared allocation policy starts at 4097 bytes and permits one strictly growing
retry capped at 1 MiB including NUL. `NOTIFICATION_PAYLOAD_LIMIT` and inconsistent
capacity responses leave a live record queued. Successful native polling consumes
before decode; malformed output cannot be rolled back. Validation covers schema,
flags, kind/tag, legacy/frame/identity consistency, negotiated tags, finite scalar
values, selected identities, UTF-8 and NUL termination. Invalid UTF-8 returns `Error(7)`;
other malformed output returns `NOTIFICATION_PROTOCOL_ERROR`.

## Evidence and reproduction

`tests/value_notifications_v1.rs` uses actual NumberInput/Input/RadioGroup input in
both static and dynamic builds: precision, setter/disabled silence, old/new FIFO, existing Checkbox/DataTable typed alternatives,
owned copies after app drop, and actual selection removal plus same-slot/new-generation
replacement through Rust's public `child` API. Old and new selected identities remain
distinct and the historical event has no live Node. The old enum's exhaustive-match
fixture compiles unchanged.

Build offline using existing pinned dependencies (this crate adds none):

```sh
CUI_BRIDGE_LIB_DIR="$bridge_dir" CARGO_TARGET_DIR="$dynamic_target" \
  cargo test --offline --manifest-path bindings/rust/Cargo.toml --no-run
CARGO_TARGET_DIR="$static_target" \
  cargo test --offline --manifest-path bindings/rust/Cargo.toml --no-run
```

Register the resulting unit, builders, callback-panic, notification and value test
executables in an external CTest harness, with `CUI_VALUE_REAL=1` and
`--test-threads=1`. Serialized tests respect the process-global layout runtime.
Exclude `run_native` explicitly from these portable checks; SDL runner proof requires
its separate native build and font. Execute native CTest only through
`python scripts/test_local.py --build-dir "$build_dir"` (private Xvfb).

For older-library proof, run the dynamic value executable with an actual initial-V1
library selected through `LD_LIBRARY_PATH`, `CUI_VALUE_OLD=1`, and test filter
`real_text_fifo_or_older_library_no_drain`. It queues real UTF-8 input, checks value
polling is unavailable, then retrieves the unchanged text with old polling.

`tests/fixtures/value_notifications_provider.c` is separate Linux **synthetic protocol
proof**, not UI input evidence. Compile it with strict C11 warnings, `-fPIC -shared`,
canonical `-Iinclude`, and link normal App exports from an actual older bridge with
`-Wl,--no-as-needed -L"$old_bridge_dir" -lcui_bridge -Wl,-rpath,"$old_bridge_dir"`.
Compile another with `-DCUI_VALUE_FIXTURE_MISSING` to omit its category query.
Use `LD_PRELOAD` for the provider and `LD_LIBRARY_PATH` for the old bridge; run only
`synthetic_protocol` in a fresh dynamic test process per `CUI_VALUE_FIXTURE` mode:
`edges`, `capacity`, `size`, `unavailable`, `missing`, `limit`, `zero`, `repeat`, `tag`,
`kind`, `utf8`, `nul`, `flags`. This covers uint64 extremes, unknown-bit filtering,
partial exports, malformed output and bounded 8192-byte copies. The no-known-category
and missing-query modes also run the real text test to verify no drain.

Generated SDK/package publication is separate integrator work; no SDK copies change
in this wrapper slice.
