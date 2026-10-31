# Slider range, orientation and scale: complete-family design checkpoint

Design only, based on signed palette followup `9fda2427`. No source/API/binding,
Core dispatch, renderer or generated-resource changes. Palette and gallery
branches remain separate. This proposal covers missing behavior alongside the
presentation/brush work; none of these subsets independently closes Slider.

## Evidence and actual source cardinality

Pinned upstream revision: `87d10ae5e1299d1be18670a37570c5402f952e34`.
Only the already cached primary files were read; no dependency install or checkout.

| Primary file | SHA256 | Exact relevant symbols |
| --- | --- | --- |
| [Base Slider](https://raw.githubusercontent.com/longbridge/gpui-kit/87d10ae5e1299d1be18670a37570c5402f952e34/crates/base/src/slider.rs) | `b677bee1185b65a2fe4b3ecfcd247395be57d8bf7543c3cc0f9e0eac6ff9e0b6` | `SliderValue`, `SliderScale`, `SliderState::{update_thumb_pos,update_value_by_position,is_nearer_start,handle_release}`, `Slider`, `SliderTrack`, `SliderIndicator`, `SliderThumb` |
| [Component Slider](https://raw.githubusercontent.com/longbridge/gpui-kit/87d10ae5e1299d1be18670a37570c5402f952e34/crates/component/src/slider.rs) | `94afa288c201ae24c67bb176d9825eef32d2b32d849810275b5051a6bcdb984d` | `Slider::{horizontal,vertical,reverse,render}`, `ThumbRing`, `ThumbInteraction` |

Base `SliderValue` (lines28–125) is **Single(f32) or Range(f32,f32)**. There is no
arbitrary-N thumb vector. One or exactly two thumbs covers inspected source
cardinality; do not advertise arbitrary multiple handles. Source range pair may
initially be reversed: `clamp` clamps each endpoint independently and
`update_thumb_pos` does not order the pair. User setters prevent crossing by
`set_start(value.min(end))` / `set_end(value.max(start))`.

| Source behavior | Observed exact contract / quirk |
| --- | --- |
| Defaults | min0, max100, step1, Single0, Linear, horizontal; source retains owned entity state. |
| Coordinates | Horizontal `(x-left)/width`; vertical `(bottom-y)/height`. Clamp coordinate0..1, then start to `[0,end_percentage]` / end to `[start_percentage,1]`. |
| Logarithmic | Requires min>0 and max>min when setting log mode; builder min/max validation depends on current mode. Forward `min*(max/min)^p`, inverse `log(value/min)/log(max/min)`, with source clamps. Linear zero/nonpositive span maps percentage0. |
| Snapping | `round(value/step)*step`, **zero-origin**, after mapping and percentage neighbor clamp. No final min/max clamp after snapping. Invalid/nonpositive step is not uniformly validated in source. |
| Stored percentage | Pointer update stores raw, unsnapped coordinate separately from snapped value. It can differ from a subsequent `update_thumb_pos` result. |
| Range track click | Nearest thumb by midpoint of *visual percentages*, strict `<` selects start; exact midpoint selects end. Source mouse track dragging is installed only in Single mode; range drag comes from individual thumbs. |
| Thumb mouse | Mouse-down stops propagation. `dragging` becomes true on a positional update, so a thumb click without update need not emit Release. |
| Events | Every positional update emits Change, including unchanged snapped values. Release emits once only after `dragging` was true. No source BEGIN or CANCEL event. |
| Touch | Track touch hitbox claims only non-prevented, topmost drag; remembers start position and thumb. Started/Moved update; Ended/**Cancelled** both Release. A missing end due to disabled/omitted UI is cleaned when another touch starts, not by an immediate omission callback. |
| Reverse | Single fill goes thumb→max instead of min→thumb. Range ignores reverse. It does not reverse values, axis, keyboard or pointer mapping. |
| Presentation | Horizontal source track hit-height24/indicator6, vertical hit-width24/indicator6 and default height120; thumb16 with1px padding and -8px centering at the default16px rem (the source mixes rem-derived sizes and literal pixel offsets). Source theme corner/style and animated hover/pressed ring apply to each thumb. Disabled closures omit visible thumb subtree. |
| Accessibility | Root role Slider exposes only `value.end`, min/max/step/orientation. Increment/Decrement call `set_value(end±step)` and therefore **convert Range to Single**. Those closures are outside `when(!disabled)`. `set_value` notifies but does not emit SliderEvent. |
| Keyboard | No direct key listener in these two files. Accessibility actions are not evidence of source arrow/Home/End listeners. |

Source division by zero, nonfinite builder inputs, inverted range, zero-origin
snapping overshoot, scalarizing a range accessibility action and disabled a11y
mutation are observations, not behavior CUI should silently import. Exceptions
below must be explicit in human docs, machine metadata and oracle cases.

## Current CUI inventory and preservation boundary

| Current authority | Actual implementation / proof |
| --- | --- |
| [`cui_base.h`](../../include/cui/cui_base.h): `cui_base_slider` | Legacy mutable float scalar. [`cui_base.c`](../../src/base/cui_base.c):245 uses X/width, min-origin quantization with final clamp; arrows/Home/End; no new state/callback. |
| `cui_base_slider_controlled`, `cui_slider_state/options/request` | Same Base source:295–454. Controlled double scalar; caller state preview/start/dragging, immutable entry snapshot; callback offers BEGIN/UPDATE/COMMIT/CANCEL without mutating owner. Missing callback read-only; disabled/readonly/zero-span cancel. Pointer uses prior completed bounds, targeted unconsumed events, primary button, Core capture/touch routing. |
| [`cui_slider_events.h`](../../include/cui/cui_slider_events.h), `cui_base_slider_events` | Typed pointer-free copyable evidence, phase counts and saturation; UPDATE=REQUEST, COMMIT=COMMIT, others LIFECYCLE; never CHANGE. Legacy and typed callback paths mutually exclusive. [`test_slider_events.c`](../../tests/unit/test_slider_events.c) proves phases, cancel, immutable snapshots, precise doubles and evidence. |
| [`cui_styled_slider_events.h`](../../include/cui/cui_styled_slider_events.h), [`cui_styled_slider_events.c`](../../src/components/cui_styled_slider_events.c) | Controlled scalar horizontal bar/label, no thumb. Fixed/custom width/hit/track/font/color; preview only while dragging, confirmed entry paint after release. Admission before boxes; reducer once. [`test_styled_slider_events.c`](../../tests/unit/test_styled_slider_events.c): preview/accept/reject, high precision, geometry, layout-only, invalid, Base equivalence, touch/cancel/label. |
| [`cui_components.h`](../../include/cui/cui_components.h), [`cui_components.c`](../../src/components/cui_components.c):2130 | `cui_slider` legacy float Styled route and stable track getter. 260px root,240px bar8; no thumb, vertical or log. Existing IDs/default geometry remain byte-for-byte legacy routes. |
| [`test_slider_controlled.c`](../../tests/unit/test_slider_controlled.c) | Actual queued pointer/key, high double values, cancel/read-only/disabled/zero-span/invalid admission. Existing tests remain required, not replaced by new model tests. |
| [`cui_slider_palette.h`](../../include/cui/cui_slider_palette.h) | Pure owned colors/brushes for4 skins+38 presets; no rendering, reducer, thumb identity or binding authoring. Independent all38 oracle is already separate. |
| [`cui.h`](../../include/cui/cui.h) / [`cui.c`](../../src/core/cui.c) | Existing role Slider, orientation, semantic parent/name/description/range; numeric ABI float. Pointer contact type/device/id, target/consumption, one capture, touch drag priority, focus APIs already exist. End-frame clears omitted capture. Public accessibility actions currently only FOCUS/ACTIVATE, **not increment/decrement**. |

Legacy request callbacks must retain signature, phase timing, copied current
value precision, keyboard same-call snapshots, reason/evidence/count meaning,
missing-handler behavior, return values, old IDs/ABI and routing. No wrapper
switches an old entry point to the new reducer. No new fields in any existing
public state/options/style/request, bridge options/node/event, or `cui_theme`.

### Actual foreign-language surface

All five share [`cui_bridge.h`](../../include/cui/cui_bridge.h): node kind9,
`set_value/get_value` scalar double, `set_range(min,max,step)` domain. The name
`range` means numeric bounds, **not two selected values**. Current
[`cui_bridge.c`](../../src/bridge/cui_bridge.c):4847 converts to **float** and
calls legacy `cui_slider`, then stores accepted value and dispatches CHANGE.
Its range setter additionally bounds Slider min/max/step to ±16777216 limits;
step must be positive and min<max. Existing bridge events are value notifications,
not owner-controlled requests. A double setter is not proof of double reduction.

| Language | Current authority | Missing new behavior |
| --- | --- | --- |
| Python | [`cui_bridge/__init__.py`](../../bindings/python/cui_bridge/__init__.py): `slider`, Widget `range/value/current_value/on_change`; [`test_components.py`](../../bindings/python/test_components.py) | Pair, orientation, log, reverse, request acceptance and typed lifecycle |
| JS | [`index.cjs`](../../bindings/node/index.cjs): `slider`, Node `range/value/current_value/on_change`; [`components.test.cjs`](../../bindings/node/test/components.test.cjs) | Same; addon calls existing C scalar route |
| TS | [`index.d.ts`](../../bindings/typescript/index.d.ts): Slider extends NumericControl; `range` returns this; NumberInput also extends Slider | Same; do not accidentally add Slider-only modes to NumberInput through inheritance |
| Go | [`native.go`](../../bindings/go/cui/native.go): `Slider`, Node `Range/Value/OnChange`, App `Value`; [`native_test.go`](../../bindings/go/cui/native_test.go) | Same; configuration error latching must survive additive methods |
| Rust | [`lib.rs`](../../bindings/rust/src/lib.rs): Kind::Slider, App/Builder `range/value/on_change_event/current_value`; [`builders.rs`](../../bindings/rust/tests/builders.rs) | Same; Result/lifetime ownership must survive additive methods |

Existing value-notification-v1 APIs and tests in all runtimes remain scalar
notification paths. Do not stuff a range into their scalar field, reinterpret
CHANGE as a request, grow existing event ABI, or claim binding support from the
source SDK merely compiling a C resolver.

## Proposed additive portable contract (names pending review)

New `cui_slider_model.h` Base header/source, separate Styled facade header/source.
Core→Base→Components dependency stays unchanged; pure value/mapping helpers have
no theme/Clay/platform dependency. Use source-specific palette values only in
Styled presentation, privately mapping to the separately approved brush work.

```c
typedef enum { CUI_SLIDER_SINGLE, CUI_SLIDER_RANGE } cui_slider_value_kind;
typedef struct { cui_slider_value_kind kind; double start, end; } cui_slider_value;
typedef enum { CUI_SLIDER_LINEAR, CUI_SLIDER_LOGARITHMIC } cui_slider_scale;
typedef enum { CUI_SLIDER_THUMB_START, CUI_SLIDER_THUMB_END } cui_slider_thumb;
typedef struct { double minimum, maximum, step; cui_slider_scale scale; } cui_slider_domain;
/* New independent records, NOT fields added to old public structs. */
typedef struct cui_slider_model_state cui_slider_model_state;
typedef struct cui_slider_model_request cui_slider_model_request;
typedef struct cui_slider_model_options cui_slider_model_options;
```

The forward struct declarations above are design placeholders: implementation
must publish concrete fixed-size caller-owned C11 definitions before review,
never opaque allocated state. State fields: stable owner ID, captured target ID,
active thumb, owned start/preview pair, dragging flag, copied domain/mode/axis and
caller source-generation, last painted frame sequence, copied pointer identity.
A proposed concrete state shape, subject to the pre-implementation ABI review:

```c
typedef struct cui_slider_model_state {
    cui_id owner, captured_target;
    cui_slider_value start, preview;
    cui_slider_domain gesture_domain;
    cui_orientation gesture_orientation;
    cui_slider_thumb active_thumb;
    cui_pointer_type pointer_type;
    uint64_t pointer_id, pointer_device, source_generation, last_painted_frame;
    bool dragging;
} cui_slider_model_state;
```

Options: owned domain, orientation, disabled/readonly, source-generation, borrowed
name/start/end value descriptions, and ONE typed handler/user. No hidden registry,
heap, event retention, float downgrade or global keyboard dispatcher.

Values are exactly Single(end) or Range(start,end). Single ignores inactive start
and factories return start0; Range requires finite ordered `min<=start<=end<=max`.
Finite domain requires min<=max, finite span, finite step>=0; log additionally
min>0,max>min. Reject unknown enums/bad strings/state-ID mismatch atomically
before mutation, callbacks, semantics or boxes. No source panic/undefined math.
step0 means continuous; min==max is inert/cancels in Linear only. Bounds/config
setters validate complete new records transactionally, not builder call order.
Getters/factories leave outputs unchanged on failure; no API silently sorts pairs.

Suggested functions: pure value/domain validation, value↔percentage mapping and
quantization; `cui_base_slider_model_events` takes immutable value, explicit
state, options and handler; `cui_slider_model_events` adds presentation. Pure
`cui_slider_model_part_id(owner, typed_part)` returns stable root/track/fill/
start-thumb/end-thumb/ring IDs, never a presence assertion. Single start parts
are intentionally not emitted. Derived IDs are shared private helpers used by
painter/getter, checked nonzero/distinct before submission; owner0 fails only on
new APIs. Preserve all legacy salts/routes unchanged.

### Mapping, snapping, ownership and evidence

Use double arithmetic end-to-end. Linear maps `(v-min)/span`; vertical pointer
fraction `(bottom-y)/height`; horizontal `(x-left)/width`. Positive prior-layout
axis length required before starting pointer gestures; outside maps to endpoint.
Log mapping uses `exp(log(min)+p*(log(max)-log(min)))`, inverse log difference to
avoid overflowing `max/min` for otherwise finite inputs. Exact p0/p1 return
min/max. Tests independently quantify tolerance; no source-f32 bit identity.

Retain CUI **min-origin** snapping, exact max endpoint and final domain clamp.
Then clamp moved endpoint to the other preview endpoint; crossing meets but never
swaps thumb identity. Neighbor may be off-grid; this final bound wins over step.
This explicitly differs from source zero-origin snapping/possible overshoot.
No absolute/logarithmic step reinterpretation: step is additive value units in
both scales. step0 keyboard increment is span/100, preserving existing C policy.
The thumb and fill paint the percentage of the **snapped preview value**, not a
separately retained raw pointer percentage; CUI owner/paint coherence wins over
the source unsnapped-coordinate quirk. Reverse fills end→max only in Single;
Range always fills start→end. Reverse never changes numeric mapping or events.

Each call captures immutable confirmed pair/domain/options. Requests contain
metadata, existing BEGIN/UPDATE/COMMIT/CANCEL phases, active thumb, copied current
and requested pairs, reason and pointer-free existing scalar input evidence.
No CHANGE is fabricated. UPDATE may repeat the same proposed pair (source emits
unchanged Change too); COMMIT is gesture completion, never proof of acceptance.
Counters count actual handler invocations, saturate with overflow flag, and
metadata records actual event index/frame/source only when present. Callback may
accept/reject in external owner storage, observed next submission. It must not
mutate active state/options/theme, recursively submit, destroy UI/users, or change
callback lifetimes during the call. Two keys in one call propose from the same
entry pair, matching old controlled snapshots; pointer preview evolves in order.

While dragging paint owned preview. On completion/cancellation paint entry owner
pair, even if callback accepted; owner change appears next frame. Cancel offers
captured start pair, never writes owner. Missing callback is read-only. Source
Change/Release maps to richer REQUEST/COMMIT semantics explicitly, not renamed
legacy callbacks. New callback ABI is independent of old scalar callbacks.

### Local routing, focus, touch, cancellation and omission

Use existing Core targets/capture only. Root track semantic is a non-tab-stop
region/group with pointer activation, no pointer focus; enabled thumb children
are individually focusable role Slider. No parent/child duplicate reducer call.
Track down picks nearest **visual percentage**, exact midpoint END, range
coincident thumbs END; direct hit chooses that thumb. `cui_focus` selects chosen
thumb; capture remains original root or thumb target until gesture ends. Scan
only those validated targets and unconsumed events, once, recording original
input indices. No event requeue, target rewriting or new dispatch architecture.

Root and thumb hitboxes must be proven under actual completed layout and clipping;
Core topmost ordering determines direct-hit precedence. If the current local
composition cannot achieve track ownership plus child focus without retargeting,
stop for review: do not modify Core or claim it solved. Root-owned track drag is
allowed for Range as an explicit CUI usability extension; source range mouse
track itself only clicks. Single behavior stays source-aligned in the new route.

At most one active gesture per widget/Core capture. Lock chosen thumb and pointer
type/device/id through release; unrelated contact/device cannot switch/release
it. Touch drag priority uses existing semantics, respects modal/occlusion and
ancestor scroll claim. Preserve CUI generic/Escape cancel as CANCEL, including
touch cancel; source touch Cancelled→Release is a documented exception.

Arrows operate focused thumb (Left/Down decrease, Right/Up increase), Home/End
select min/max subject to neighbor; no thumb swap, hidden acceleration, global
Tab handling or scroll hijack. Native Tab focuses each emitted thumb using Core
order; coincide remains keyboard reachable. Single only END is a tab stop.
Disabled/read-only/zero-span/source-generation/domain/mode/axis change cancels
once before admitting a new gesture. Owner acceptance alone does not reset drag;
application increments explicit source-generation for source replacement.

Disabled or cancelled interaction resets thumb/ring activity targets; retained
source ring state must not leave an invisible pressed thumb active on re-enable.
Layout-only submits admissible layout/semantics but dispatches no input/callback,
starts no capture and mutates no gesture/preview/frame-tracking state. Omission
emits no immediate callback; Core clears missing semantic capture at end-frame.
On next painted submission, a gap in state's last-painted sequence cancels stale
preview once (reason OMITTED, no invented input), then uses current owner. A lost
capture without terminal input similarly cancels once. Duplicate painted
submission of same state/frame is rejected before dispatch. Disposal after return
may discard state silently; no fabricated callback into a dead user. A future
explicit lifecycle function must specify phase/ownership separately, not secretly
submit hidden semantics or rely on a new Core capture-cancel API.

### Semantics and accessibility boundary

Range root is a group, each thumb role Slider with parent root, meaningful
application name/description, normalized float numeric value (value-domain
fraction, not log screen fraction), normalized neighbor-constrained min/max and
orientation. Preserve precise value/units in caller value-description strings;
never claim float semantic payload carries full double precision. Disabled and
read-only state applies to root and thumbs; disabled presentation follows the
reviewed source/CUI exception policy without leaving enabled invisible targets.

Pure typed increment/decrement request commands for a chosen thumb can support
host adapters without keyboard fabrication, preserving pair mode and disabled
rules. However existing Core public accessibility actions only support
FOCUS/ACTIVATE. This slice cannot claim actual AT increment/decrement delivery.
Host/platform action integration is a **whole-family completion blocker** until
separately scoped/approved and verified; no Core action architecture change is
approved here. Source range→Single/disabled a11y quirks are deliberately rejected.

## All-five binding authoring and acceptance plan

Keep kind9/default `slider()` and all existing range/value/on_change paths
untouched. Add explicit `slider_model(...)` factory / opt-in typed model method
backed by independent versioned C bridge records and functions; do not infer the
new route from calling legacy `range`. Private node data may grow; public old
by-value records may not. New fixed-width bridge tags mirror SINGLE/RANGE,
LINEAR/LOGARITHMIC, horizontal/vertical, reverse-fill, thumb and phases; records
own two doubles, never host pointers or platform enums. Validate struct sizes and
all active fields atomically before enabling. Old get/set scalar APIs on an
opted-in model reject INVALID_STATE; they do not flatten end or silently disable
Range. New domain/value/options methods update complete records transactionally.

Language callbacks currently dispatch queued events **after** bridge step/run
frame processing (`bridge_change` queues; Python `dispatch`, JS `_dispatch`, Go
poll/dispatch and Rust equivalents invoke users later). Preserve that boundary.
Do not call Python/JS/Go/Rust request handlers during active Core reduction or
stepping. New bridge model handler only copies immutable request records into a
separate bounded typed request queue; old event/value-notification queues and ABI
stay untouched. Each node's reduction preflights enough free request slots for
an upper bound of three phases per targeted frame input plus one synthetic
cancel, with overflow-safe arithmetic. If insufficient, report CAPACITY before
that model's state/input/callback mutation; do not promise whole-frame rollback.
Expose exact queue capacity/overflow behavior and test independent queue fullness.
Allocation, if needed, occurs when enabling the new model outside a frame, not
inside reduction; no new fields in existing bridge creation options are required.

Polling returns immutable copyable typed requests including full node slot/
generation, configuration/source-generation, unique request sequence, frame and
phase evidence. Invalid size/output preserves outputs; no unbounded retained
payload. `accept_slider_request` takes its receipt and an explicit valid pair;
missing acceptance rejects by leaving owner unchanged. Acceptance is permitted
only outside active Core frame/step on owning UI thread (including the existing
after-frame dispatch phase), and changes the next submission's confirmed owner.
It never rewrites already emitted requests, current frame semantics or paint.

Receipt validity: same app, live full node generation, unchanged model/domain/
source generation, request sequence newer than last accepted request. Applying
one accepted request does not invalidate later requests from the same immutable
frame snapshot; direct owner replacement/configuration does invalidate older
receipts. This permits ordered UPDATE then COMMIT acceptance without accepting a
stale previous frame after a newer request. Closed/disposed/reused nodes reject
atomically; no stored callback pointer is dereferenced after disposal. These new
receipt and queue contracts need a separate concrete ABI checkpoint before code.
No old callback timing or accepted CHANGE behavior is modified.

| Runtime | Planned additive typed consumer API (final spelling reviewed together) |
| --- | --- |
| Python | Frozen/dataclass SliderValue/Domain/Options/Request, `slider_model`, `.slider_value`, `.slider_options`, `.on_slider_request`; event `.accept(value)` validates receipt through bridge. Optional symbol resolution: old library loads; only requesting new API reports capability unavailable. |
| JS | Owned validated value/options objects, `slider_model`, `.slider_value`, `.slider_options`, `.on_slider_request`; frozen request with explicit accept. Node addon methods feature-detected. No implicit conversion of JS array length other than exact pair; copied records outlive polling buffer. |
| TS | Discriminated `{kind:'single',end}` / `{kind:'range',start,end}` and dedicated SliderModel interface. Compile positive/negative cases and run JS implementation; remove accidental NumberInput inheritance exposure by adding a distinct new interface, without breaking existing declarations. |
| Go | Exported value/domain/options/request structs with typed enum constants; new factory/methods + `OnSliderRequest`, explicit App accept returning error. Preserve Node error latching/configuration ordering and optional dlsym behavior; no callback while cgo reducer active. |
| Rust | Owned enums/value/options/request records, factory/App/Builder methods returning Result, request observer and explicit accept; optional symbol lookup for dynamic old libraries. No references into bridge queue or callback reentrant borrow; validate stale handles/receipts before mutation. |

All wrappers must agree on finite validation, min-origin step, neighbor clamp,
source-generation and call ordering; none may pre-round to float or infer mode
from two values passed to the existing bounds method. An unavailable optional
symbol must never fall back to show-only/static or scalar behavior. Boundaries
between configuration, owner update, typed request and accepted notification
remain explicit. New rendering options must map to the actual new facade,
including axis/scale/reverse, not an unimplemented builder metadata cache.

## Headless proof and whole-family closure gates

No tests were executed for this design; no new binaries or build artifacts.
Implementation is gated by integrator review, concrete record/bridge receipt
contract, and one coordinated low-priority compiler slot. The following are
required evidence, not claims of completed support.

| Proof group | Required cases / independent oracle |
| --- | --- |
| Pure value math | Literal pinned defaults; Single vs exactly two Range; min/max/equal endpoints, neighbor bounds, off-grid neighbor; negative min/min-origin rounding, midpoint ties, exact max; continuous step0. Independent double reference using a different implementation, not copied production helper. Compare source cases separately and label deliberate differences. |
| Log | 1..1000 at0,1/3,2/3,1→1,10,100,1000; roundtrip/endpoints; asymmetric domains, huge finite ratio avoiding overflow, close positive endpoints, invalid min0/negative/max<=min, NaN/Inf/overflowing linear span. Step remains value units; no flattening to linear positions. |
| Range interaction | Both thumbs from direct hit and track midpoint/tie, no crossing, coincident keyboard reachability, preview other endpoint retained, accept/reject, ordered and unchanged UPDATE, exact phase counts and copied input evidence. Same-frame multiple keys use immutable pair; pointer evolves preview. No fabricated CHANGE/activation identity. |
| Axis/reverse | Wide/short and narrow/tall, both endpoint directions, prior-layout resize/scroll/clipping/modal occlusion, horizontal/vertical log with Range; reverse Single fill only and ignored for Range. Assert actual software pixels and semantic/part bounds, not only draw counts. |
| Pointer/touch | Primary vs secondary, touch type/device/id collision, second contact ignored, outside release, Escape/generic/touch cancel, ancestor scrolling vs drag priority, overlaid layer hit ownership, pointer leave vs cancel, missed end, omission/reappearance, capture stolen, disabled/readonly/source replacement mid-drag. Match existing Core routing without global edits. |
| Phase/lifetime | Normal vs layout-only/no-paint, duplicate submission, invalid arguments before any state/UI change, zero-span, missing handler, context/state ID mismatch, callback owner acceptance next frame, safe copied request after callback/frame, no stale stack references. New-state exact cancel reasons and no callback to disposed user. |
| Semantics | One or two role Slider thumb nodes, group parent, per-thumb names/descriptions, orientation, neighbor bounds, read-only/disabled, precise-unit text vs float normalization, focus order at coincident positions. AT increment/decrement delivery stays an explicit blocker until supported by approved host work. |
| Legacy regression | Existing float Base/Styled and controlled/event suites unchanged: IDs, width/layout, route, accepted values, high-double snapshots (1e10+.125), repeated-key behavior, callback counts/reasons, layout-only and invalid admission. Existing bridge/binding on_change scalar fixtures remain old paths. |
| Presentation integration | Four builtin skins and all38 independently verified source palettes; Aurora gradient actually painted, disabled omission, each thumb ring hover/pressed animation, track corner override, custom theme, brushes/alpha/clipping and real part styling. All38 source value checks alone do not establish rendered parity. |
| Bridge atomicity | Exact record sizes/tags and bad active fields; queue capacity/overflow preflight, unchanged outputs on failure, correct copy lifetime, wrong kind/thread/active-frame calls, no scalar flattening. Receipt node reuse/app/config/source/sequence stale failures; multiple same-frame accepts ordered correctly. Old library optional-symbol loads unchanged. |
| Five actual runtimes | Python, JS, TS compile **and runtime**, Go, Rust configure and query Single/Range/axis/log/reverse, drive real queued pointer/key/touch, reject/accept after-frame request, verify next-frame actual pair/paint/bounds and semantic snapshots. Prove requested builder fields reach facade, callback time outside step, invalid/stale receipts and disposed handles. Source SDK snapshots alone are not binding proof. |
| Browser/native/distribution | Headless browser uses actual combined WASM gallery with interactive two-thumb/vertical/log stories and copyable compiled C snippets; no dummy endpoints. Native Linux runs only `scripts/test_local.py --build-dir build-focused` private Xvfb. Portable strict C11 selective and umbrella headers plus Base-only dependency checks. No desktop/GPU tests; untested hardware/OS/AT explicitly deferred. |

Update component-usage metadata **and exact mirrored COMPONENT_USAGE guide text**
together and run `tests/test_component_usage.py`. Update family/feature ledger,
actual binding coverage, source list, generated dist/SDK/skill resources and skill
checks; keep all78 entries. Compile/test source revision must match the delivered
artifact; use external signed receipts for command, source hashes and proof.

The smallest coherent implementation order is: (1) reviewed pure value/domain
math and explicit state contract; (2) new Base reducer+local composition/focus and
all C interaction/semantics cases; (3) source-faithful Styled presentation using
owned palette/approved background path; (4) versioned bridge request/acceptance
and all five real builders; (5) actual WASM/native/installed distribution proof
and separately approved accessibility action delivery. Each stage is partial.
Full family status must remain open until the union covers source Single/Range,
vertical/log/reverse, disabled/themes/style/rings, touch, events and semantics,
all promised bindings and real host proof, with every deliberate exception
recorded. Unavailable AT actions or gradient rendering cannot be hidden behind
a narrower new-feature completion label.

Signed: `/root/hidden_windows`, 2026-10-31. Design checkpoint for integrator review;
no production/gen/build changes, no merge/push.
