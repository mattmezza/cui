# Controlled typed Slider bridge v1: exact proposal

Design only, `/root/hidden_windows`, 2026-11-01. Based on integrated
`259e298a2c65ad5d1554e04360c7b74ac437d58f`, including the new typed gallery.
No production declarations/implementation, compilation, dependency or generation
changes here. This makes the future-bridge section of
[SLIDER_MODEL_ABI_V1.md](SLIDER_MODEL_ABI_V1.md) concrete; its b24 immutable-current /
evolving-requested and explicit acceptance resolution remains authoritative.
Root decisions below precede implementation; this is not an implemented API.

## Existing authority and preservation boundary

- `include/cui/cui_bridge.h`: ABI_VERSION1, exact-size `cui_bridge_options`,
  node `{uint32_t slot,generation}`, old scalar event/ex/notification layouts.
  Keep every old struct, enum value, exported signature and constructor unchanged.
- `src/bridge/cui_bridge.c`: `bridge_ready` checks null/closed/stepping, not a
  portable thread identity. `bridge_node` checks full live generation.
  `cui_bridge_node_create` retires generation UINT32_MAX slots and stops ID
  allocation at UINT64_MAX; failed creation can consume an unpublished internal
  ID. Preserve that monotonic/no-reuse policy, not fabricated identity reuse.
- Legacy Slider render branch casts node doubles to float, calls `cui_slider`,
  writes its result back and enqueues CHANGE. `set_range` requires min<max,
  step>0 and Slider bounds/step within its existing float-safe range. Keep this
  exact route, callbacks, precision, limits, autoaccept and notifications.
- `bridge_change` / `poll_event_ex` / notification views share the existing FIFO.
  New proposals must never enter it, downgrade to LEGACY, or be drained by old
  polling. New typed requests are not scalar accepted-value notifications.
  Existing shadow target contracts remain unchanged for legacy Slider. New kind81
  has no implicit shadow setter support: initially reject those authoring calls
  explicitly; named typed root/track/thumb shadow authoring needs its own facade
  mapping review, not salt reuse or a false THUMB capability. This omission must
  appear in all-five guidance until implemented.
- Actual model source: `src/base/cui_base_slider_model.c` validation reserves
  sequence space with **all** current frame events, `3*count+1`; it does not
  expose a target-only capacity reservation. Use that conservative bound.
- `web/gallery.c` now demonstrates synchronous C model requests, external owner
  acceptance and passive wakes. Bridge requests instead cross an owned queue and
  callbacks happen after step. Do not claim their acceptance repaints that frame.

## New kind and exact records

Propose append `CUI_BRIDGE_SLIDER_MODEL = 81`, a versioned authoring route for the
same Slider family, not an additional catalog family. All78 stay intact. A new
factory below creates it; old `slider()` builders still create kind9. Generic
`node_create` with81 rejects INVALID_STATE and directs callers to the factory
until valid config/value can be supplied atomically. Common Row/Column composition,
label text and disabled policy are supported; legacy scalar value/range APIs
reject kind81. No hidden upgrade of mounted kind9 nodes.

Public header remains independent of Core/Base/Styled headers. All tags are
uint32_t fields and all identities uint64_t; enums below name fixed numbers.
Order new record headers `struct_size,schema_version`, matching current bridge
V1 conventions (the older design's version/size order is not a shipped ABI).
Natural C ABI, no packing, pointers, unions, size_t, platform or Clay types:

```c
#define CUI_BRIDGE_SLIDER_MODEL_V1 1u
/* Value kind SINGLE0/RANGE1; scale LINEAR0/LOGARITHMIC1; axis H0/V1. */
typedef struct { uint32_t kind, reserved; double start, end; }
    cui_bridge_slider_value_v1;
typedef struct { double minimum, maximum, step; uint32_t scale, reserved; }
    cui_bridge_slider_domain_v1;
typedef struct {
    uint32_t struct_size, schema_version, flags, orientation;
    uint64_t source_generation;
    cui_bridge_slider_domain_v1 domain;
    float rem, length;
    uint32_t layout, reserved;
} cui_bridge_slider_config_v1;
/* flags: REQUESTS1, READONLY2, REVERSE_FILL4, REDUCED_MOTION8, HIDE_FOCUS16.
 * layout: FIXED0 / PARENT1. Node disabled is the existing common policy. */
typedef struct {
    uint32_t struct_size, schema_version, render_status, reserved;
    double core_time, next_wake_seconds;
} cui_bridge_slider_presentation_v1;
typedef struct {
    uint32_t struct_size, schema_version, capacity, reserved;
} cui_bridge_slider_queue_options_v1;
typedef struct {
    uint32_t struct_size, schema_version;
    uint64_t app_epoch;
    cui_bridge_node node;
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
    uint32_t struct_size, schema_version, phase, reason;
    uint32_t thumb, event_kind, event_source, event_flags;
    cui_bridge_slider_receipt_v1 receipt;
    uint64_t event_target, event_frame, event_index;
    cui_bridge_slider_value_v1 current, requested;
    cui_bridge_slider_domain_v1 domain, gesture_domain;
    cui_bridge_slider_input_v1 input;
} cui_bridge_slider_request_v1;
```

Expected sizes from field-layout arithmetic (not compiler proof at this static
checkpoint): LP64 with 8-aligned double/u64, i386-style ILP32 with4 alignment, and
wasm32 with8 alignment all give the same sizes/offsets, because preceding groups
are deliberately multiples of8. Production must prove native and actual pinned
wasm32 `sizeof/offsetof` with strict fixtures; no claim all ILP32 ABIs measured.

| Record | Bytes on these three models | Important offsets |
| --- | ---: | --- |
| value |24|kind0,reserved4,start8,end16|
| domain |32|min0,max8,step16,scale24,reserved28|
| config |72|flags8,axis12,source16,domain24,rem56,length60,layout64,reserved68|
| queue options |16|capacity8,reserved12|
| presentation |32|render_status8,reserved12,Core time16,wake24|
| receipt |56|epoch8,node16,source24,revision32,frame40,sequence48|
| input |72|type0,flags4,key8,modifiers12,x16,y20,repeat24,type28,button32,action36,id40,device48,target56,value64|
| request |296|phase8,reason12,thumb16,kind20,source24,flags28,receipt32,target88,frame96,index104,current112,requested136,domain160,gesture192,input224|

Size query functions below return library sizes, not permission to truncate or
reinterpret records on another ABI. Fixed nested records have no separate schema
header; their authority is the containing v1 schema and explicit size argument.

## Exact exported signatures proposed

Every declaration below would carry CUI_BRIDGE_API and return existing status.
No C foreign callback registration is added. The Base handler is a private
bridge queue producer, not a language trampoline during Core composition.

```c
uint32_t cui_bridge_slider_model_versions(void); /* bit0 means complete v1 */
uint32_t cui_bridge_slider_config_v1_size(void);
uint32_t cui_bridge_slider_value_v1_size(void);
uint32_t cui_bridge_slider_receipt_v1_size(void);
uint32_t cui_bridge_slider_request_v1_size(void);
uint32_t cui_bridge_slider_presentation_v1_size(void);
cui_bridge_status cui_bridge_slider_queue_enable_v1(cui_bridge_app *,
    uint32_t options_size, const cui_bridge_slider_queue_options_v1 *);
cui_bridge_status cui_bridge_slider_model_create_v1(cui_bridge_app *, cui_bridge_node parent, const char *key_utf8,
    uint32_t config_size, const cui_bridge_slider_config_v1 *,
    uint32_t value_size, const cui_bridge_slider_value_v1 *, cui_bridge_node *out);
cui_bridge_status cui_bridge_slider_model_configure_v1(cui_bridge_app *, cui_bridge_node,
    uint32_t config_size, const cui_bridge_slider_config_v1 *,
    uint32_t value_size, const cui_bridge_slider_value_v1 *);
cui_bridge_status cui_bridge_slider_model_get_config_v1(cui_bridge_app *, cui_bridge_node,
    uint32_t config_size, cui_bridge_slider_config_v1 *out);
cui_bridge_status cui_bridge_slider_model_set_value_v1(cui_bridge_app *, cui_bridge_node,
    uint32_t value_size, const cui_bridge_slider_value_v1 *);
cui_bridge_status cui_bridge_slider_model_get_value_v1(cui_bridge_app *, cui_bridge_node,
    uint32_t value_size, cui_bridge_slider_value_v1 *out);
cui_bridge_status cui_bridge_poll_slider_request_v1(cui_bridge_app *,
    uint32_t request_size, cui_bridge_slider_request_v1 *out);
cui_bridge_status cui_bridge_accept_slider_request_v1(cui_bridge_app *,
    uint32_t receipt_size, const cui_bridge_slider_receipt_v1 *,
    uint32_t value_size, const cui_bridge_slider_value_v1 *);
cui_bridge_status cui_bridge_slider_model_get_presentation_v1(cui_bridge_app *, cui_bridge_node,
    uint32_t presentation_size, cui_bridge_slider_presentation_v1 *out);
cui_bridge_status cui_bridge_slider_model_part_bounds_v1(cui_bridge_app *, cui_bridge_node,
    uint32_t part, cui_bridge_rect *out);
```

Enable is explicit before factory; no implicit allocation in old constructors.
Queue options size/schema16/1, reserved0, capacity1..4096;0 selects4096. Re-enable
same capacity on owner thread is idempotent, different capacity INVALID_STATE;
no resize/shrink/receipt replay. New factory requires enabled sidecar. A readonly
new model can exist without REQUESTS, but enabling remains explicit for common
thread/epoch ownership. Capacity exhaustion/unsupported returns existing codes.
Part0..4 means ROOT/TRACK/FILL/START_THUMB/END_THUMB. It queries actual last
completed geometry; absent/inactive START/disabled omitted paint can return EMPTY
without changing out. Getter never manufactures presence from stable IDs.
Root geometry and semantic nodes may exist when decorative surface paint omits.
Presentation query copies last completed render status (explicit mapped0..5),
absolute accumulated Core time and passive next wake (0 idle). Before first
successful submission EMPTY; failed completed frame CORE_ERROR, outputs unchanged.
It exposes observations, never internal animation state or renderer capability.
Manual hosts translate `next_wake_seconds-core_time` to their monotonic clock;
accept invalidates via existing Core mechanism and needs a later host frame.
`src/bridge/cui_bridge_sdl.c:bridge_run_frame` already continuously invalidates when
an after-frame callback exists. Preserve that old run cadence; no claim this
slice converts native run/all-five loops to passive scheduling. New UI examples
can use the copied deadline in manual loops without adding a dispatcher.

## Validation and atomicity

- Exact top-level size/schema1, reserved0, known flags/tags, nonnull accessible
  declared storage extents. Public callers own all extents; no buffer probing.
- Value: reserved0, SINGLE0 ignores start even NaN and canonicalizes copied start0;
  RANGE1 requires finite min<=start<=end<=max. End always finite. No sorting,
  float32 casts or scalar end-only fallback. Getters return full tagged copies.
- Domain uses actual `cui_slider_model_domain_valid_v1`: finite min<=max, finite
  span, finite step>=0; log requires positive min<max. Do not inherit legacy
  min<max/positive-step/float-safe limit restrictions. Off-grid pairs may be valid.
- Config: orientation0/1; source_generation is copied opaque64, including0;
  rem finite>0 with finite1.5*rem. FIXED requires finite length>0; PARENT ignores
  length and canonicalizes copied length0. No palette/brush/theme pointers.
  Owned style is resolved from current app theme via the actual facade, not
  preset-name inference. V1 visible linear brush rejects before Base. Real V2
  host/palette transport is deliberately absent, not flattened or advertised.
- REQUESTS absent or READONLY set makes new model readonly. Flags remain stored
  separately so changing the explicit observer policy is visible in config.
  New common disabled changes invalidate receipt policy; old nodes unchanged.
  Existing set-text copies the accessible name, with bounded copied derived
  start/end labels; no public borrowed strings. New-kind label changes revise
  receipt configuration. Actual visible labels remain ordinary sibling Label
  nodes in examples. Initial source/rem/layout/domain defaults are explicit
  wrapper config factories, not inferred from scalar legacy Value/range.
- Config+value change validates both before commit, updates external confirmed
  pair and increments revision, retains Base reducer for captured cancellation,
  resets presentation as needed. Never reinitialize an active Base gesture to
  bypass CANCEL. Direct set-value also increments revision even for equal pair;
  successful accept does not. Revision UINT64_MAX rejects replacement atomically.
- Invalid getters, size/version/reserved/tag/pair, wrong phase/thread or capacity
  preserve output bytes and queues/owner/config. Public input is copied before
  mutation, so normal caller input/output aliasing is safe. Model factory failure
  preserves public out/queues/other nodes; existing allocator may consume an
  unpublished internal ID, never a reusable public receipt identity.
- REQUEST output is fieldwise mapped and canonicalized, never memcpy of Base
  metadata/padding. phase BEGIN0/UPDATE1/COMMIT2/CANCEL3; reason0..9 follows actual
  Base named vocabulary, thumb START0/END1. event_kind UPDATE=REQUEST1,
  COMMIT=COMMIT3, BEGIN/CANCEL=LIFECYCLE5. SOURCE UNKNOWN0,PROGRAMMATIC1,POINTER2,
  KEYBOARD3,ACCESSIBILITY4. event flags HAS_FRAME1/HAS_INDEX2/HAS_SOURCE4 only;
  HAS_ACTIVATION8 never synthesized. Target is actual semantic64 ID, not node.
- Absent event frame=0/index=UINT64_MAX; present size_t index widens checked.
  UNKNOWN omits HAS_SOURCE. Actual input flags HAS_INPUT1/HAS_NUMERIC_VALUE2;
  absent input is allzero. Pointer/key fields copy actual evidence only; numeric
  SET keeps raw published-semantic double/action/target. Increment/decrement
  inactive numeric value canonical0; no coordinate or key fabrication. Producer
  maps supported Core constants explicitly in source; it must not assert raw
  enum ABI equality. Proposed output tags: POINTER_MOVE0/DOWN1/UP2, KEY_DOWN3,
  CANCEL7, ACCESSIBILITY13; key NONE0/ESCAPE4/LEFT5/RIGHT6/UP7/DOWN8/HOME9/END10/
  PAGEUP19/PAGEDOWN20, modifier SHIFT1/CTRL2/ALT4/META8; pointer MOUSE0/TOUCH1/PEN2,
  button PRIMARY0/SECONDARY1/MIDDLE2, accessibility INCREMENT2/DECREMENT3/SET4.
  Inapplicable fields zero; unsupported future output vocabulary requires a
  new schema rather than silently mapping to mouse/key/UNKNOWN evidence. New wrappers retain unknown future
  schema as unsupported rather than guessing. Output records are copies, not
  submitted event authority; accept reads only receipt+explicit value.

## Queue, identity and receipt authority

Proposed lazy private app sidecar: one pointer in private app, a bounded pointer
index for `(max_nodes+1)` typed owners, queue and owner thread/epoch state.
No old node-record growth, eager queue allocation or new old-constructor branch.
Allocate sidecar/index/queue in one checked aligned block on successful enable;
per-new-node owned model/presentation/config/value allocated independently.
Formula is `sizeof(private_header)+aligned((max_nodes+1)*sizeof(owner_pointer))+
capacity*sizeof(request)` plus explicit alignment padding; exact header/owner
sizes require production `sizeof` receipt, not an invented ABI promise. Queue
payload at default4096 is1,212,416bytes; opt-in only, retained until app destroy.
One-node pure state160 +presentation112 observations do not imply private-owner
allocation size. Allocation failure leaves previous state untouched.

Before each writable model submission, check overflow-safe `3*frame_event_count+1`
free request slots, using actual public `cui_frame_events`; reserve/release unused
budget around that one synchronous reduction. Count all events, including ones
not ultimately eligible. No allocation/overwrite/drop-oldest on callback, no
foreign callbacks while stepping. Preflight failure occurs before this node's
Base/state/input/callback/paint, sets bridge frame_error CAPACITY; previously
processed nodes/events remain real. It is not whole-frame rollback. Small custom
queues can conservatively reject even when fewer requests would have emitted.
With current max_nodes1024 / dispatch512,4096 covers an initially empty queue's
conservative bound plus at most one synthetic cancel per preceding model; this
is source-derived, not a permanent public Core capacity promise. Backlog can
still exhaust it. No new target-dispatch introspection or copied reducer logic.

Epoch proposed: private process counter starts1, issued only to successful new
sidecar opt-in, never pointer-derived or reused. Protect uint64 counter using
C11 atomic_flag (always lock-free flag primitive), saturating at UINT64_MAX;
exhaustion CAPACITY. No global receipt registry. Production must verify actual
native/WASM implementation/link support before advertising versions bit0.
Node identity remains full slot+generation plus semantic ID in evidence. Receipt
revision starts1; source/config/owner replacement invalidates old authority.
Record first/last **actually emitted contiguous** request sequence per current
submission plus last accepted sequence; no sequence wrap. A receipt is an
integrity-checked process-local snapshot, not a cryptographic capability against
malicious in-process code. Matching an unissued guessed sequence must reject.

Poll copies one record and consumes once; exact-size invalid fails before any
cleanup. Removed/reused node origins may be pruned on valid poll, matching current
bridge stale-origin policy; live but expired receipts remain readable snapshots.
EMPTY preserves out. Already copied records outlive later polls/frames/app destroy;
using a destroyed C app pointer remains invalid, never a supported accept call.

Accept requires owner thread, outside stepping/Core frame (after-frame allowed),
valid completed Core snapshot, matching nonzero epoch/live full node/source/
revision/latest admitted painted state-frame and emitted range, sequence strictly
larger than last accepted, valid full pair in CURRENT domain/kind and writable
current policy. Accept updates owner +last accepted only; it schedules future
paint using existing bridge invalidation/host wake policy, never alters prior
commands. Owner may choose a valid pair different from proposal. In the same
submission, accepting UPDATE does not invalidate later COMMIT. No autoaccept on
phase, poll, input consume or callback success. Captured CANCEL is readable old
kind/domain; explicit adjusted current-valid acceptance is possible, never blind.

Invalidate windows on failed begin/end/current invalidity even after error clear;
request copies already emitted remain readable, not acceptable. Geometry-only
recovery must not renew: require actual model state-frame==submission frame plus
successful completed snapshot; next healthy admitted state-frame can reopen.
No rollback/replay of already performed callbacks or earlier owner acceptance.
Existing b24 layout-only rule remains: no renewal or expiry solely from a
layout-only visit. **Proposed clarification requiring root review:** ordinary
completed omission revokes acceptance, whereas retaining a layout-only copy does
not grant new authority. Removal/reparent/policy change invalidate regardless.
No frame-wrap collision: sequence never reuses, emitted window empty if no new
requests; zero-generation phase never creates a receipt.

## Thread and callback policy: explicit decisions

The old bridge does not enforce portable thread identity. Do not pretend existing
`bridge_ready` already rejects wrong-thread typed calls. Proposal: capture a
private C11 `_Thread_local` tag address on first successful enable; new API, new-kind common mutation branches and
new-model step path require same tag (plus serialized lifetime/no concurrent
calls). Owner OS thread stays alive through destroy, preventing thread-exit/tag
reuse from becoming an ownership-transfer API. On wasm single-thread targets,
one explicit constant owner is sufficient only when host cannot share it across
threads. No portable thread dependency or platform types leak publicly.

**Root decision required:** approve/probe this private TLS policy and epoch
atomic_flag before implementation, or explicitly revise the earlier guaranteed
wrong-thread rejection to a caller precondition. Unsupported TLS/target mode must
advertise versions0 rather than claim enforcement. No locks around Core/callbacks,
new dispatcher, old thread checks, thread migration or public host callback added.
No process-race guarantee for destroying a handle concurrently with any API.
Go already locks the creating OS thread and checks a private platform token in
`bindings/go/cui/native.go`; Rust already documents creating-thread ownership.
Reuse their authority rather than insert a second wrapper owner or transfer API;
the new C check closes the current direct-C gap only.

Existing C `step` never invokes foreign change callbacks. Python `step` only
steps; dispatch is separate. JS `step` steps then `_dispatch`; step_only doesn't.
Go Step and Rust step also only step; their Dispatch/dispatch/pump/run compose
callbacks in existing phases. Preserve these differences and ALL legacy order.
Propose new `dispatch_slider_requests` methods and explicitly call them after
legacy dispatch in existing combined helpers/run callbacks. No global original
ordering between the two queues; each queue is FIFO. Do not silently interleave
or retime old callbacks. Manual polling never dispatches. A legacy callback
that advances an app may expire later typed receipts; do not change old behavior
or replay stale requests to conceal that fact.

During new typed callback only, wrapper guard rejects nested step/run/typed
dispatch, keeping accept/config operations explicit and outside active Core.
External owner/userdata mutation and enqueueing later work are permitted. New
request records are owned immutable copies; closure registration lives until
removal/dispose. No C callback pointer retained. Python/JS exceptions and Rust/Go
panic cannot cross C: callbacks are after FFI return; native after-frame trampoline
uses existing catch/latch/rethrow/error-on-return policy. The failed callback's
already-popped request stays consumed, no automatic accept/retry; later queue
entries remain available. Dispose semantics remain the existing deferred native
run contract; never use app after destroy returns.

## Five authoring surfaces and real tests

| Consumer | Proposed additive authoring and owned mapping |
| --- | --- |
| Python | `slider_model(key, config, confirmed).on_request(fn)`, frozen tagged value/domain/input/request/receipt dataclasses, `app.accept_slider_request(request.receipt, pair)`; float doubles, int identities, no ctypes storage retained. Optional symbol group allows old library loading and errors before mount on use. |
| JS | `slider_model(key, config, confirmed).on_request(fn)`; addon fieldwise copies, frozen tagged objects; number doubles and bigint ALL64-bit identities/frame/index/source/sequence, uint32 node fields. Reject unsafe Number identity, no JSON transport. Explicit app accept and typed dispatch. |
| TS | Match JS in both declaration entrypoints, discriminated Single/Range, bigint receipts, typed callback+explicit accept. Positive and negative compile tests plus actual JS UI runtime, not types-only closure. |
| Go | `SliderModel(key, config, value).OnRequest(fn)`, uint32 tags,uint64 IDs,float64 complete pairs; App.AcceptSliderRequest returns error, copied cgo output, existing error latching/mount ordering retained. Optional symbols preserve old dynamic loads. |
| Rust | Builder/owned Single/Range and config/request/receipt records using f64/u64; explicit Result accept; owned copy before callback and no RefCell/mutable App borrow held across callback. Static matching SDK; optional dynamic symbol group fails cleanly on old libraries. |

Each language needs a genuinely mounted UI example (Row/Column +Single and Range,
real callback and accept/reject policy, next-frame accepted paint), not a launcher
or a print-only poll demo. Runtime tests use current bridge and an independently
preserved pre-feature shared library to prove old symbols/loaders still work.
New feature missing returns BACKEND_UNAVAILABLE without silently using Value/range.
All existing legacy builders and callbacks keep their routes/timing.

Current manual injection gaps: `cui_bridge_queue_key` accepts only keys1..8
(Tab through Down), and `queue_pointer` carries only mouse phase/x/y. Native SDL
can deliver richer Core events, but old foreign injection cannot prove Home/End,
PageUp/Down or contact identity. Do not widen old validation invisibly or call a
mouse event a touch test. Separate reviewed additive versioned key/contact input
records forwarding existing Core queue (no dispatcher) or actual native host
fixtures are required for those all-five tests. Their exact API is OUTSIDE this
receipt slice; mark omitted until approved. Similarly typed SET injection helpers
are not Linux AT-SPI delivery proof; native precision/host options remain separate.

Verification plan for C slice: native/ILP32-layout/actual wasm ABI static asserts,
old-size/default-loader/legacy trace parity, output byte snapshots on every
failure, input aliasing, all value/domain extremes from existing pure oracle;
actual new Styled paint/bounds/current-vs-requested pair, both axes/kinds,
reverse/log/step/rejection/disabled/readonly, held capture config CANCEL,
omission/recovery/current layout failure+clear, same-frame UPDATE→COMMIT ordered
accept, foreign app/node reuse/revision/source/frame/sequence/guessed-unissued
rejection, epoch/revision/sequence exhaustion, queue preflight/backlog/retention,
post-failure readable-but-unacceptable snapshots, wrong-thread policy and callback
exceptions/reentrancy in each runtime. First frames have real generation only.
Headless native always private Xvfb; browser headless. No whole-family claim from
C helpers: source palette/gradient consumers, real native numeric delivery,
vertical registered-scroll admission and all-five installed UI authoring remain
independent requirements. On implementation, update exact guide/usage metadata,
binding coverage/source lists/SDK/dist/skill, preserve78 and run usage6/skill18.

Proposed ownership: bridge worker owns header/new private model queue module,
bridge internal sidecar/render integration and focused C tests; language workers
own only their wrapper/runtime example/tests after C ABI approval; Core/native
and vertical-layout owners remain independent. Integrator alone owns shared
`tests/web/test_browser.py`/Pages workflow and canonical integration/push.

## Review resolution and portability probe receipt (2026-11-01)

Root approved kind81, fixed records, independent FIFO, explicit acceptance,
legacy timing preservation and ordinary completed omission expiry. Native owner
TLS plus saturating atomic_flag epoch policy is now bounded-probe verified;
compatible bridge production still waits for root's review of this checkpoint.
Record layout calculations above remain calculations until production ABI tests.

Approved target policy is conservative and explicit, **not** an assumption that
WASI cannot have shared memory. The pinned linker demonstrably supports it.
Proposed private implementation guard, exercised by the test-only fixture:

```c
#if defined(CUI_BRIDGE_SLIDER_NO_TLS)
#define OWNER_SUPPORTED 0
#elif defined(__wasm__)
#if defined(CUI_BRIDGE_SLIDER_WASM_UNSHARED) && CUI_BRIDGE_SLIDER_WASM_UNSHARED == 1 && \
    !defined(__wasm_atomics__) && !defined(_REENTRANT)
#define OWNER_SUPPORTED 1
#else
#define OWNER_SUPPORTED 0
#endif
#else
#define OWNER_SUPPORTED 1
#endif
```

- Native portable C11 TLS/atomic_flag: supported where these compile; actual
  runtime proof here is Linux GCC, not Windows/macOS runtime evidence. An
  unsupported compiler/build sets private `CUI_BRIDGE_SLIDER_NO_TLS` and keeps
  new versions0/stubs BACKEND_UNAVAILABLE. Do not fall back to a constant owner.
- **Default WASM versions0**. Only explicit
  `CUI_BRIDGE_SLIDER_WASM_UNSHARED=1` opts a trusted unshared builder in, provided
  atomics/reentrant macros are absent. This flag is a build assertion, not a
  runtime capability query or public record field. It requires final-artifact
  verification: memory is nonshared and no imported shared memory can replace it.
- The C preprocessor cannot detect another translation unit's flags or a later
  linker `--shared-memory`; compile flags alone never prove final host ownership.
  Unverified generic WASM SDK consumers must retain versions0. Known builds
  must run postlink verification on their actual final module before advertising
  this opt-in. The probe's actual default module had imports[] and nonshared
  exported memory; it used the compiler's `_Thread_local`, no replacement token.
- Atomics/pthread/shared WASM remains versions0 even with the unshared assertion.
  No Worker TLS initialization, thread tag uniqueness across Workers, native
  shared-memory ownership enforcement or threads-disabled fallback is claimed.
  The support-disabled production path must not allocate/issue epochs/enable
  kind81; old bridge routes remain available. No more shared-Worker work is
  required for this v1 slice.
- The owner OS thread must stay alive until app destroy. No ownership transfer,
  dead-thread tag reuse, concurrent API/destroy or synchronization of Core from
  another thread is supported. Native worker threads in the probe stay alive
  while their distinct tags are checked; it does not use recycled thread IDs.

### Reproducible bounded evidence

Test-only sources are `tests/fixtures/bridge_owner_primitives_probe.c` and `.cjs`.
They include no bridge/Core implementation or public declarations. No production
allocator/epoch test hooks are added. Native compile is strict C11/O2/warnings
as errors with `-DPROBE_NATIVE=1 -DPROBE_EXPECT_SUPPORT=1 -pthread`; only the probe's
C11 thread harness needs pthread linkage. The intended production primitives do
not introduce a C11 thread-library dependency. Four simultaneously live owners,
4000 unique epoch values and MAX-1/MAX/exhausted output-atomic behavior passed.
Execution used the standard private-Xvfb wrapper with a CTest script invoking
this windowless binary, not the desktop.

Pinned Zig executable is the installed0.16.0, compiled with `-target wasm32-wasi
-mexec-model=reactor -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror`. Export probe
owner/match/epoch/seed, support and feature-epoch functions plus memory. Explicit
unshared build adds `-DCUI_BRIDGE_SLIDER_WASM_UNSHARED=1
-DPROBE_EXPECT_SUPPORT=1`; actual Node/WASI reactor runtime observed support1,
nonshared memory, stable own tag, wrong-tag rejection and exact uint64 saturation.
The default/unasserted guard compiled to an object with
`-DPROBE_EXPECT_SUPPORT=0` and a successful static assertion. It was deliberately
not silently opted in. Zig's `-fsyntax-only` wrapper first diagnosed an unused
implicit `-c`; the check was rerun as a real strict object compile, without
suppressing warnings or modifying primitive behavior.

Shared characterization adds `-matomics -mbulk-memory -pthread`, the same unshared
assertion, `-DPROBE_EXPECT_SUPPORT=0`, linker `--shared-memory --import-memory`
and exports `__wasm_init_tls`, `__tls_size`, `__tls_align`. Strict compile/link
succeeded. Actual Node instantiation imported shared memory with the module's
exact257-page minimum/maximum; too-small/too-large trial import limits were
correctly rejected first. Runtime verified SharedArrayBuffer, support0 and
feature-epoch rejection0. It did NOT call raw shared TLS primitives or invent a
Worker initialization protocol. Node emitted its existing experimental WASI
warning; no C source warning was suppressed.

All artifacts reuse existing `build-focused`; no new checkout/build tree,
engine build, dependency installation or heavy skill suite. Native/default/shared
three final cached links and the additional default-policy object terminated
successfully. Logs and artifact/source hashes are recorded externally under
`/home/matteo/.cache/cui-current-validation/bridge-owner-*`; the signed handoff
receipt identifies the exact commit. No performance/benchmark claim.

Future production should first implement this approved supported-target C slice,
then prove real records/receipt rules and frozen legacy behavior. Richer bridge
input, five foreign authoring routes, native AT precision/delivery, vertical
registered-scroll and actual V2 gradient adapters remain separate open work.

### Production admission (2026-11-01)

Root approved the supported-target compatible C slice after the guard probes.
Factory signature is corrected before shipping: explicit full parent handle
follows app and precedes key. There is no public attach/reparent API; implicit
root would break nested composition. Factory validates actual Row/Column parent
and retains existing key/ancestry/allocation atomicity. Other records/signatures
and all old ABIs stay unchanged. Required proof includes nested Row/Column UI.
The unshared runtime fixture now asserts memoryShared==false, rather than merely
logging it. Actual final-module postlink verification must enforce that condition.

The unshipped CMake helper from the ABI checkpoint is removed before
implementation: defining a feature on a shared bridge target while verifying
one consumer cannot certify every other final consumer. No shared-WASM CMake
linkage/automatic capability claim is made. Instead the trusted opt-in recipe is
explicit and applies to EACH final module: compile the actual bridge TU with
`-DCUI_BRIDGE_SLIDER_WASM_UNSHARED=1` and no atomics/reentrant flags, link a valid
wasm32 final module, run the normal engine/link validation, then run
`python scripts/verify_slider_unshared_wasm.py FINAL.wasm` before delivery, and
independently inspect actual runtime Memory.buffer in the host proof. Re-linking,
embedding in another final host or transforming requires repeating those checks.
Do not distribute a pre-opted-in archive/shared bridge as if unrelated consumers
were already verified; generic SDKs remain default versions0. A dedicated build
variant/automation requires separate review rather than global compile policy.

`scripts/verify_slider_unshared_wasm.py` inspects imported and defined memory
descriptors, rejects shared/memory64/multiple/missing/ambiguous memory and malformed
descriptor data; it is a memory-policy checker, not a full module validator.
Successful normal final-module validation is a prerequisite. Four offline tests
pass, actual pinned unshared artifact passes and actual shared artifact fails.
The runtime fixture additionally asserts actual Memory.buffer is not shared.
No renderer negotiation or automatic supported-host discovery is inferred.

### Implementation review resolutions

The private epoch counter is scoped to one loaded bridge image / unshared WASM
module instance, not a registry across independent duplicate libraries/modules.
All handles and receipts must stay within the matching instance and app;
cross-image mixing is unsupported. Same-image app epochs never reuse/wrap.
This supersedes the proposal's overly broad “process counter” wording; no
registry, random nonce or security boundary is added.

Any failed actual bridge step, including later-node preflight/facade failure or
native rendering after successful Core end, revokes all typed receipt windows.
Queued copies remain readable; presentation/bounds return CORE_ERROR unchanged.
A retained private failure bit survives diagnostic clear and geometry/layout-only
recovery until a healthy ordinary admitted model state-frame. Invalid argument
rejection before submission does not expire existing authority. Already delivered
application effects are never rolled back.

Outer named thumb bounds are rem-sized interaction/layout geometry, not the
inner rem-2 painted surface/rings. Disabled source omits thumb paint but keeps
track/fill paint and their actual positive bounds. Unsupported future producer
tags/modifiers or incoherent numeric evidence fail publication atomically; a
failed map latches observable frame error and suppresses remaining publication,
so an unissued sequence gap cannot become an acceptable interval.

### C implementation evidence (2026-11-01)

The additive C-only kind81 implementation is verified, pending integration; this
is not completion of native AT, five typed builders, bridge touch/SET injection,
vertical registered-scroll admission or actual V2 adapters. The original proposal
above is retained as review history; implemented consumer authority is
`include/cui/cui_bridge.h` and `docs/BRIDGE_SLIDER_MODEL.md`.

Strict GCC C11/O2/Wall/Wextra/Wpedantic/Werror, linked to the integrator's existing
engine without rebuilding it, and private-Xvfb CTest:11/11 passed. Public UI proof
composes nested Row/Column, Single/Range and both axes; verifies original-order
keys, immutable current/evolving intent, explicit alternative full-pair owner
acceptance, double retention, readonly/disabled policy, capacity and expiry.
Test-only same-TU proof covers lazy allocation failure, actual Core normalized
SET .75→domain75/raw .75,64-bit target identity, failed Core clear/recovery,
later-node CAPACITY with valid Core snapshot, simulated native render failure
through actual step/end/render/finish, phase0 observation/receipt preservation,
ordinary omission, removal/reuse, revision/epoch exhaustion and failed-map gaps.
The simulated host proves control flow, not physical SDL/hardware failure.

Independent frozen legacy trace from exact `ecb97d5e` bridge TU/private/public
headers linked to the SAME engine matched candidate and preserved pre-typed
binary byte-for-byte in three distinct executable processes. SHA256:
`e2ce75ffe88a98e784acead4e5269175e1565a70e7081b6924b9850f89790415`.
The preserved binary's source revision is unknown; no revision is inferred.
No LD_LIBRARY_PATH/LD_PRELOAD; each executable has its intended absolute rpath.
The portable regression retains that independently captured trace and normalizes
only CRT `%a` spelling, comparing exact represented doubles without tolerance.

Actual LP64 object sizes: private app1280→1288 (+8), old node73136 unchanged,
lazy header56 (failure bit fits padding), per-new-owner472. Enable block is
`align_up(56+(max_nodes+1)*sizeof(pointer),alignof(request))+capacity*296`;
default256nodes/4096requests uses1,214,528bytes, allocated only on enable.
Existing constructor allocation count is unchanged; its private allocation grows8.
Node keys/names retain existing separate allocation policy.

Pinned Zig0.16 wasm32-wasi strict actual bridge TU policy assertions passed:
default0, explicit unshared1, atomics/pthread0. Fixed record ABI assertions also
compiled/linked and ran in actual Node WASI (24/32/72/56/72/296/32/16byte records).
The earlier real memory/TLS probes remain the supported-mode evidence; these
object checks are not a final application renderer/AT/runtime proof.
Canonical SDK/agent generation, usage6, skill18 and format/amalgamation/SDK/skill
freshness all passed. No new dependencies or build/output tree were created.
