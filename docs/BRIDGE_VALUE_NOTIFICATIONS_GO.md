# Go owning value notifications

`(*App).PollValueNotificationV1() (ValueNotificationV1, bool, error)` is an additive,
manual owning view over the existing FIFO. `PollNotificationV1`, its struct and enum,
legacy `PollEvent`, `Dispatch`, `Step` and `Run` keep their contracts. Nothing here
registers callbacks, broadcasts events or makes foreign synchronous requests.

```go
field := cui.Input("name")
app, err := cui.New(cui.Options{})
if err != nil { panic(err) }
defer app.Close()
if err := app.Mount(cui.Column("root").Children(field)); err != nil { panic(err) }
if err := app.Step(1.0 / 60); err != nil { panic(err) }
for {
    event, ok, err := app.PollValueNotificationV1()
    if err != nil { panic(err) }
    if !ok { break }
    switch event.Kind {
    case cui.ValueNotificationScalarChangeV1:
        fmt.Println(event.Scalar.Value)
    case cui.ValueNotificationTextChangeV1:
        fmt.Println(event.Text.Text)
    case cui.ValueNotificationSelectionChangeV1:
        fmt.Println(event.Selection.Handle)
    }
}
```

The example assumes imports `fmt` and `github.com/mattmezza/cui/bindings/go/cui`.
Call `Step` after host input as appropriate; it does not automatically dispatch Go
handlers. Do not also drain through `Dispatch`/`Run` when manual ownership is intended.
All consumers divide the same queue rather than receiving independent copies.

`ValueNotificationKindV1` is a separate enum, with activation, boolean, table,
surface-close and legacy alternatives plus scalar/text/selection. Exactly the
kind-selected payload pointer is nonnil; activation/legacy have no typed pointer.
The new payloads are `ScalarChangeV1{Value float64}`, `TextChangeV1{Text string}` and
`SelectionChangeV1{Handle Handle, Node *Node}`. The common fields mirror the existing
view, including complete owning `LegacySnapshotV1`. Existing enum values/types do
not acquire extra alternatives.

Frames remain exact `uint64`, handles/indices exact `uint32`, scalar values binary64.
NumberInput retains its double precision; Slider/Rating cannot recover precision
already lost in float models. See [the C allowlist](BRIDGE_VALUE_NOTIFICATIONS.md)
for the four scalar, four plain-text and five selection producers. Combobox/menu
and unsupported producers remain LEGACY, with no JSON parsing or invented evidence.
The bridge has no secure-text configuration or inferred redaction. This API is not
a forwarding layer for all direct-C event facades.

Values, strings and identities are copied and survive later model mutation/removal
and app disposal. Go records are caller-mutable, not frozen objects. Payload pointers
point to owned Go allocations, not C storage. Optional `Node` references are live
objects resolved by exact slot **and generation**, not immutable model snapshots.
A removed historical selection retains its identity but resolves to nil.

Both dynamic symbol lookup and the portable static weak-symbol path independently
resolve size/version/category/poll capabilities. Only advertised known category
bits are requested; unknown capability bits are ignored. Missing optional symbols,
wrong layout/version or no known category fails before polling, without fallback
to a legacy drain. Older libraries remain usable through existing polling APIs.

The existing allocation policy starts at 4097 bytes, permits one strictly growing
retry and caps capacity at 1 MiB including NUL. `ErrNotificationPayloadLimit` and
inconsistent retries leave the live record queued. Successful native polling
consumes before decoding; malformed kind/tag/metadata/UTF-8/payload errors cannot
roll the record back. Decode checks include genuine frame presence, legacy identity
consistency, finite scalar values, full selection identity and negotiated tags.
No source/input-index/activation metadata is fabricated.

## Evidence and reproduction

`bindings/go/cui/value_notifications_v1_test.go` exercises real queued NumberInput,
Input and RadioGroup events in both static and dynamic builds, binary64 precision,
old/new FIFO interleaving, setter/disabled silence, copied UTF-8 and actual removal of a historical selection.
Go's public mounted-tree API cannot append/recreate a replacement in this fixture.
A separate **synthetic registry entry** checks same-slot/different-generation lookup;
this is not Go allocator-reuse proof. Actual C allocator reuse is covered separately
by C/Python/JS tests. An actual older initial-V1 library proves unavailable value
polling leaves queued text available to the old poll.

Build test executables offline, without regenerating SDK copies:

```sh
cd bindings/go
GOTOOLCHAIN=local GOPROXY=off go test -tags cui_dynamic -c ./cui -o "$build_dir/go-dynamic.test"
GOTOOLCHAIN=local GOPROXY=off go test -c ./cui -o "$build_dir/go-static.test"
```

Use `CGO_CFLAGS=-I/absolute/canonical/include` when testing against a newer canonical
header before SDK regeneration. Register executables in an external CTest harness.
Set `CUI_VALUE_REAL=1` for real producers; dynamic mode also needs an absolute
`CUI_BRIDGE_LIBRARY`. For an actual older library set `CUI_VALUE_OLD=1` and run only
`-test.run=^TestValueNotificationV1TextFIFO$`. Static portable checks exclude
`-test.skip=^TestNativeRunLifecycle$`; SDL runner tests need their native build/font
and are outside this portable slice. Existing V1 and other portable Go tests remain
included. Run CTest only through `python scripts/test_local.py --build-dir "$build_dir"`.

The isolated Linux provider in `cui/testdata/value_notifications_provider.c` is
synthetic protocol evidence, not input evidence. Compile it with strict C11 warnings,
`-fPIC -shared`, canonical `-Iinclude`, and `-Wl,--no-as-needed -L"$old_bridge_dir"
-lcui_bridge -Wl,-rpath,"$old_bridge_dir"`. It depends on the actual older bridge for
normal App functions; do not substitute it for production. The no-known-category and missing-query modes also retain actual queued text for the old poll. A second build with
`-DCUI_VALUE_FIXTURE_MISSING` omits the category query. Point dynamic library selection
to the provider and run only `TestValueNotificationV1Protocol`, fresh process per
`CUI_VALUE_FIXTURE` mode: `edges`, `capacity`, `size`, `unavailable`, `missing`, `limit`,
`zero`, `repeat`, `tag`, `kind`, `utf8`, `nul`, `flags`. These check exact uint64 extremes,
unknown capability filtering, optional exports, malformed output and bounded copies.

Rust value wrappers and generated SDK publication remain separate integration work.

The repository integrator runner also stages current SDK sources and registers
static, dynamic, legacy, protocol and actual older-library cases offline:

```sh
python scripts/test_notification_bindings.py --build-dir /absolute/validation-dir \
  --library /absolute/current/libcui_bridge.so \
  --value-older-library /absolute/initial-v1/libcui_bridge.so
```

The older value library must expose initial V1 but lack the new value symbols.
Without it, missing-export/older-library no-drain cases are explicitly deferred.
The independent `--older-library` option tests a genuinely pre-V1 library instead.
