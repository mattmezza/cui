# Typed Slider delivery checkpoint

Design-only audit, 2026-11-01; `/root/hidden_windows`. Source baseline signed
`3d5ef56deb2b3bfc7d66acbb2c8f6c14367621e7`; root is integrating that facade
with its newer Core/Dialog work. Future production must start from the resulting
integrated source, not overwrite those changes. No production, generated files,
compiler, dependencies, merge or push at this checkpoint.

## Actual coverage and remaining boundaries

| Path | Existing authority | Remaining work |
| --- | --- | --- |
| C controlled UI | `include/cui/cui_base_slider_model.h`, `include/cui/cui_styled_slider_model.h`; `cui_styled_slider_model_events_v1`, owned options/presentation, Base handler/result | Solid Single/Range UI implemented; no native delivery/all-five binding implication. `examples/styled_slider_model_controlled.c` is a complete windowless callback/composition example. |
| Core numeric input | `include/cui/cui.h`: `cui_set_accessibility_numeric_actions`, `cui_queue_accessibility_set_value_v1`, `cui_claim_accessibility_numeric_input_v1`, completed `cui_accessibility_numeric_snapshot_ready`, admitted `cui_frame_input_snapshot_ready` | Typed queue/capabilities/claim exist. They do not deliver platform actions themselves or certify presentation/AT success. |
| Native accessibility | `include/cui/cui_accessibility.h`: old create/update/drain/request; `backends/accesskit/cui_accessibility.c`: `make_tree`, `action`, update and drain | `make_tree` advertises only Focus/Click; callback ignores other actions. Snapshot numeric values are legacy semantic floats. No numeric-enabled constructor or copied SET mailbox yet. |
| C bridge | `include/cui/cui_bridge.h`: `cui_bridge_set_value`, `cui_bridge_set_range`, `cui_bridge_poll_event_ex`; `src/bridge/cui_bridge.c` Slider paint branch | Existing scalar double storage is cast to float before `cui_slider`, then auto-accepted and emits CHANGE. Domain range is not selected Range. No typed pair/config/request/receipt API. |
| Python | `bindings/python/cui_bridge/__init__.py`: Widget.value/range, App dispatch | Legacy route only. No typed controlled request acceptance. |
| JS | `bindings/node/addon.c`, `bindings/node/index.cjs` | Legacy route only. Needs native record conversion plus actual UI authoring, not just exports. |
| TS | `bindings/node/index.d.cts`, `bindings/typescript/index.d.ts` | Must agree with JS runtime; legacy Slider interface is insufficient for tagged pairs. |
| Go | `bindings/go/cui/native.go` | Legacy route only; dynamic symbol compatibility and copied float64 pairs needed. |
| Rust | `bindings/rust/src/lib.rs` | Legacy route only; owned f64 pairs/receipts and matching generated static SDK needed. |
| Hosted gallery | `web/gallery.c` controls section calls `cui_slider` for Volume; Progress control independently calls legacy Slider | New facade has no hosted public interaction demonstration yet. Preserve existing option-off legacy routes and events. |
| Registered scroll | `src/core/cui_scroll_layout_internal.h`, `cui_core_sl_tree` rejects noncanonical percent-height descendants | New vertical presenter uses percentage-height spacers/anchors. Ordinary-flow vertical works; registered-scroll vertical is an explicit rejected context, not completion. Horizontal admission proof exists in `docs/design/STYLED_SLIDER_MODEL_V1.md`. |

Renderer boundary: visible Aurora gradient is rejected on V1 before reduction.
A copied V2 declaration is a host responsibility, not invented negotiation.
Current gallery/adapters must not be declared V2-gradient capable from the C
fixture. All78 catalog entries stay present; these gaps keep Slider partial.

## Recommended order and explicit proposed ownership

1. **Hosted controlled authoring first**: a gallery-only worker owns
   `web/gallery.c`, minimal gallery module/build list, a focused headless browser
   fixture and consumer example documentation. Add a clearly selectable typed
   solid Single/Range demo, with horizontal/vertical, reverse/log, disabled and
   rejected/accepted proposals. Keep owner pair separate from intention; show
   request phase/current/requested and acceptance through a real callback. Use
   the already implemented facade, no new input route or Core API. Default/off
   legacy Volume and Progress behavior stays unchanged. Parent reviews exact
   controls before production. WASM source list must include new facade and
   pure style/model dependencies explicitly; do not silently rely on native CMake.
   Headless proof: actual pointer/touch/keyboard, owner rejection then acceptance,
   two increments, Range bounds and distinct painted thumbs, resize both axes,
   focus/rings/software or browser pixels, disabled/no requests, reduced motion
   and settling idle wake. Copy panel contains complete compiling C callback,
   state init and compose code, not an undefined `application` fragment. No
   public V2 gradient support claim. This is the highest-impact next slice.
2. **C bridge typed records and receipts**: a separate bridge worker owns only
   new public bridge declarations, private node state/typed queue, bridge tests
   and source metadata. Reuse the reviewed full contract in
   `SLIDER_MODEL_ABI_V1.md` (including its b24 evolving-intent resolution).
   Before implementation, root reviews exact record layout, exported signatures,
   bounded queue allocation, app-epoch exhaustion and optional-symbol policy.
   New versioned config/value getter/setter, dedicated poll and explicit accept
   reach the actual Styled facade; old scalar records/CHANGE/poll remain exact.
3. **Five authoring bindings**: independently reviewable language slices follow
   the same C ABI, not five different state machines. Each owns its wrapper,
   focused runtime tests and an actual mounted Single/Range UI example with
   callback/explicit acceptance and next-frame paint. JS+TS ship together.
4. **Native numeric-enabled adapter**: separate adapter worker owns only
   `cui_accessibility.h`, AccessKit backend and private host tests/docs. Implement
   the reviewed versioned16-byte constructor options/ENABLE_NUMERIC policy in
   `SLIDER_ACCESSIBILITY_ACTIONS.md`; keep old constructor/config ABI and its
   Focus/Click snapshot/timing unchanged. This can proceed independently of
   binding work after its exact option/mailbox receipt review.
5. **Vertical registered-scroll admission**: Core-layout owner only, separate
   design/proof and root approval before production. Do not blanket-remove
   percent-height rejection. Prove fixed positive ancestor dependence is
   independent of Y scroll offset and converges under existing two-pass budget,
   including nested anchors, clip, responsive resize and failure atomicity.
   Alternative presenter geometry must preserve source offsets/current bounds
   without input replay or a hidden new dispatcher. No exception is approved yet.

Ownership above is proposed, not permission to edit another worker's files.
Integrator schedules one bounded compiler lease and canonical regeneration.

## Shared copied transport and acceptance contract

New bridge records are pointer-free exact-size/version/reserved validated copies:
complete tagged Single(end)/Range(start,end), double domain and gesture domain,
orientation/reverse/log/step, immutable entry current, evolving requested intent,
phase/reason/input evidence and receipt. Preserve all reviewed64-bit app epoch,
node generation, source/config generation, frame, sequence, target and event
identity fields; no conversion through float32, scalar CHANGE or JS Number.
Borrowed callbacks/userdata stay alive through registration removal/disposal;
request copies survive poll/frame/dispose but acceptance authority expires.

Callbacks run only in existing after-step UI-thread dispatch, never inside Core
reduction. They may mutate external application/userdata and enqueue work;
protected borrowed model/options/theme cannot be mutated during their submission.
No reentrant step/run. Exceptions/panics must be caught before crossing C; expose
them through the language's existing error path, never implicit acceptance.

Acceptance checks owning app/UI thread, live full node generation, config/source,
latest painted model frame and increasing accepted sequence. Next painted model
submission expires old receipts; layout-only does not renew them. Multiple
same-frame UPDATE then COMMIT remain independently acceptable in order. Explicit
accept validates the complete pair and changes future owner state only; never
retroactively paint or auto-accept COMMIT. Missing observer means read-only.
Bounded queue preflight precedes that node's reduction/consumption/callback;
there is no whole-frame rollback of callbacks already delivered. Invalid
poll/accept preserves outputs/queue/owner as specified in the ABI contract.

| Language | Owned mapping and compatibility proof |
| --- | --- |
| Python | Frozen tagged dataclasses, float doubles/int identities; optional symbols retain old-library loading; owned ctypes copies and real UI runtime test. |
| JS/TS | Frozen discriminated pair/request objects; bigint identities, number doubles; no JSON transport; positive/negative TS compile plus actual addon runtime/UI proof. |
| Go | uint32 tags/uint64 identities/float64 pairs; explicit error-returning accept; copy before cgo return, no callback during cgo stepping; optional new symbols retain old loads. |
| Rust | Owned enums/f64/u64 records and Result acceptance; no reentrant borrow across dispatch, no unwind across FFI; static matching SDK and dynamic compatibility policy reviewed separately. |

## Native proof and unresolved precision authority

New backend mode publishes only explicitly registered current capabilities after
valid completed Core snapshot; invalid readiness discards pending numeric intent,
while genuine Core queue capacity alone retries. Tokens remain backend generations,
not CUI IDs. Under mutex, foreign callbacks copy finite SET double into a bounded
owned mailbox and wake only; UI-thread drain queues typed input. Increment and
Decrement map only where the actual host supports them. Linux pinned AT-SPI uses
actual SetValue: prove SetCurrentValue over private DBus/Xvfb reaches an
ACCESSIBILITY request, then explicit owner acceptance changes next-frame paint.
A direct helper call is not native delivery. Disabled/readonly/omitted/modal,
role/token replacement, queue-full/retry, close and failed-layout/error-clear
recovery require tests. Unsupported SET in new mode is truthfully readonly;
old mode remains unchanged. No pinned dependency patch or fabricated keys.

Before promising exact domain-value AT queries, resolve that existing semantic
value/min/max storage is float while typed owner/domain are double. The reviewed
SET payload uses published semantic numeric units; adapter must not silently
claim exact double query parity from a cast. A normalized projection policy or
separate copied domain metadata requires an explicit root decision. This is a
precision/authority contract gap, not permission for public semantic struct growth.
Physical screen-reader acceptance, non-Linux hosts, real V2 gradient consumers
and all-five installed authoring remain independent completion gates.

## Validation and documentation when each slice lands

Keep frozen legacy float callbacks/autoaccept/precision/IDs/timing tests. New tests
must distinguish rejected intention, consumed input and accepted owner. Exercise
receipt wrong app/node/config/frame/sequence, disposal/reuse, queue bounds,
output atomicity, ordered events and external-owner mutation. First frame,
omission/reappearance, invalid frame+clear+fresh recovery and mode changes stay
required. Native automated work uses private Xvfb only; browser is headless.
No heavy build is needed for this audit. Future changes update usage guide,
component-usage/binding coverage, source lists/SDK/dist and skill canonical
resources; run usage6, skill18/freshness and meaningful runtime proofs without
marking the full family closed from a subset.
