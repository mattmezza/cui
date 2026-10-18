# JavaScript and TypeScript owning value notifications

`App.poll_value_notification_v1(): ValueNotificationV1 | undefined` is an explicit,
manual drain of the **same queue** used by `poll_notification_v1()`, legacy dispatch,
`step()` and `run()`. It adds no listeners or synchronous requests. Existing V1
API/types and automatic dispatch stay unchanged. Use `step_only()` when the caller
owns draining; mixing consumers divides events rather than broadcasting them.

```js
const c = require('@cui/native');
const app = new c.App();
try {
  app.mount(c.column().children(c.input('name', '')));
  app.step_only(); // call after each host input/frame as appropriate
  let event;
  while ((event = app.poll_value_notification_v1()) !== undefined) {
    if (event.kind === 'text-change') console.log(event.text);
    if (event.kind === 'scalar-change') console.log(event.value);
    if (event.kind === 'selection-change') console.log(event.selectedHandle);
  }
} finally { app.dispose(); }
```

The new union extends the existing notification alternatives with:

| Kind | Owning payload |
| --- | --- |
| `scalar-change` | `value: number`, the captured C binary64 value |
| `text-change` | `text: string`, copied UTF-8 |
| `selection-change` | frozen `selectedHandle: {slot, generation}`, optional `selectedNode` |

See [the C allowlist](BRIDGE_VALUE_NOTIFICATIONS.md) for the exact four scalar,
four plain text and five selection producers. Combobox, menu and other unsupported
producers retain complete `legacy` snapshots; no JSON is parsed. NumberInput retains
its double precision; Slider/Rating cannot recover precision already lost in their
float models. There is no secure-text configuration or inferred redaction in this
bridge API. This is not uniform component-event-facade coverage.

Records, legacy snapshots and handle objects are frozen. Strings/numeric values and
handles survive later mutation, queue reuse and app disposal. `node`/`selectedNode`
are optional live JS object references resolved by the complete slot/generation;
these references are not immutable model snapshots. Removed historical selections
retain their copied handles without resolving a replacement generation. Handles and
indices are exact uint32 Numbers; every frame is an exact uint64 **BigInt**, including
values above 2^53. JSON serialization requires an explicit BigInt conversion policy.

The production addon resolves the new C exports optionally, verifies V1 size/version,
and requests only advertised known bits SCALAR/TEXT/SELECTION. Unknown capability bits
are ignored. Missing addon method, missing native exports, or no known categories
throw without falling back to another drain. Older native libraries remain usable
through existing APIs. A missing/mismatched capability is checked before polling.

Storage starts at 4097 bytes, with one strictly growing retry capped at 1 MiB including
NUL. Invalid/non-growing/repeated/over-limit capacity responses throw; capacity failure
does not consume the live record. A successful native poll consumes before validation:
malformed tags/kinds, frame/legacy mismatches, flags, scalar values, selection handles,
UTF-8 or termination cause a protocol error and cannot roll that record back. No
unavailable source, activation or input-index evidence is fabricated.

## Validation and reproduction

`bindings/node/test/value-notifications-v1.test.cjs` uses actual queued input for
NumberInput, Input and RadioGroup, old/new FIFO, setter silence, payload copies and
removed/reused selection generations. Run it again with `CUI_EXPECT_OLD=1` and an
addon linked to an actual initial-V1 library to prove unavailable value polling does
not drain its queued text. Legacy notification/binding/component tests also pass.

The separate Linux interposition fixture is **synthetic protocol evidence**, not UI
input. It checks unknown capability filtering, partial exports, layout failure,
malformed kind/tag/flags/UTF-8/NUL, retry limits, an 8192-byte copy and uint64 extremes.
Compile the production addon and fixture without modifying repository build wiring:

```sh
# Set bridge_dir to the directory containing the desired libcui_bridge.so.
cc -std=c11 -Wall -Wextra -Wpedantic -Werror -fPIC -shared \
  -Iinclude -I/usr/include/node bindings/node/addon.c \
  -L"$bridge_dir" -lcui_bridge -Wl,-rpath,"$bridge_dir" -o "$build_dir/cui.node"
cc -std=c11 -Wall -Wextra -Wpedantic -Werror -fPIC -shared -Iinclude \
  bindings/node/test/value-notifications-provider.c -o "$build_dir/provider.so"
# Compile again with -DCUI_VALUE_FIXTURE_MISSING for the partial-export case.
```

Register Node invocations in an external CTest harness with `CUI_NODE_ADDON` selecting
the addon. For each protocol mode set `LD_PRELOAD` to the fixture and
`CUI_VALUE_FIXTURE` to `edges`, `capacity`, `size`, `unavailable`, `missing`, `limit`,
`zero`, `repeat`, `tag`, `kind`, `utf8`, `nul` or `flags`; run
`value-notifications-protocol.test.cjs` in a fresh process. The `missing` fixture must
use the missing-export build and an actual older bridge library, so the real library
cannot supply the omitted symbol. Run native CTest only through
`python scripts/test_local.py --build-dir "$build_dir"` (private Xvfb).

Both old and new type fixtures compile with pinned TypeScript 5.8.3:

```sh
npm exec --yes --package typescript@5.8.3 -- tsc --noEmit --strict \
  --target ES2022 --module NodeNext --moduleResolution NodeNext \
  bindings/node/test/notifications-v1.types.ts \
  bindings/node/test/value-notifications-v1.types.ts
```

Go/Rust value wrappers and generated package/SDK publication are separate integration
work. Generated `index.d.cts` continues to re-export the canonical TypeScript file.
