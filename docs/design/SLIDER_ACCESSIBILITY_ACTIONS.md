# Slider numeric accessibility actions: minimal queue extension

Design-only addendum to [the complete Slider proposal](SLIDER_RANGE_ORIENTATION_SCALE.md),
source checkpoint `502f1e22`. No implementation, dependency/build, generated or
public-ABI changes in this commit. Existing FOCUS/ACTIVATE behavior stays intact.
Numeric action delivery remains a whole-family blocker until implemented/tested.

## Audited actual route and hazards

| Authority | Current behavior / required boundary |
| --- | --- |
| [`cui.h`](../../include/cui/cui.h):113,654 | Only FOCUS=0/ACTIVATE=1. `cui_event` already has targeted action enum+ID, so no event/semantic/details/config struct growth is needed. |
| [`cui.c`](../../src/core/cui.c):1270–1310 | Supported query uses completed `previous` semantics and previous modal eligibility, returns false during frame. Queue validates support, then normalizes to action type/target/action only, zeroing unrelated input. Capacity checked before enqueue. |
| Core:1502–1520 | Begin-frame revalidates against previous semantics and routes to explicit target. **Every non-FOCUS action currently becomes activation**; must change to explicit ACTIVATE branch before numeric actions exist. |
| Core:3187–3240 | `cui_interact` handles disabled focus/capture and validates activation role/current scope. Numeric requests must not enter this activation path at all. |
| Core:4190–4342 | End-frame removes inert/layout-only semantics, withdraws actions after current omission/role/scope/eligibility changes, copies current to previous. Its FOCUS/else-ACTIVATE eligibility branch also needs explicit numeric branch. |
| Core:4517–4640 | `cui_event_target` is a raw routed-target query, not current capability validation. `cui_consume_event` marks consumed; activation cancellation has no slot for a non-activation action. A new validated claim boundary is necessary. |
| Core:4979,5068 | Details can change after interaction; dismissed scopes compact semantic arrays. Capability sidecars must follow every compaction, not merely append/reset/end copy. |
| [`cui_component_events.h`](../../include/cui/cui_component_events.h) | ACCESSIBILITY source already exists. Activation helper sets HAS_ACTIVATION; **wrong helper for increment/decrement** because they create no activation. |
| [`cui_accessibility.c`](../../backends/accesskit/cui_accessibility.c):237–251,466,518–562 | Snapshot action mask, native action mapping, mailbox validation and drain currently only Focus/Click. Native callback queues locked tokens, never calls Core; UI-thread drain revalidates. |
| [`test_core.c`](../../tests/unit/test_core.c):3119 onward; [`test_accessibility.c`](../../tests/unit/test_accessibility.c) | Existing targeted Focus/Activate ABI values, queue/routing/disabled/role/omission/overflow/lifetime coverage remains required and unchanged. |

Pinned AccessKit C0.23.0 commit `0824c4a1e3a4d13ce5582df20e394fba49485a15`
([pin](../../third_party/accesskit/pin.json), archive SHA256
`19c3ecda791acb100b43442adb8738e245dd14ad641de5fa2b494ac52a9f11a9`).
The retained pinned header `include/accesskit.h`:84/88 contains
`ACCESSKIT_ACTION_DECREMENT` / `ACCESSKIT_ACTION_INCREMENT`; no dependency update
is needed. Source URL:
https://raw.githubusercontent.com/AccessKit/accesskit-c/0824c4a1e3a4d13ce5582df20e394fba49485a15/include/accesskit.h .
The CUI adapter currently does NOT advertise/map them. Other CUI native hosts
have no independent numeric accessibility-action implementation found here;
SDL helper supplies window metadata only. WASM gallery DOM controls are not a
semantic bridge for canvas-painted Sliders. Do not imply browser/other-OS AT parity.

## Minimal additive portable API, no public struct growth

Append explicitly numbered enum values INCREMENT=2, DECREMENT=3 to the existing
`cui_accessibility_action`; preserve existing0/1 and all existing event-type values.
Add numeric-only capability constants and functions (spelling subject to review):

```c
#define CUI_ACCESSIBILITY_NUMERIC_INCREMENT (1u << 0)
#define CUI_ACCESSIBILITY_NUMERIC_DECREMENT (1u << 1)
bool cui_set_accessibility_numeric_actions(cui_context *, cui_id, uint32_t mask);
bool cui_claim_accessibility_numeric_action(cui_context *, cui_id, size_t event_index,
                                             cui_accessibility_action *out);
```

Registration is explicit per **current semantic**. No automatic role-based
advertisement for legacy Slider, controlled scalar reducer, Progress, Scrollbar,
NumberInput, or arbitrary callers that do not consume these events. Initially
allow only role Slider; future supported roles require separate implementation
and tests. Mask0 revokes; only two declared bits accepted. Existing Focus/Activate
capability policy remains derived exactly as today and is not set through this API.

Registration requires active mutable painted submission, nonzero existing unique
current semantic ID, supported role and valid mask. It changes only that semantic's
private mask. Invalid role/ID/mask/lifecycle leaves mask unchanged and reports the
normal applicable error; inert/layout-only submission returns false with no
side effects. Disabled/readonly semantics may retain authored registration, but
supported/claim/publish eligibility always gates them off using actual details.
New component sets details first, then registers, then claims input. A legacy
component never calls registration and therefore remains unsupported.

Add two private uint32 arrays indexed alongside current/previous semantic arrays,
each bounded by existing `config.max_elements`. Allocate at create with checked
size arithmetic; fail create and clean up on allocation failure. No per-frame
allocation or new public capacity knob. Every new semantic starts mask0, even
when same ID/role appeared before; beginning a frame does not inherit previous
capabilities. Move masks in visible semantic compaction and dismissed-scope
compaction; copy to previous only with final completed semantics. Clear/recycle
slots deterministically so capacity reuse cannot grant an unrelated ID actions.
At default bounds this costs8 bytes per semantic slot, not an unbounded registry.

## Eligibility, delivery and independent acceptance

1. **Completed query/enqueue:** preserve existing query timing (outside frame
   only). Numeric support requires previous same-ID Slider registration bit,
   enabled, not readonly, and allowed previous modal scope. Unknown enums false.
   Enqueue normalizes to the existing target/action event only; lifecycle,
   unsupported and capacity failure leave queue/interaction state unchanged under
   existing sticky-error conventions. Generic `cui_queue_event` follows same rule.
2. **Begin routing:** recheck previous snapshot before routing. Numeric actions set
   target only; no focus change, focus-visible change, capture/release transition,
   activation slot/snapshot, pointer/keyboard event or implicit key repeat.
   FOCUS and ACTIVATE execute their old explicit branches unchanged.
3. **Current claim:** validate output and index/type/action, unconsumed event,
   exact routed target, previous AND current same-ID/same-role Slider,
   both capability bits, current enabled/not-readonly/modal eligibility, mutable
   painted phase. Only then copy action and consume the original input once.
   Failure leaves output and consumption unchanged; no capability-less reducer
   can claim it. `cui_event_target` alone is insufficient authority.
4. **Component request:** the new Slider model maps action to focused *targeted*
   thumb value±step (step0 uses existing span/100 policy), neighbor/domain clamps
   and min-origin snapping. The target selects thumb; it need not be keyboard
   focused. Each action emits BEGIN/UPDATE/COMMIT from one immutable entry pair,
   not a mouse gesture; no drag capture/preview survives. Existing source-f32
   accessibility range→Single behavior is deliberately rejected. Rejection by
   owner does not unconsume the action or propagate it to an ancestor.
5. **End-frame:** invalidate still-unclaimed numeric events whose target is absent,
   role changed, mask revoked, disabled/readonly, inert/layout-only or outside
   current modal scope. Discard them this frame; never replay next frame or onto
   reused IDs. Completed capabilities reflect final current semantics, not the
   event's previous admission. Preserve FOCUS rollback/ACTIVATE withdrawal paths.

Support means a handler can process a request, not that value must change. At
min/max or a neighbor boundary the new model may emit the same clamped value,
consistent with its keyboard/source repeated-update contract; it advertises both
numeric actions while enabled/writable with valid positive domain. Zero-span
linear, missing handler and readonly register mask0. No Core inference from float
numeric min/max determines step/domain/double precision. Log actions remain
additive in value units; reverse fill does not reverse increment/decrement.

Role/disabled/readonly/revocation changes **before claim** prevent callbacks.
Omission means no claim is called; final snapshot withdraws pending action.
No synchronous system can undo an already delivered request if the application
later changes the tree in that same frame. Claim freezes eligibility/evidence at
that point; callers defer destructive/reentrant changes until submission returns.
Late dismissal can withdraw remaining input and next-frame capabilities, not
retroactively revoke emitted owner intent. Do not promise transactional end-frame
rollback or delay all component callbacks to introduce a new dispatcher.
Same-ID/same-role logical replacement still requires a new CUI ID, as current
native-token contract states; no new hidden object generation is invented.

## Evidence and request ABI

Do not grow existing `cui_slider_event_input` or scalar callback records. The
proposed new model request owns its new input record containing action/target
alongside existing pointer-free fields. Actual numeric request has input=true,
ACCESSIBILITY source, frame/index from the original normalized queue event and
explicit action/target. It has **no HAS_ACTIVATION** and activation identity stays
zero. Use `cui_component_event_init` + frame-index helper directly, never
`cui_component_event_from_activation`. No fabricated keyboard fields, pointer
contact, activation or “physical screen reader” evidence from a test enqueue.

Multiple queued actions remain ordered individual inputs; each uses the call's
same immutable owner pair, matching existing controlled key snapshots. Acceptance
is external and appears next submission. The C model handler is synchronous;
bridge copies these requests and language callbacks remain after-frame, using
the independently reviewed acceptance receipt contract. Missing observer is
read-only, not silent auto-accept. Consumed input does not imply accepted value.

## Native adapter, lifecycle and portable fallback

Extend the adapter's existing private action mask with separate increment/
decrement bits populated solely by completed Core supported queries. Include new
bits in snapshot equality (already compares mask). Advertise AccessKit actions
only for those bits; native callback maps exact action to the matching CUI enum,
using the existing locked bounded token mailbox and foreign-thread wake contract.
No native callback may invoke Core/SDL/user handlers. Request mask validation must
use an explicit safe enum→bit mapping, not unchecked shift on caller enum.

UI-thread drain validates live token, live snapshot bit AND current completed
Core capability, then enqueues before begin-frame. Existing queue-full policy
retains unqueued requests and reports capacity; rejected stale/unsupported actions
are discarded with rejected stats. Do not label every failed enqueue CAPACITY:
unsupported/invalid/lifecycle failure must retain its actual diagnostic/status,
otherwise the new branch can turn permanent invalid work into infinite retries.
No duplicate accepted work after recovery. Snapshot capacity/string failure keeps
last published tree/tokens; Core eligibility is rechecked even when publication
failed. Closed backend synchronously disables wake and drops requests as today.

Removal/reappearance or role change gets a new backend token. Same-role mask
changes need not churn token: requests validate live bits at request/drain and
Core again at claim. Capability revoked and restored without any intermediate
published snapshot is indistinguishable from continuous support, like existing
same-frame identity; do not assert stronger stale-token protection. Native token,
CUI ID and bridge node generation are distinct authorities.

Existing optional Linux native accessibility integration is explicitly opt-in
([native guide](../NATIVE_ACCESSIBILITY.md)); it is not automatically created by
all bridge runners. Required host proof uses real pinned adapter action mapping
and a private DBus/Xvfb AT-SPI session, not only `cui_accessibility_request` helper.
Inspect which AT-SPI interface the pinned AccessKit platform emits for Increment/
Decrement before choosing the client call; do not invent action names or claim
AT-SPI Value set-current-value is supported by merely adding these two actions.
SET_VALUE is currently unsupported; the separately typed design below adds numeric
SET_VALUE only. Text editing remains unsupported. UIA/macOS/other platform delivery stays
unverified until tested; unsupported adapters publish no numeric capability and
return/report unsupported rather than translate to keys or direct state writes.

### Pinned Linux delivery blocker, verified during this audit

The already-retained Cargo.lock resolves `accesskit_unix`0.23.0 (crate checksum
`665c9079b7325a8168a6793437a172dda44b19a05af8ed52d7e32abaada996fe`)
and `accesskit_atspi_common`0.20.0 (checksum
`c9d47ad644916f6cb7e432a5ca0dbd7cc78281a9772881057bb72d4aadb93257`).
Read only those retained source files, no download/install/build:

| Pinned crate/file | Raw file SHA256 | Actual platform authority |
| --- | --- | --- |
| [accesskit_unix0.23.0/src/atspi/interfaces/action.rs](https://docs.rs/crate/accesskit_unix/0.23.0/source/src/atspi/interfaces/action.rs) | `7ffa334b1cace21aa567a074d0c1ba59dd2229449af02ffc329bfd4671940903` | Delegates AT-SPI action names/count/do_action to common PlatformNode. |
| [accesskit_unix0.23.0/src/atspi/interfaces/value.rs](https://docs.rs/crate/accesskit_unix/0.23.0/source/src/atspi/interfaces/value.rs) | `829dfa73ab581da1be9210c1a6a0361bbb8563fb8d49c7057557cc1ddefafbab` | Value property setter delegates set_current_value, not increment/decrement. |
| [accesskit_atspi_common0.20.0/src/node.rs](https://docs.rs/crate/accesskit_atspi_common/0.20.0/source/src/node.rs) | `ce331e201e4a65f68f2e4d155b034a27c30f513d7ca3645976a5968ef0d3cdec` | `n_actions`/`get_action_name`:532 only click when clickable. `do_action`:1104 index0 emits Action::Click; others false. `set_current_value`:1745 emits Action::SetValue with NumericValue. No Increment/Decrement mapping. |

Consequently **adding Core and CUI AccessKit callback support does not deliver
these actions through this pinned Linux AT-SPI platform**. An adapter helper
injection can test CUI mapping/mailbox only; it is not real native delivery proof.
Do not publish increment/decrement host capability on Linux as if that path exists. Keep
Core consumer capability distinct from native platform delivery capability.
Native Windows/macOS crates are not retained here and were not audited/tested;
locked versions alone do not establish support.

Root review rules out patching/updating this dependency. The additive typed
SET_VALUE alternative below reuses actual AT-SPI Value delivery without changing
its pinned source. Increment/decrement remain Core semantics for hosts that can
deliver them; an increment-only slice is **not Linux host completion**. Full
physical-AT acceptance is separately measured, not inferred from either path.

## All-five binding authority and compatibility

C model facade alone registers real capabilities. Binding options/method names
cannot assert support for a legacy/unpainted/readonly Slider. A versioned bridge
query/action API should resolve a live model node plus START/END thumb to its
actual stable semantic ID, then call completed Core supported/query queue; it
must validate node generation, mode, completed presence and thread/lifecycle.
No exposed raw ID cast from a language handle. Single START returns unsupported;
Range root is not an editable Slider target. Queueing uses actual accessibility
input, not Arrow keys or a direct accepted-value setter.

Python/JS/TS/Go/Rust wrappers expose the same optional capability query and typed
action enum through the new model API only. Native symbol absence reports
unsupported without breaking old library loading; TypeScript compiles against
the same real JS capability/method path. Distinguish (a) configured model support,
(b) current completed Core consumer capability, (c) installed native platform
numeric delivery, and (d) actual request acceptance. Only (b) authorizes Core
enqueue; only measured (c) supports a host delivery claim. A live request still
requires explicit owner acceptance after frame dispatch under the reviewed
bridge receipt contract. No existing scalar callback/event ABI changes.

Current bridge native runners do not automatically own a `cui_accessibility`
backend. Enabling it needs explicit lifecycle integration/configuration and proof
in each promised runner, reusing the existing optional adapter contract: create
outside stepping, drain before frame, publish after end-frame, destroy before
window/user teardown. Do not silently add backend creation to all runners, grow
old bridge options or claim installed-language native AT delivery from direct
Core tests. Renderer-only/headless hosts may support Core query/event tests while
reporting native accessibility unavailable. Browser canvas semantic host mapping
remains separately unsupported until implemented; DOM gallery controls are not it.

## Meaningful headless proof and review gate

No compiler/tests were run for this design. Required future evidence:

| Layer | Required proof |
| --- | --- |
| Old compatibility | Existing Core Focus/Activate and AccessKit unit/native/package suites retain enum values, normalized event bytes, timing, focus/capture, activations, rollback and error/capacity semantics. Legacy mutable/controlled/event Slider cannot advertise or queue numeric actions without explicit new registration. |
| Capability storage | Two independently registered Slider semantics; current append defaults0; mask replace/revoke; unknown bits/zero/wrong role/missing ID/duplicate ID; max_elements allocation/overflow/failure cleanup. Compaction through hidden/layout-only/dismissed scopes keeps masks attached to correct IDs; next frame does not inherit authorization. |
| Snapshot timeline | Previous allowed→current disabled/readonly/role change/revoked/omitted/current modal blocked rejects claim. Newly enabled capability unavailable until first completed publication. Gap/reappearance and different role cannot resurrect stale events. Late mutation after claim has documented non-rollback behavior; no replay. |
| Target/non-activation | Actual normalized numeric event retains only target/action; exact original frame/index; unchanged focus/capture/focus-visible, zero activation counts/slots/snapshots. Two targets/same input order, wrong target/index/action/consumed claim fails unchanged output. Claim consumes even owner rejects. FOCUS/ACTIVATE are never claimable as numeric actions. |
| Model values/evidence | Per-thumb additive step/clamp/log/range/limit/continuous/zero-span/read-only; pair never scalarizes or crosses. Same-call immutable snapshots, begin/request/commit counts, unchanged requested value permitted at bound. ACCESSIBILITY source and real input/action metadata, no invented key/pointer/activation. Copied request survives frame/context lifetime as documented. |
| Bounded recovery | Fill Core event queue and adapter mailbox, failed enqueue preserves input/focus/capture, clear error/drain recovery delivers once, unsupported work discarded rather than endless retry, snapshot overflow preserves tree/token but rechecks Core eligibility. Foreign-thread wake/non-reentrancy, close/destroy/no stale user access use existing sanitizer/lifetime fixtures. |
| CUI adapter | Verify published AccessKit action masks match Core exact capability; native callback maps Increment/Decrement independently; unknown native actions freed/ignored; current token/role/mask/generation checks. Helper injection proves this layer only. No advertisement of an unimplemented native platform delivery route. |
| Real native platform | For the typed alternative below, call actual AT-SPI Value SetCurrentValue through private DBus/Xvfb; observe actual new model request with ACCESSIBILITY origin and accepted next-frame pair/paint. No direct helper call or keyboard event substituted. Other OS/browser hosts remain unsupported/unverified until independently proven. |
| Five runtimes | Actual Python/JS/TS-runtime/Go/Rust new-model queries and action queue calls target correct thumb, preserve all metadata and after-frame callback timing, reject stale nodes and unavailable symbols, accept/reject request explicitly. Verify legacy scalar paths untouched; actual native runner proof separate from Core/API tests. |
| Distribution/docs | Strict C11 selective/umbrella declaration+link checks and ABI old-client probes; generated SDK/dist/skill/source metadata, all78 entries, usage JSON+exact mirrored guide and component_usage tests. Explicit supported-host and claim-vs-acceptance documentation. |

No public struct growth, keyboard fabrication or global dispatcher is needed for
Core numeric actions and typed SET_VALUE payloads. Explicit bounded semantic capability storage and a validated
current-semantic claim boundary are the minimum coherent Core changes. Do not
ship bare enum additions or pretend optional AccessKit action tags close the
pinned Linux platform gap. Concrete API/error/sidecar tests and the native
typed payload/native integration contracts require integrator review before production.

Signed: `/root/hidden_windows`, 2026-10-31. Design-only checkpoint; no production,
generated files, compiler/dependency changes, merge or push.

## Root-requested typed SET_VALUE alternative (no dependency changes)

This section extends/supersedes the increment/decrement-only claim prototype
above before any API ships. Append SET_VALUE=4 after INCREMENT2/DECREMENT3;
registration gains numeric SET_VALUE bit2. Keep Focus/Activate derived policy and
old event/semantic/details/config/activation layouts unchanged. A legacy Slider
still registers nothing; no action is advertised merely because role is Slider.

### Exact new portable records and queue/accessor boundary

```c
#define CUI_ACCESSIBILITY_NUMERIC_SET_VALUE (1u << 2)
#define CUI_ACCESSIBILITY_NUMERIC_INPUT_V1 1u
#define CUI_ACCESSIBILITY_NUMERIC_HAS_VALUE 1u
typedef struct {
    uint32_t version;  /* exactly1 */
    uint32_t reserved; /* exactly0 */
    double value;     /* finite, semantic numeric units */
} cui_accessibility_set_value_v1;
typedef struct {
    uint32_t version, action, flags, reserved; /* action is fixed-width enum tag */
    cui_id target;
    double value; /* present only for SET_VALUE; canonical0 otherwise */
} cui_accessibility_numeric_input_v1;
bool cui_queue_accessibility_set_value_v1(cui_context *, cui_id, uint32_t payload_size,
                                           const cui_accessibility_set_value_v1 *);
bool cui_claim_accessibility_numeric_input_v1(cui_context *, cui_id, size_t event_index,
                                               uint32_t output_size,
                                               cui_accessibility_numeric_input_v1 *out);
```

Use the versioned claim instead of publishing two overlapping numeric claim
functions. It handles increment/decrement (flags0/value0) and SET_VALUE
(HAS_VALUE/value copied exactly). Generic no-payload `cui_queue_event` or
`cui_queue_accessibility_action(...SET_VALUE)` rejects INVALID_ACTION: value0
must not be guessed from missing payload. Existing no-payload enqueue still
works unchanged for Focus/Activate and explicitly numeric increment/decrement.
Supported query may return true for SET_VALUE; the documented typed entry point
is required to enqueue that action. No value hidden in x/y, key, text, pointer
identity, activation or unrelated fields.

Queue validates exact record size/version/reserved0, nonnull pointer, finite
value, current completed capability/target and outside-frame lifecycle **before**
queue/payload mutation. NaN/Inf/unknown versions/malformed size fail atomically.
Copy the scalar into private owned storage before return; caller may reuse stack
payload immediately. No arbitrary blob, borrowed pointer, allocation per event,
new transport abstraction or generic event bus. New records are additive; no
existing public-by-value type grows. Matching new API clients require matching
new symbols, while old ABI clients retain existing behavior.

### Bounded sidecar and stable event-index lookup

Existing Core has queued events bounded by CUI_CORE_EVENT_MAX and dispatched
events bounded by CUI_CORE_DISPATCH_MAX. Begin-frame copies queued events and
sets `event_origin[i]=i`. Touch handoff can insert synthetic cancellations
(origin SIZE_MAX), memmove origins and later swap origins with events
(Core1429–1433,1473–1483,1718–1720). A sidecar naively indexed by dispatch offset
would attach the wrong value after those operations.

Use two private fixed arrays, queued payloads and frame payloads, each exactly
CUI_CORE_EVENT_MAX entries of `{bool present; double value;}` (or equivalent
explicit tagged slot). Before incrementing queued_count, internal enqueue commits
the normalized event and matching payload as one operation. Ordinary events
explicitly clear the slot. Typed enqueue cannot call a public generic enqueue
that first rejects SET_VALUE or publishes a half-filled slot.

At begin-frame copy queued payload slots into frame origin slots alongside event
copy; clear reused queued slots. Retain the original queued count as a private bounded frame count. Claim maps
actual dispatch event_index through `event_origin[index]`, validates origin<count, SET_VALUE tag,
presence, finite value and capability. Synthetic events (origin SIZE_MAX), missing
payloads and mismatched event/action cannot read an old value. Origin already
moves/swaps with its event; no second dispatch-array movement protocol is needed.
Freeze record into new component callback payload before invoking owner code.
Clear frame presence for every new frame, and on context destruction/recovery;
no values replay after overflow, omission or rejected lifecycle call.

Storage cost is bounded by two scalar sidecars: typically32*CUI_CORE_EVENT_MAX
bytes if each slot is16 bytes, exact `sizeof` recorded in implementation evidence.
Private semantic capability arrays still cost8*max_elements bytes. Both bounds
already exist; no public config/layout changes or unbounded receipt registry.
Allocation/size failure follows existing create cleanup; a static private-context
array is also viable within the same fixed bound, subject to root review of
context size. The design does not require global input-dispatch replacement.

### Semantic units, domain conversion, range and precision

Core transports a finite double in **published semantic numeric units**, not
arbitrary application units and not a screen percentage. Core does not infer
step, scale, pair shape or domain from its float semantic fields. A custom
component opting in owns its documented conversion. For the new Slider model,
semantics publish value-domain normalization `(v-min)/(max-min)` in0..1, including
when visual placement is logarithmic. The selected thumb is the exact target
semantic ID; root and missing Single START do not opt in.

The model clamps incoming semantic value to0..1, converts with double
`min+fraction*span` (exact0/1 endpoints), applies its value-unit min-origin step,
then domain and neighbor constraints. This is a SET request, not a series of
increment requests and not inverse visual-log mapping. Range remains Range and
only targeted endpoint changes. Disabled/readonly/missing handler/zero-span
publishes no SET capability. Reverse fill has no effect. Out-of-range finite
incoming requests clamp at component domain/neighbor, rather than storing an
invalid owner or fabricating keyboard input; preserve the original input double
in request evidence separately from the clamped/snapped proposal.

The incoming f64 is retained without float conversion. High domain offsets
(e.g.1e10 plus .125) use the existing double model conversion and explicit owner
acceptance. However published semantic current/min/max remain float-normalized
because the existing semantic ABI is float; this channel does not recover
precision absent from a host's queried current value. Per-thumb descriptions
carry precise application units. Actual native AT-SPI query values and rounding
must be tested; do not claim full native double-value query parity.

Current/previous capability, same-role, enabled/readonly, modal and claim rules
are identical to increment/decrement, with SET_VALUE's payload presence additionally
required. Claim consumes input independent of acceptance. It generates actual
ACCESSIBILITY BEGIN/UPDATE/COMMIT with no focus/capture/activation/key. New model
request owns a versioned numeric input copy including actual action/target/value;
existing scalar event input ABI is untouched. Programmatic tests exercising this
API establish accessibility-route evidence, not physical assistive-technology use.

### Actual native Value path and copied mailbox payload

Add a versioned native adapter request-set-value function using the same owned
record and token authority. Its private bounded mailbox request slot gains
`has_value,double value` only; old public config/stats/semantic/event structs do
not grow. The native callback accepts AccessKit SET_VALUE only when
`request.data.has_value` and `data.value.tag==ACCESSKIT_ACTION_DATA_NUMERIC_VALUE`,
then validates finite `numeric_value`, copies it while request is alive, and frees
the AccessKit request exactly once on all paths. No cast of a string/value union
arm and no retained AccessKit pointer. Missing/wrong payload tags fail/report
rejection; no direct model mutation from the native thread.

Snapshot publishes AccessKit SET_VALUE only if completed Core query says true
and the native platform route is implemented. One pinned-platform nuance must be handled honestly: common NodeWrapper
`supports_value`:490 depends only on a numeric current value, so the AT-SPI Value
interface (including its setter method) exists even when AccessKit SET_VALUE is
not advertised. Do not claim its presence proves writability. For Slider nodes
without SET capability, derive native read-only state in the adapter snapshot
while retaining numeric query values and existing Focus/Click action policy;
Core legacy semantic/input behavior is unchanged. A caller ignoring native
read-only can still invoke the method, but mailbox/Core capability rejection
prevents a request or owner mutation. This small truthful native-state change
needs explicit legacy snapshot regression review, not hidden role-wide opt-in.
The retained `accesskit_consumer`0.39.0 `src/node.rs` SHA256
`79686e8c6867cff462dd7920bba8774dc01163829f1cfb75d46af07cbf2ec6a2`
(`is_read_only`:469, `is_read_only_supported`:861) confirms Slider supports
read-only state; common node state mapping376 publishes that state. Native
Focus capability is still independently derived, not removed because numeric
actions are unavailable.

UI drain rechecks token and current
Core SET capability and calls the typed Core queue function with owned scalar;
Core queue-full preserves the whole original mailbox slot/value for later drain.
No payload is lost or re-associated on interleaved Focus/Click/increment requests.
Existing locked foreign-thread wake and close/destroy contracts remain unchanged.
Linux's pinned `PlatformNode::set_current_value` already emits SetValue with
NumericValue, so this uses existing actual platform delivery with no patch/update.
Increment/decrement advertisement remains gated by actual host capability; do
not claim their Linux delivery merely because Core now accepts them.

Real Linux proof must call AT-SPI Value SetCurrentValue over private DBus/Xvfb,
observe native callback→owned mailbox→typed Core event→new model request with
original semantic double, then accept/reject and inspect next-frame pair/paint.
No direct typed queue helper or owner write may substitute for that chain. Query
and validate actual platform numeric range/current values first. Native physical
screen-reader/AT acceptance, UIA/macOS, browser semantic delivery and other host
claims remain separate verification requirements.

### Additional tests and binding implications

- Queue malformed version/size/reserved/null/NaN/Inf atomically; typed zero is
  distinguishable from missing payload. Generic SET enqueue rejects. Failure
  leaves queued count and prior payload/interaction state intact.
- Interleave typed SET values with text, Focus/Click, pointer events causing
  synthetic touch-cancel insert/reorder and multiple numeric targets. Verify
  original dispatch index maps to its correct origin payload, no slot reuse or
  frame carryover, malformed synthetic origin cannot claim a scalar.
- Claim invalid index/target/action/output-size/missing payload/revocation/role/
  omission/read-only/disabled/modal fails unchanged output and consumption.
  Successful claim consumes once even if owner rejects. No activation identity.
- Check normalized semantic0,.125,.5,1 and finite outside-range inputs against
  independent domain/neighbor/step oracle, linear/log/vertical/Range, high-double
  offsets and endpoint roundoff. Verify actual requested pair and preserved raw
  incoming semantic scalar separately; no scalarization or float downgrade.
- Native malformed AccessKit data tags/free/lifetime, copied-stack mailbox value,
  queue overflow/retry/destroy, stale tokens, same-role capability changes and
  real private-session Value setter proof. Existing Focus/Click test suites stay
  unchanged, plus current unsupported legacy Slider behavior.
- All five optional bridge wrappers use a dedicated typed semantic SET request
  API, never `.value()` accepted-owner setters or simulated arrows. Capability
  queries resolve live model/part authority. TS compile+runtime, actual Py/JS/
  Go/Rust tests verify f64 payload/evidence copying and after-frame acceptance;
  actual native host integration is separately proven, not implied by bindings.

Root review is requested for this narrowly typed payload sidecar and native
SET_VALUE route. No dependency patch/update, public struct growth, general event
bus, keyboard fabrication or new dispatcher is proposed. An independent Core
increment/decrement slice remains partial and does not close Linux host delivery.

Signed addendum: `/root/hidden_windows`, 2026-10-31. Design only; no production,
generation, compiler or dependency mutation.
