# Typed Slider model v1: concrete record and request contract

Signed design checkpoint following `502f1e22`, numeric-action design `df63de89`
and readiness revision `0e7c2bf9`. No model/host/bridge/binding implementation is
authorized here. The whole Slider family remains open. Source cardinality is
Single or exactly two ordered Range values, never arbitrary N thumbs; pinned
source hashes/quirks are recorded in [the source audit](SLIDER_RANGE_ORIENTATION_SCALE.md).

## New records, exact version and ownership rules

Proposed new public header `cui_slider_model.h`, portable C11/Core→Base only;
Styled presentation uses a separate facade header and reviewed owned palette.
No existing state/options/request/result, theme, event or bridge struct grows.
The following are new **v1** types. Record scalar tags use uint32_t rather than
ABI-sized enum fields; named enum constants provide the authoring vocabulary.
Domain/value math remains double; input coordinates retain actual Core floats.

```c
#define CUI_SLIDER_MODEL_V1 1u
enum { CUI_SLIDER_MODEL_SINGLE=0, CUI_SLIDER_MODEL_RANGE=1 };
enum { CUI_SLIDER_MODEL_LINEAR=0, CUI_SLIDER_MODEL_LOGARITHMIC=1 };
enum { CUI_SLIDER_MODEL_HORIZONTAL=0, CUI_SLIDER_MODEL_VERTICAL=1 };
enum { CUI_SLIDER_MODEL_START=0, CUI_SLIDER_MODEL_END=1 };
enum { CUI_SLIDER_MODEL_BEGIN=0, CUI_SLIDER_MODEL_UPDATE=1,
       CUI_SLIDER_MODEL_COMMIT=2, CUI_SLIDER_MODEL_CANCEL=3 };
enum { CUI_SLIDER_MODEL_INPUT=0, CUI_SLIDER_MODEL_DISABLED=1,
       CUI_SLIDER_MODEL_READONLY=2, CUI_SLIDER_MODEL_ZERO_SPAN=3,
       CUI_SLIDER_MODEL_SOURCE_REPLACED=4, CUI_SLIDER_MODEL_DOMAIN_CHANGED=5,
       CUI_SLIDER_MODEL_MODE_CHANGED=6, CUI_SLIDER_MODEL_AXIS_CHANGED=7,
       CUI_SLIDER_MODEL_OMITTED=8, CUI_SLIDER_MODEL_CAPTURE_LOST=9 };
typedef struct {
    uint32_t kind, reserved; /* reserved0 */
    double start, end;
} cui_slider_model_value_v1;
typedef struct {
    double minimum, maximum, step;
    uint32_t scale, reserved; /* reserved0 */
} cui_slider_model_domain_v1;
enum { CUI_SLIDER_MODEL_DISABLED_BIT=1u, CUI_SLIDER_MODEL_READONLY_BIT=2u,
       CUI_SLIDER_MODEL_REVERSE_FILL_BIT=4u };
typedef struct {
    uint32_t version, struct_size, flags, orientation;
    uint64_t source_generation;
    cui_slider_model_domain_v1 domain;
    cui_str name, start_name, end_name; /* empty thumb name falls back to name */
    cui_str start_description, end_description; /* borrowed through call */
} cui_slider_model_options_v1;
```

SINGLE validates end only; inactive start is ignored (including NaN). Factories,
requests and stored state canonicalize Single start0 and reserved fields0. RANGE
requires both finite and `minimum<=start<=end<=maximum`; never sort or scalarize.
Domain requires finite min/max/step, min<=max, finite span, step>=0; Linear permits
zero span (inert); Logarithmic additionally min>0,max>min. Unknown tags/nonzero
reserved/unknown option bits fail. Source-generation is opaque caller-owned64-bit
identity; equal means same source, no arithmetic ordering/wrap inference.

Versioned top-level records require version1 and **exact** sizeof(v1), not a
minimum-size guess. Value/domain nested records inherit their containing v1
schema and carry only their reserved fields. New versions use new types/symbols;
no silently accepted larger tails. Null descriptions require length0; names and
descriptions are borrowed only synchronously and copied by Core where necessary.
Options are read-only through return; snapshot their fields before callbacks.
Root label uses name; each thumb uses its explicit name or the root-name fallback,
and its own precise value description. Caller supplies meaningful endpoint names;
no hidden English labels or invented accessibility units.

### Explicit caller-owned state, no allocation or opaque layout copy

```c
enum { CUI_SLIDER_MODEL_DRAGGING=1u };
typedef struct {
    uint32_t version, struct_size, flags, active_thumb;
    cui_id owner, captured_target;
    cui_slider_model_value_v1 start, preview;
    cui_slider_model_domain_v1 gesture_domain;
    uint32_t gesture_orientation, pointer_type;
    uint64_t pointer_id, pointer_device, source_generation;
    uint64_t last_painted_frame, request_sequence;
} cui_slider_model_state_v1;
bool cui_slider_model_state_init_v1(uint32_t state_size,
                                      cui_slider_model_state_v1 *out);
```

Init accepts exact state size/non-null output, writes fully canonical owned
state (version1, sizeof, no owner/drag/sequence). Invalid init leaves output
unchanged. State itself has no pointers/heap/borrowed lifetime; one state per
stable nonzero owner and context. Caller owns storage but must not mutate active
fields or reuse state with a different context/owner. Existing state has no
context nonce: a newly created context requires freshly initialized state;
frame sequence comparison is not cross-context identity proof.

Only dragging-state capture fields are active; validate finite start/preview,
gesture-domain/mode/orientation and pointer tags before use. Owner0 binds on
first valid painted submission; a different nonzero owner fails atomically.
Sequence is monotonic per state, never silently wraps. Presentation fields are
not confirmation of accepted owner values. Reinitialization after disposal is
explicit and must occur outside callback/submission, after Core no longer uses
that state; no implicit destructor callbacks.

## Pointer-free input, request and result records

```c
enum { CUI_SLIDER_MODEL_HAS_INPUT=1u, CUI_SLIDER_MODEL_HAS_NUMERIC_VALUE=2u };
typedef struct {
    uint32_t type, flags, key, modifiers;
    float x, y;
    uint32_t repeat, pointer_type, pointer_button, accessibility_action;
    uint64_t pointer_id, pointer_device;
    cui_id accessibility_target;
    double accessibility_value; /* only HAS_NUMERIC_VALUE */
} cui_slider_model_input_v1;
typedef struct {
    uint32_t version, struct_size, phase, reason;
    uint32_t thumb, reserved[3];
    cui_component_event_metadata event;
    uint64_t submission_frame, request_sequence, source_generation;
    cui_slider_model_value_v1 current, requested;
    cui_slider_model_domain_v1 domain, gesture_domain;
    cui_slider_model_input_v1 input;
} cui_slider_model_request_v1;
typedef void (*cui_slider_model_request_fn_v1)(void *user,
                                               const cui_slider_model_request_v1 *request);
typedef struct {
    cui_slider_model_request_fn_v1 callback;
    void *user; /* borrowed through submission */
} cui_slider_model_handler_v1;
enum { CUI_SLIDER_MODEL_OK=0, CUI_SLIDER_MODEL_INVALID_ARGUMENT=1,
       CUI_SLIDER_MODEL_INVALID_STATE=2, CUI_SLIDER_MODEL_SEQUENCE_EXHAUSTED=3,
       CUI_SLIDER_MODEL_CORE_FAILURE=4 };
enum { CUI_SLIDER_MODEL_VALID=1u, CUI_SLIDER_MODEL_SUBMITTED=2u,
       CUI_SLIDER_MODEL_COUNT_OVERFLOW=4u };
typedef struct {
    uint32_t version, struct_size, status, flags;
    uint64_t submission_frame;
    uint64_t begin_count, request_count, commit_count, cancel_count;
} cui_slider_model_result_v1;
cui_slider_model_result_v1 cui_base_slider_model_events_v1(
    cui_context *, cui_id owner, cui_slider_model_value_v1 confirmed,
    cui_slider_model_state_v1 *, const cui_slider_model_options_v1 *,
    cui_slider_model_handler_v1);
```

Header includes exact Core/component-metadata authorities, not duplicated private
layouts. `cui_component_event_metadata` is pointer-free but includes ABI-size
size_t and an inactive activation snapshot; it is valid portable C, **not a raw
FFI wire record**. Never memcpy it into fixed bridge records. Requests zero all
reserved/inactive fields. Input fields are actual input only: pointer/key fields
zero for accessibility; no invented activation, text, composition or pointer.

Request current is the immutable confirmed entry pair for **every** phase in
that call, even if callback changes external owner storage. Requested is phase's
proposed pair. Domain is entry domain. Gesture-domain records the captured
pointer-start domain for cancellation audit; for non-drag keyboard/a11y commands
it equals entry domain. Synthetic cancel after domain/mode replacement may offer
captured start outside current domain; it is restoration intent, not automatic
permission to store an invalid new owner. Explicit acceptance validates current
owner configuration. `thumb` always identifies affected START/END; SINGLE uses END.

Metadata kinds: UPDATE→REQUEST, COMMIT→COMMIT, BEGIN/CANCEL→LIFECYCLE; never CHANGE.
Actual event index/frame/source copied from original targeted input. Numeric
accessibility uses approved typed Core claim, ACCESSIBILITY source and explicit
action/target/raw double; no HAS_ACTIVATION. Option/omission/capture-loss cancel
has no input and unknown physical source, no invented index/activation. Separate
submission_frame is actual painted call identity; it does not manufacture input
evidence. A copied request remains usable after callback/frame/context destruction;
its pointers are only in the separately borrowed handler/options, not the request.

Return result always has canonical version/size and initialized status/flags.
Counts mean actual callback invocations, not physical events, accepted values or
consumption. They are uint64, saturate with COUNT_OVERFLOW if necessary; a callback
is never skipped merely to hide overflow. No-handler produces no requests/counts
and acts read-only. Options do not contain a second legacy callback path; new and
old callbacks cannot be combined on this new route.

## Mapping, capture, ordering and aggregation

Pure helpers validate/constrain/map values with double outputs and atomic failure.
Linear mapping uses value-domain fraction; vertical increases bottom→top. Log
visual fraction uses log differences and stable exp interpolation, exact endpoints,
not overflowing max/min. Keyboard and a11y increment/decrement stay additive in
value units; continuous step0 uses span/100. SET_VALUE is published semantic
value-domain normalization, **not** inverse log screen position; convert incoming
semantic double, snap and clamp. Preserve raw incoming semantic value in input.

Min-origin round-to-nearest with halfway away from zero matches existing CUI
controlled step policy, followed by domain and neighbor clamp; exact max is
reachable. Off-grid neighbor may win over the lattice. Never cross/swap thumbs.
At bounds a request may repeat unchanged pair. Reverse affects Single fill only,
ignored for Range; it changes no axis/value/request behavior. Display snapped
preview position rather than source unsnapped-coordinate quirk. These are explicit
CUI exceptions, not source-f32 bit parity claims.

| Input / transition | Exact ordered callback emissions |
| --- | --- |
| Pointer down on admissible prior-layout root/thumb | BEGIN(entry), UPDATE(pointer proposal); captures chosen thumb/pointer and starting pair |
| Pointer move while dragging | UPDATE(evolving preview), even if same snapped value |
| Pointer up while dragging | UPDATE(final pointer), COMMIT(final preview); no retained dragging |
| Down+up in one frame | BEGIN, UPDATE, UPDATE, COMMIT, in original event order |
| Up without active gesture | none |
| Escape/generic/touch cancellation | CANCEL(captured start), no COMMIT; copied actual input where present |
| Each supported key or claimed numeric a11y action while not dragging | BEGIN(entry), UPDATE(proposal from entry pair), COMMIT(same proposal) |
| Two supported keys / two a11y actions in one call | Two independent three-phase groups; both proposals use immutable entry owner pair, not first callback's acceptance |
| Disabled/readonly/zero-span/source/domain/mode/axis change while dragging | One synthetic CANCEL before later admissible input; deterministic reason priority listed below |
| Missing callback | Read-only cleanup; no invocation into absent user; zero counts |

Changing reverse-fill alone does not cancel (presentation only). Domain equality
compares actual fields, not struct padding or memcmp.

Cancellation priority if several transitions apply: DISABLED, READONLY, ZERO_SPAN,
SOURCE_REPLACED, MODE_CHANGED, DOMAIN_CHANGED, AXIS_CHANGED, OMITTED, CAPTURE_LOST.
Do not emit several cancels for one drag. Owner acceptance changing confirmed value
alone is not source replacement; explicit generation marks external replacement.
A cancellation before later input permits a fresh gesture only if current options
are writable/valid and input is an actual new eligible down/command.

While dragging, paint preview only. Otherwise paint entry confirmed pair, including
after same-call accept/release/cancel; next submission observes owner acceptance.
No per-frame coalescing of typed requests: each actual emission counts individually.
Old scalar bridge CHANGE remains its existing aggregate route, never reinterpreted.

Root track pointer target chooses nearest visual thumb, exact midpoint END; direct
thumb hit selects that thumb. Core focus/capture/target/consumption remain sole
routing authority. One gesture locks pointer type/device/contact and thumb; second
contact cannot hijack. Existing Core touch drag priority/occlusion/modal rules apply.
New local composition must prove child focus plus original track capture without
retarget/requeue/global dispatcher; otherwise stop for review. Focused-thumb arrows
Left/Down decrease, Right/Up increase; Home/End min/max bounded by neighbor. Core
Tab remains unchanged. Current a11y capability is explicit per thumb, not inferred
from role or presence; old scalar reducers ignore new actions and never opt in.

Layout-only may submit valid layout/semantics but mutates no model/capture/sequence
and dispatches no requests. Omission gives no immediate callback; next painted
submission detects last-frame gap and cancels stale preview once, without replaying
omitted input. Context-level readiness follows approved Core predicate; a known
invalid snapshot/error forbids new claims/requests. Already delivered callbacks
are not rolled back after a later layout/Core failure.

## Atomic admission, bounds and borrowed lifetimes

Before boxes/semantics/interaction/callbacks: validate all top-level versions/sizes,
options/values/domain/state/owner/frame/lifetime/string/enums, active capture data
and request-sequence budget. Reject invalid inputs with canonical return status,
zero submitted/counts, unchanged model state/owner/callbacks and no UI submission.
INVALID_ARGUMENT covers malformed versions/tags/values/strings; INVALID_STATE
covers wrong frame, owner/state reuse or duplicate submission; SEQUENCE_EXHAUSTED
covers preflight sequence budget. All return counts0 and no SUBMITTED flag.
A valid layout-only call is VALID|SUBMITTED with frame0/counts0 and unchanged state.
The owner value is passed by value and never written. Validity does not claim
painted parts exist. Pure factories/getters preserve output bytes on failure.

One painted submission per owner/state/frame; duplicate rejected before dispatch.
Preflight request-sequence capacity using an overflow-safe conservative bound of
three emissions per frame input plus one synthetic cancel (zero budget when no
handler can be invoked); reject the entire
submission if remaining uint64 sequence space is insufficient. This prevents
half-emitted gesture due to sequence wrap; excess conservative reservation does
not increment unused sequence numbers. Increment sequence exactly once per actual
callback. Frame IDs use Core identities, not caller-supplied guesses.

Argument admission is transactional; do not promise whole-frame rollback on a
later Core allocation/layout/semantic capacity failure after submission has begun.
Return CORE_FAILURE, preserve accurate counts already invoked, and stop further
requests once error is known. Already copied request/owner effects are not undone.
Any need for stronger capacity reservation requires a separate approved Core API,
not a false preflight guarantee. No heap allocation in model state/reducer itself.

Handler/function/user and strings remain alive through return. Callback can only
accept/reject externally or enqueue application work for later composition; it
must not destroy/mutate current UI/state/theme/options/users, recursively submit,
or alter borrowed buffers mid-call. No callback retained by model. State/request
copies contain owned numeric fields only. End-frame/renderer borrowed-storage
lifetime requirements of Styled presentation remain separately documented.

## Future bridge receipt/request ABI: exact mapping, no scalar fallback

This is a separately reviewable proposed ABI, not authorization to implement it.
The bridge header stays independent of Base headers. Define its own fixed-width
public mirrors; use explicit field mapping, never raw C model struct memcpy.
Natural C ABI alignment applies (not packed/network wire); exact struct-size
checks use caller/library matching ABI and version, including32-bit alignment.

```c
typedef struct { uint32_t kind, reserved; double start, end; }
    cui_bridge_slider_value_v1;
typedef struct { double minimum, maximum, step; uint32_t scale, reserved; }
    cui_bridge_slider_domain_v1;
typedef struct {
    uint32_t version, struct_size;
    uint64_t app_epoch;
    cui_bridge_node node; /* full slot+generation, not slot only */
    uint64_t source_generation, configuration_revision;
    uint64_t submission_frame, request_sequence;
} cui_bridge_slider_receipt_v1;
typedef struct {
    uint32_t type, flags, key, modifiers;
    float x, y;
    uint32_t repeat, pointer_type, pointer_button, accessibility_action;
    uint64_t pointer_id, pointer_device, accessibility_target;
    double accessibility_value;
} cui_bridge_slider_input_v1;
typedef struct {
    uint32_t version, struct_size, phase, reason;
    uint32_t thumb, event_kind, event_source, event_flags;
    cui_bridge_slider_receipt_v1 receipt;
    uint64_t event_target, event_frame, event_index;
    cui_bridge_slider_value_v1 current, requested;
    cui_bridge_slider_domain_v1 domain, gesture_domain;
    cui_bridge_slider_input_v1 input;
} cui_bridge_slider_request_v1;
```

Bridge version1 tags are explicit numeric constants corresponding to the C v1
vocabulary; do not reuse existing scalar notification payload tags or add fields
to old event structs. Model metadata→bridge mapping:

| C model authority | Bridge field / rule |
| --- | --- |
| Request version/sizeof/phase/reason/thumb | version/struct_size and explicit fixed tag mapping; reject unsupported schema rather than reinterpret |
| Call node | receipt full node slot+generation, owning app epoch; semantic event_target remains distinct from language node handle |
| options.source_generation | receipt.source_generation copied unchanged64-bit |
| Immutable node configuration revision | receipt.configuration_revision; domain/mode/axis/source/readonly policy or explicit owner replacement invalidates outstanding receipts |
| request.submission_frame / sequence | receipt.submission_frame / request_sequence; sequence never wraps/reuses under same live node generation |
| event.kind/source/flags | event_kind/source/flags map known fields; numeric route never HAS_ACTIVATION. Unknown input source stays unknown, not keyboard |
| event.target | event_target exact actual thumb/root semantic ID; wrappers do not cast it to node handle |
| event frame/index | event_frame/index only with presence flags; absent canonical0/UINT64_MAX. size_t index widens with checked conversion, never float/JS Number identity |
| current/requested | Both complete value records, kind preserved; no storage in legacy scalar value or “end only” shortcut |
| domain/gesture_domain | Both double records copied at emission, not re-read after later setter |
| input | Copy pointer-free actual fields, incl raw SET semantic double and target/action; absent/inactive fields zero |

App epoch is a nonzero opaque process-local app identity, never a pointer value.
Its generation/overflow mechanism requires separate concrete bridge review; no
unbounded receipt registry or hidden lifetime borrowed from a polling buffer.
Node generation, app epoch, semantic ID and frame/sequence are different identity
spaces. Epoch/sequence exhaustion fails rather than reuses an identity.

Proposed entry points: new versioned model config/value getter/setter, dedicated
request poll and explicit accept. Every new record takes exact struct-size/version
checks; invalid output pointers/sizes preserve outputs/queue. Poll copies owned
record, so wrappers can keep it after later polling/disposal; copied readability
is not perpetual acceptance authority. No old get/set_value, bounds `range`,
CHANGE callback or notification-v1 payload is overloaded with a pair.

### Receipt acceptance window and capacity

A receipt is acceptable only on owning UI thread outside active frame/step,
including existing after-frame callback dispatch, with matching app epoch, live
full node generation, model configuration/source generation and **latest painted
submission frame**. The next painted model submission expires prior receipts,
even when no newer value was accepted. Layout-only does not renew/expire a painted
request window. This makes delayed manual poll copies readable but stale for
acceptance after the app advances that model. No implicit replay is attempted.

Within that same submission frame, request sequence must exceed last accepted
sequence. All BEGIN/UPDATE/COMMIT records from one frame remain individually
eligible in order; a later COMMIT being queued does not invalidate an earlier
UPDATE before dispatch. Accepting one request changes owner only and advances
last-accepted sequence; it does not increment configuration revision or expire
later same-frame requests. Direct owner replacement/config/source changes do
increment revision and invalidate old receipts. Explicit accept value may differ
from proposal but must validate complete current pair/domain. CANCEL's captured
start outside new domain cannot be accepted without valid owner adjustment.

Default is reject-by-inaction. No success is inferred from COMMIT or input consume.
No old CHANGE notification synthesized for a proposed/accepted pair; any future
accepted-value notification requires its own explicit typed producer contract.
Disposed/removed/reused node, foreign app, wrong thread/phase, stale receipt,
invalid pair and unavailable symbol return status without owner/queue mutation.
The UI frame already painted never changes retroactively.

A bounded dedicated bridge request queue stores copied records. Before model
reduction, bridge preflights overflow-safe3*targeted_input_count+1 free slots
(or a documented conservative larger upper bound) so capacity failure precedes
that node's reduction/callback/consumption. No whole-frame rollback claim. Missing
observer config makes new model read-only; never silently auto-accept. Queue
capacity and poll/accept error behavior require independent bridge ABI approval.

### All-five concrete consumer mapping

| Consumer | Proposed owned value/request and acceptance mapping |
| --- | --- |
| Python | Frozen value/domain/input/request/receipt dataclasses; SINGLE/RANGE tagged values; Python int for all64-bit identities, Python float for actual double values. Exact pair validation and `.accept(value)` call bridge; optional symbol resolution preserves old library loads. No ctypes pointer survives poll. |
| JS | Frozen discriminated value/request/input objects; bigint for epoch/generation/frame/sequence/event-index/semantic IDs, number for finite double values. Explicit `.accept(value)`; addon maps records field-by-field and rejects unsafe Number IDs rather than rounding. No JSON stringify roundtrip for bigints or payload transport. |
| TS | Dedicated SliderModel/SliderRequest discriminated interfaces matching JS bigint identities and number values; SINGLE has end, RANGE start/end. Positive/negative compilation plus actual JS runtime proof, not declarations alone; do not extend NumberInput through legacy Slider inheritance. |
| Go | Exported owned value/domain/input/request/receipt structs with uint32 tags,uint64 IDs,float64 values; explicit App.AcceptSliderRequest returns error. Copy cgo fields before return, preserve error latching/config ordering; no pointer into queue or callback invoked while cgo/Core stepping. |
| Rust | Owned enums/records with f64 values,u64 identities and explicit Result accept; borrowed callback receives immutable owned copy, no RefCell reentrant borrow during native reducer. Optional dynamic symbol handling; static builds require matching generated SDK. |

Wrappers invoke request observers only after step/end-frame, as existing binding
callback phase requires. They must not silently accept, call old scalar Value
setter, narrow to float32, use numeric domain bounds as Range selected pair, or
promote proposed requests to accepted CHANGE. All five must expose Single/Range,
orientation/log/reverse/domain/value separately and reach actual new C facade.
Native host SET delivery and numeric-enabled backend options remain separately
reviewed; a bridge request helper is not evidence of actual platform AT success.

## Review gates, deterministic evidence and compatibility omissions

Required offline/headless contracts before implementation approval/completion:

- Public records compile strict C11 selective/umbrella with exact sizeof/offset
  probes on available ABIs. Version/size/reserved/enum/active-field rejection leaves
  state/output/queue unchanged; canonical inactive fields, pointer-free copy lifetime
  and no existing public ABI growth checked. State size/ownership is explicit.
- Independent domain/math oracle covers all source cardinality/axes/log endpoints,
  min-origin step vs documented zero-origin source difference, off-grid neighbor,
  extreme finite doubles and high offsets. No roundtrip oracle copied from helpers.
- Phase table asserted exactly with actual queued pointer/key/touch/a11y input:
  unchanged repeated requests, same-call frozen pair, four callbacks for click,
  six for two keys, cancel priority, no-handler read-only, omission/layout-only,
  acceptance on next frame, frame/sequence budget and duplicate submission.
- Copied request metadata remains valid after callback/frame/context; actual numeric
  SET raw semantic value remains double and not an invented activation. Core
  readiness/new-layout failure interoperability matches approved Core slice; no
  replay or rollback claims after later error.
- Future bridge tests fill request queue, reject wrong phase/app/node/config/source/
  frame/sequence receipts, accept ordered same-frame UPDATE→COMMIT, expire on next
  painted submission and preserve layout-only windows. All five runtime consumers
  preserve Range pairs and64-bit identities; TS compile+runtime, optional old-library
  loading, installed SDK revision checks and real native host tests stay distinct.
- Any changed component-usage JSON is mirrored EXACTLY in COMPONENT_USAGE guide
  and checked by test_component_usage. Source metadata, binding coverage, generated
  dist/SDK/skill and all78 entries remain required; no narrower success label closes
  whole Slider family.

Explicit omissions: no arbitrary-N thumbs, thumb swapping, source invalid-domain
panic behavior, unsnapped display percentage, source a11y Range→Single mutation,
legacy scalar ABI migration, automatic accepted CHANGE, generalized event bus,
Core input dispatcher rewrite, renderer architecture, implicit native backend
creation, pinned dependency patch, browser/physical-AT proof or all-five production
in this contract. Actual source presentation (including disabled omission/rings),
owned brush rendering and installed native host delivery remain independent gates.

Signed: `/root/hidden_windows`, 2026-10-31. Design-only checkpoint; no production,
generated files, compiler, dependencies, merge or push.
