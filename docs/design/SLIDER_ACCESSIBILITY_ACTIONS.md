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

Store numeric capabilities/payloads in a **lazy owned numeric-state allocation**
behind one private context pointer, initiallyNULL. Allocate only on the first
otherwise-valid nonzero capability registration; mask0 before opt-in allocates
nothing. Existing `cui_create` performs no new allocation and gains no new OOM
branch for legacy users. Arrays remain bounded by existing max_elements/event
limits and live until context destroy. The exact layout/failure/publication
contract in the revision below supersedes the earlier eager allocation idea.

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

The arrays live in the lazy numeric state described below, never inline in every
context and never allocated by legacy cui_create. Their opted-in allocation is
bounded by existing event/semantic limits; no public config/layout changes or
unbounded registry.

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
not advertised. Do not claim its presence proves writability. Only under the explicit numeric-enabled backend mode below, for Slider nodes
without SET capability derive native read-only state in the adapter snapshot
while retaining numeric query values and existing Focus/Click action policy;
Core legacy semantic/input behavior is unchanged. A caller ignoring native
read-only can still invoke the method, but mailbox/Core capability rejection
prevents a request or owner mutation. This observable native-state policy is NOT applied by the existing constructor;
default Focus/Click snapshots remain unchanged. New mode requires explicit opt-in
and regression review, not hidden role-wide policy changes.
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

## Revised allocation, native compatibility and failed-frame contract

This root-reviewed revision supersedes eager storage or unconditional native
read-only policy in earlier discussion. Design only; no production approval yet.

### Lazy per-context owned numeric state and exact cost

Add only `cui_core_numeric_state *numeric` to private context, initializedNULL.
**Idle context cost is exactly one pointer (`sizeof(void *)`, ordinarily8 bytes
on64-bit /4 on32-bit), subject only to normal containing-struct alignment.**
No inline event arrays, capability arrays or numeric allocation in `cui_create`.
Legacy create failure cases and old Focus/Activate allocation path do not change.

Proposed private C11 allocation layout:

```c
typedef struct { double value; bool present; } cui_core_numeric_slot;
typedef struct {
    size_t frame_origin_count;
    cui_core_numeric_slot queued[CUI_CORE_EVENT_MAX];
    cui_core_numeric_slot frame[CUI_CORE_EVENT_MAX];
    uint32_t masks[]; /* current[N] followed by previous[N], N=config.max_elements */
} cui_core_numeric_state;
```

One zeroed allocation of exactly
`sizeof(cui_core_numeric_state) + 2*N*sizeof(uint32_t)` bytes; implementation
checks `N <= (SIZE_MAX-sizeof(state))/(2*sizeof(uint32_t))` before arithmetic.
`sizeof(state)` includes its fixed arrays/header/alignment, so this is the exact
allocation formula, not an assumed unpadded byte sum. On ordinary64-bit ABI with
16-byte slot the sidecars contribute32*EVENT_MAX bytes, header/alignment normally
sizeof(size_t) rounded to struct alignment, plus8*N capability bytes; actual sizeof values belong in test evidence.
Any implementation layout change must retain explicit formula and bounded cost.

Allocate on first **otherwise-valid nonzero** numeric registration, after all
lifecycle/ID/role/mask/error checks. Publish pointer and requested current mask
only after calloc succeeds. `mask0` on a valid current Slider with pointerNULL
returns true and allocates nothing. Failed validation or allocation leaves pointer,
previous/current masks, event queues/payloads, focus/capture and callback delivery
unchanged. OOM/size overflow reports existing CUI_ERROR_CAPACITY; no invented
allocation error enum. Root must approve meaningful allocator-failure test hook;
no public allocator/config expansion is needed. Once allocated, retain until
context destroy (including if all masks later0), free exactly once; clear/revoke
never reallocates or loses pending input. Queue/claim never allocate: without
numeric state no numeric action is supported, regardless of role.

This bounded allocation may happen during the first opt-in painted submission;
it is an explicit cost of enabling the new feature, not a hidden legacy setup
change. If Core policy later requires pre-reservation, that is a separate additive
opt-in contract rather than returning to eager allocation for every context.

### Explicit native backend mode; old constructor stays old

Add new owned options record/constructor; do not grow existing
`cui_accessibility_config`, window, stats, events or semantics:

```c
#define CUI_ACCESSIBILITY_OPTIONS_V1 1u
#define CUI_ACCESSIBILITY_ENABLE_NUMERIC 1u
typedef struct {
    uint32_t version; /* exactly1 */
    uint32_t flags;   /* known ENABLE_NUMERIC only */
    uint32_t reserved[2]; /* both0 */
} cui_accessibility_options_v1;
cui_accessibility *cui_accessibility_create_with_options_v1(
    cui_accessibility_config config, uint32_t options_size,
    const cui_accessibility_options_v1 *options);
```

Exact size/version/known flags/reserved validated before allocation; null or
malformed options fail (constructor returnsNULL). flags0 explicitly selects the
same policy as legacy `cui_accessibility_create(config)`; old constructor keeps
Focus/Click only, identical action masks/read-only projection/token snapshots,
threading and native callback policy. It never derives native read-only because
SET_VALUE is absent. Numeric APIs on a default-mode backend report unsupported
without creating a hidden new mode. Existing symbol loading/binaries unchanged.

ENABLE_NUMERIC explicitly opts that backend into completed Core numeric queries,
implemented host action mapping, typed mailbox payloads and native Slider
read-only derivation when SET unsupported. It does not grant capability to any
Core semantic. An application choosing new mode may therefore see a legacy
Slider read-only in native AT projection; that is a documented new-mode policy,
not a global legacy snapshot change. Focus/Click remain independently derived.
Do not equate new-mode backend creation with native-active or working AT delivery.

Keep default-mode private mailbox payload cost unchanged where practical: owned
numeric mailbox slots can be a separate `max_actions`-bounded scalar sidecar
allocated only by numeric-enabled constructor, with checked `count*sizeof(slot)`
and normal create cleanup. Old token/action mailbox ordering remains authoritative;
sidecar clears/moves with head/tail reuse. No per-request allocation, borrowed
native pointer or public capacity growth. Exact additional allocation formula is
`max_actions*sizeof(cui_core_numeric_slot)` (or backend-local equivalent same
owned scalar+presence record); report real sizeof. New-mode allocation failure
may fail the new constructor; it must not add a new failure path to old creation.

All five future binding host opt-ins must choose this constructor/policy explicitly
through a separately reviewed versioned host integration API; no new flag stuffed
into existing bridge/run options. Without it, old backend snapshots stay old,
new Core helpers can still be tested, but installed native numeric delivery is
unsupported. No typed Slider/binding production is approved by this addendum.

### Failed opt-in layout frames cannot certify numeric snapshots

Coordinated with the Core scroll-layout worker: opting into new layout constraint
calculation and failing it yields empty draw and **no valid geometry/prepaint/
semantic snapshot promise**. Trial calculations publish nothing. Successful
calculation commits borrowed scalar offsets together before the normal single
semantic pipeline; later errors do not roll back committed offsets/application
or input effects. Legacy zero-opt-in frame failure behavior remains unchanged.

Existing `cui_last_error` is sufficient to detect the failed constraint case:
completed failure has non-OK error, including restoring CUI_ERROR_LAYOUT at end
if a caller cleared an error inside the failed frame. No dependency on another
worker's private marker names is needed. The approved narrow predicate below
exposes only completed Core semantic readiness, not a general layout observer.
Numeric publication integrates at Core's ordinary completed semantic commit:

- Only successful painted semantic commit with `cui_last_error==CUI_OK` can copy
  current masks alongside final semantics and set `published_valid=true`.
- Any non-OK completed frame while numeric state exists marks numeric snapshot
  invalid, zeroes current/previous authorization and frame payload presence, and
  discards unclaimed numeric input. It does not publish partial registration or
  infer valid geometry from retained nonempty semantic arrays. Old Focus/Activate
  behavior remains unchanged; this fail-closed rule is for new numeric actions.
- During a frame, numeric registration/claim rejects once current error is non-OK;
  no new numeric request is delivered after known invalidity. A request already
  claimed before a later layout/error failure is not rolled back. This is the
  same explicit non-transactional callback boundary as the main design.
- Outside-frame numeric support/queue also requires `published_valid` AND OK
  current error. Clearing sticky error alone does not restore validity; recovery
  requires a later successful completed painted semantic commit with fresh
  explicit registrations. No stale request/payload/gesture intent is replayed.
- Current registration only is not completed authority. First successful nonzero
  registration allocates storage but no enqueue is authorized until successful
  end-frame publication. Failed initial opt-in keeps storage owned/reusable while
  granting no actions. mask0 still allocates nothing before any opt-in.

Native snapshot update must not read/certify failed Core semantic snapshots.
Non-OK Core frame causes numeric-enabled update to reject/retain its last good
published tree and suspend numeric mailbox admission/drain until a later valid
completed snapshot (query/commit rules above). Requests already queued against a
previous native token are rejected at drain when Core numeric support is invalid;
never retried as capacity-only work. A retained OS tree may temporarily display
old capabilities after update failure; no consumer intent is delivered from that
stale display. Document this stale-publication limitation rather than pretending
an invalid Core frame produced a valid replacement tree. Default backend policy
and legacy snapshot error semantics remain unchanged.

### Mandatory normalization/reset/compaction audit before implementation

Implementation must enumerate and test every following lifecycle path; a single
successful queue test is insufficient:

| Path in current Core | Numeric obligation |
| --- | --- |
| Create/destroy (`cui_create/cui_destroy`) | Initialize/free only lazy pointer; allocator-failure ownership, no legacy allocation/calloc-count changes. |
| `cui_interact` append / per-frame semantic reset | Current mask starts0 for each newly appended semantic; no inheritance from previous/reused slot. Layout-only/inert does not register. |
| Normal generic enqueue / typed enqueue normalization | Ordinary event clears payload slot; typed SET validates/copies atomically; no-payload SET rejected; invalid/capacity/lifecycle leaves all preexisting payloads intact. |
| Begin-frame queued→dispatch copy/origin initialization | Copy only original queued payloads and original count; reset queued presence safely; clear old frame slots every frame. Failed begin admission must not discard a previously valid queued value. |
| Touch cancellation insert/memmove and later origin swap | Origin lookup stays attached to original SET event; synthetic SIZE_MAX has no value; no dispatch-offset alias. Check every reorder site, not only first memmove. |
| Normalization / suppression / event consumption | Payload cannot authorize wrong type/action/target; consumed/withdrawn numeric events cannot be claimed twice or from stale frame. Ordinary Focus/Activate normalization/evidence unchanged. |
| Semantic visible compaction / focus-scope dismissal | Copy masks with IDs/nodes/scopes at every retained index; cleared tail cannot grant capacity-reused semantics. |
| Details mutation / role / readonly / disabled / modal changes | Claim checks current AND previous authorization; later changes revoke final publication/remaining input, no rollback already delivered request. |
| Successful final snapshot copy | Caps commit with final completed semantics exactly once, valid flag true only OK painted publication. |
| Error/failed new-layout frame / sticky clear/rebuild | Invalidate numeric authority and discard unclaimed payloads; no intent after known error, no recovery from mere error clear, no replay. Preserve old zero-opt-in behavior. |
| Adapter snapshot failure / mailbox retry / close | Retain last good tree but suspend new numeric intent while Core invalid; capacity-only retry preserves exact scalar once, unsupported invalid work discarded, payload slots cleared on head/tail reuse and close. |

Add allocation instrumentation proof for untouched legacy scene and mask0 scene
(no numeric allocation), first nonzero successful registration (one exact bounded
allocation), failed first allocation (atomic), repeated registrations/revoke/recovery
(no reallocation), context destruction (one free). Add numeric-enabled vs old
backend snapshot goldens proving read-only/action changes are opt-in only. New
layout constraint failure tests include initial registration failure and previously
valid numeric snapshot→failed frame→error clear→good-frame recovery, plus a claim
made before later failure to prove no dishonest rollback/replay promise.

Signed revised contract: `/root/hidden_windows`, 2026-10-31. No production,
compiler, dependencies, generated files, merge or push; pending root review.

### Approved snapshot-ready predicate and production boundary

Root selected the following narrow predicate instead of the three-state proposal:

```c
bool cui_accessibility_numeric_snapshot_ready(const cui_context *);
```

It means Core has a valid completed semantic snapshot. It certifies **neither**
renderer/presentation success, working native bridge, physical AT acceptance,
precise domain-value query parity nor any particular numeric capability.

- false for NULL, before first successful completed frame, during any active
  frame, and after a failed begin/end that invalidates layout/semantic geometry;
- false persists after clearing the error from such an invalidating failure;
  only a fresh successful completed frame restores it;
- true after an ordinary successful frame even with no numeric allocation, no
  registered capabilities, or an empty semantic list;
- new numeric supported/enqueue paths require readiness and per-target previous
  capability. True readiness alone never opts a legacy Slider into any action;
- current claim uses the retained valid previous snapshot plus current eligibility
  and error checks; the public predicate is intentionally false during frame.
  Known failed current input/layout cannot deliver more requests. Already claimed
  requests are not rolled back by a later failure.

Add one private context validity flag, separate from lazy numeric state; remove
the redundant `published_valid` field from the lazy record. Initial flagfalse;
set only at successful ordinary semantic publication. Invalidating begin/end
failures clear it before returning, including new scroll-layout constraint
failure and downstream end error. Error clear never sets it. Public predicate
never allocates. Failed-begin admission checks must distinguish non-invalidating
API misuse from geometry-invalidating failure; test exact cases rather than
claim that every error destroys a snapshot. While a non-geometry sticky error
is present numeric actions may conservatively reject; clearing that error does
not recreate geometry and cannot override a false validity flag.

The private idle delta is **one pointer plus one bool and containing-struct
alignment**, not exactly8 bytes. Report baseline/modified `sizeof(cui_context)`
and flag/pointer offsets on tested ABIs before implementation handoff; an
8-aligned insertion of8+1 bytes may cost16 bytes, and32-bit may similarly add
padding. Do not promise a platform-independent8-byte context growth. The only
legacy cost is this bounded private layout delta; no eager numeric allocation or
new cui_create OOM branch. Lazy record becomes `{size_t frame_origin_count;
queued[EVENT_MAX]; frame[EVENT_MAX]; uint32_t masks[];}`. Exact allocation stays
`sizeof(cui_core_numeric_state)+2*N*sizeof(uint32_t)` with checked overflow; no
published-valid flag in that allocation. Normally slot16 yields fixed header
`sizeof(size_t)+32*EVENT_MAX`, rounded to its actual struct alignment; record
actual values instead of relying on the illustrative estimate.

Numeric-enabled adapter update and drain must call the predicate. Update false
rejects/suspends numeric publication, retaining last good copied tree rather than
reading/certifying invalid Core semantics. Drain false discards pending numeric
requests as invalid, never capacity-retries them. On recovery, fresh successful
completed snapshot plus explicit per-target registration reauthorizes actions.
Only genuine Core event-queue fullness retains numeric requests for retry; other
invalid/stale/lifecycle outcomes are rejected, with accurate diagnostics. True
predicate with no capabilities may still publish truthful new-mode read-only
projection; no allocation or fake registration is required.

Old accessibility constructor/mode and old FOCUS/ACTIVATE paths **never use this
predicate**. Their existing timing/routing/error/snapshot behavior stays unchanged.
Only the new numeric-enabled backend constructor policy gates on readiness; its
versioned16-byte options record, explicit ENABLE_NUMERIC flag, reserved-field
validation and separate bounded scalar mailbox cost above remain the contract.
Host/Core snapshots and typed input capability are distinct from request owner
acceptance; no rollback of callbacks already delivered.

This resolves the observer decision. After the Core layout worker's handoff/base
is available, root authorizes only the narrow Core capability, typed queue, claim
and readiness slice, with real new-layout failure interoperability tests. Host
backend implementation, typed Slider model, bridge receipt ABI, host option
transport and all5 binding production remain independently gated. No pinned
dependency patch, general dispatcher, keyboard fabrication, old public struct
growth or full-host claim from Core helper proof.

Signed contract revision: `/root/hidden_windows`, 2026-10-31. Design only; no
production/compiler/dependency/generated changes at this checkpoint.
