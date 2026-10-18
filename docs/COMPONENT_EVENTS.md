# Shared component event metadata

The additive `cui_component_events.h` utility supplies common metadata and a bounded text-copy helper. It does **not** change any existing component callback, result, state owner or dispatch timing. The [78-family coverage](design/COMPONENT_EVENTS_COVERAGE.md), [binding audit](design/COMPONENT_EVENTS_BINDINGS.md) and [design contract](design/COMPONENT_EVENTS_CONTRACT.md) describe the wider work. The first [Button and Check facades](CONTROL_EVENTS.md) are implemented; [Switch/Toggle facades](BOOLEAN_EVENTS.md) are also implemented; other family facades remain future slices. This utility is not full component parity.

## Names and meanings

`cui_component_event_metadata` is a pointer-free value containing a kind, nonzero stable target ID, presence flags, optional context-local frame/index, source, and an optional copied `cui_activation_snapshot`. Component values and reasons stay in separate typed payloads: a requested boolean, tri-state, date, double or selection is not a `void *value`.

| Kind | Meaning supplied by the component that uses it |
| --- | --- |
| `CUI_COMPONENT_EVENT_ACTIVATE` | Accepted attempt to invoke an action; no implied value change. |
| `CUI_COMPONENT_EVENT_REQUEST` | Proposed typed value; the owner may reject it. |
| `CUI_COMPONENT_EVENT_CHANGE` | A transition that actually applied. |
| `CUI_COMPONENT_EVENT_COMMIT` | Gesture/transaction completion, possibly with no net change. |
| `CUI_COMPONENT_EVENT_SUBMIT` | Explicit submission, distinct from typing or blur. |
| `CUI_COMPONENT_EVENT_LIFECYCLE` | Component-specific open/close/begin/end reason, kept separately. |

`cui_component_event_init(&metadata, kind, target, source)` validates the enums and target and clears unavailable fields. `SOURCE_UNKNOWN` omits `HAS_SOURCE`; `SOURCE_PROGRAMMATIC` explicitly records a known application origin without inventing an input event. Pointer, keyboard and accessibility sources may also be explicitly supplied when known. Source alone does not imply activation evidence. An all-zero struct is not a valid target-bearing event.

Presence flags are authoritative. `HAS_FRAME`, `HAS_INDEX`, `HAS_SOURCE` and `HAS_ACTIVATION` describe optional information; index zero is a valid first event, not an absence sentinel. `cui_component_event_set_frame_index(&metadata, frame_sequence, event_index)` records a known pair and rejects frame zero, index `SIZE_MAX`, or replacement of copied activation identity. The utility does not infer a position for programmatic work. Frame/index identities are local to the originating context and cannot be compared across recreated contexts. No timestamp, global ordering, physical device or pointer position is manufactured.

## Copy accepted evidence before mutable callbacks

This complete helper can be used by a component adapter at its existing accepted-activation point. Calling it does not route, consume, accept or redispatch an input event:

```c
#include <cui/cui_component_events.h>

static bool activation_metadata(cui_context *ui, size_t event_index,
                                cui_component_event_metadata *out) {
    cui_activation_snapshot snapshot;
    if (!cui_event_activation_snapshot(ui, event_index, &snapshot))
        return false;
    return cui_component_event_from_activation(out, CUI_COMPONENT_EVENT_ACTIVATE,
                                                &snapshot);
}
```

Use a real snapshot or its unchanged copy. `from_activation` checks nonzero identity and supported accepted event types, derives the source, and copies existing evidence; it cannot authenticate a fabricated struct. Pointer release, key down/up and accessibility activation are supported. The utility preserves the snapshot exactly, including optional press/release/key-bounds flags. Key and accessibility events gain no fake pointer sample. Snapshot acquisition must precede mutable callbacks that could consume input. Failed helpers leave outputs unchanged.

The resulting metadata may be copied by value and kept after callback, later frames or context destruction. This guarantee covers this pointer-free metadata and Core's snapshot, not arbitrary typed component payloads. In particular, copying `cui_event` does **not** deep-copy its borrowed text pointer.

## Copy borrowed text into caller storage

`cui_component_event_copy_utf8(source, maximum_bytes, storage, capacity, &copied)` returns a `cui_component_event_copy_status`. Argument order deliberately places the source and payload byte limit before destination storage and its capacity. The source length and maximum are UTF-8 **bytes**, not glyphs. Capacity includes the trailing NUL; the returned `cui_str.length` excludes it.

```c
static cui_component_event_copy_status keep_text(cui_str borrowed,
                                                 char storage[257], cui_str *saved) {
    return cui_component_event_copy_utf8(borrowed, 256, storage, 257, saved);
}
```

The caller owns both storage and descriptor. Keep storage alive for every use of the copied view. The helper allocates nothing, truncates nothing and never retains a handler or pointer. It checks the maximum and capacity before reading bytes, reuses Base's existing UTF-8 validation, rejects embedded NUL, and uses `memmove` so source and destination text may overlap. The output descriptor itself must be disjoint from both text regions. `{NULL,0}` is valid empty input but still requires one writable destination byte for NUL. Callers must provide the declared readable/writable extents; no C function can discover arbitrary allocation sizes from a pointer.

`COPY_INVALID_ARGUMENT`, `COPY_LIMIT`, `COPY_CAPACITY` and `COPY_INVALID_UTF8` leave **both destination bytes and output descriptor unchanged**. A source length of `SIZE_MAX` fails as a limit error before adding NUL or reading bytes. No partial output or required-size side channel is published on failure. Caller-selected limits make the memory/time budget explicit; this utility does not choose a hidden global payload limit.

## Integration and lifetime boundary

These are portable C11 header-only helpers. Include `cui/cui_component_events.h`; the text-copy helper calls existing `cui_edit_valid_utf8` and therefore requires the existing portable `cui` library. Metadata-only use needs no extra runtime or allocation. No platform/Clay types, callback registration, scheduler or networking are introduced.

Direct C components still retain their existing behavior: Button legacy callback precedes its typed callback; controlled requests are distinct from owner acceptance; frame-result reads do not replay callbacks. Speech explicit pump, bridge queued language delivery, retained Surface/Notification/Dock registrations and current-frame Plot/Chart deferred callbacks keep their own documented timing and lifetimes. This utility neither permits recursive frame/component submission nor context destruction during callbacks.

No binding event facade or automatic callback export is supplied by this slice. Future facades and bindings require their own typed-value, lifetime, multiplicity, reentrancy and compatibility tests. Applications must choose one side-effect path per action rather than performing it from both callback and frame result.

The focused `tests/unit/test_component_events.c` exercises actual accepted pointer, keyboard and accessibility snapshots, evidence retention after consumption/context destruction, absent/programmatic metadata, identity overwrite rejection, atomic UTF-8 failure, exact byte/capacity limits, empty text, malformed encodings, overflow-sized input and overlapping copies. Native test runs use the invisible wrapper:

```sh
python scripts/test_local.py --build-dir build -- -R component_events
```

The helper is registered in native and WASI tests and included in generated distributions, the Go/Rust source SDKs and the agent resource index. Inclusion of a C header in a source SDK does not expose a language wrapper. Binding event facades remain documented omissions.

## Nonblocking application work

The separate [async HTTPS fetch example](../examples/async_fetch/README.md) builds an actual native UI using the existing synchronous Button callback. It copies requests to a worker, shows loading, prevents duplicate requests, delivers completions through a bounded mailbox and wakes the idle UI. Its guide covers failure, retry, stale responses, cancellation and joined shutdown, with deterministic offline tests. Networking and worker code belong to the optional Linux host example; they are not Core APIs or a replacement event-dispatch architecture.

The bridge separately supports [versioned typed manual notification polling](BRIDGE_NOTIFICATIONS.md). These queued post-mutation views preserve legacy dispatch timing and do not provide synchronous value rejection through language bindings.

The controlled [Slider phase facade](SLIDER_EVENTS.md) preserves double values and distinguishes BEGIN/UPDATE/COMMIT/CANCEL, including cancellation without fabricated input evidence. It does not add a Styled or foreign-language request facade.

The [Base Input observer](TEXT_INPUT_EVENTS.md) and [Styled Input facade](TEXT_INPUT_STYLED_EVENTS.md) report committed text changes and eligible submissions synchronously. It does not add rejectable edit requests or multiline observers. [Go and Rust manual notification views](BRIDGE_NOTIFICATIONS_GO_RUST.md) preserve typed queue values with owning payloads; their callbacks retain existing timing.
