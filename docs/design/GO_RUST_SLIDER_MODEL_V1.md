# Go/Rust controlled Slider bridge checkpoint

Source audit at integrated `02240266`; proposal only, no implementation or runtime
claim. C authority is [BRIDGE_SLIDER_MODEL_V1.md](BRIDGE_SLIDER_MODEL_V1.md).
Python and Node/TypeScript already expose the separate bridge-owned route. These
two remaining languages must preserve legacy scalar Slider timing/autoacceptance.

## Actual integration constraints

| Area | Go | Rust |
|---|---|---|
| Authoring | `native.go` owns retained `Node`, `App.Mount` preflight and native creation; `Slider(key)` remains kind9. | `src/lib.rs` owns imperative `App::root/child/create` and app-borrowed `Builder`; `Kind` currently ends at80. |
| Loading | `native.go` C preamble owns `bridge_lib`, mandatory `API` table and optional notification/window resolvers. `New` permits only one loaded-library identity per process. | Dynamic `notification_symbol_v1` uses Unix lookup or Windows module-by-existing-symbol; `cui_bridge_static` directly pairs with bundled sources. |
| Thread | `New` locks the goroutine OS thread before create; `App.check` validates the saved pthread/Windows token. `Close` destroys, nulls and unlocks (native runner defers unlock until return). | `App` has `PhantomData<Rc<()>>`, making it !Send/!Sync. `Node` contains a private app ID. Owner thread stays alive through Drop. |
| Dispatch | `Step` only steps; `Dispatch` drains legacy FIFO, counts entries and stops on Close. Native `goBridgeAfterFrame` captures panic/error before returning to C. | `step` only steps; `frame` steps then dispatches. Legacy callbacks take only `&ChangeEvent`; `run_with` catches panic/error and resumes outside C. |
| Teardown | `RequestClose` requests shutdown; `Close` destroys. Existing `Node.Remove` is explicit. | `App::close` only requests shutdown; Drop destroys. `remove` clears legacy registrations after successful C removal. |

Do not add new functions to Go's mandatory `API` table. Its optional C helpers
must live beside the existing TU-local `bridge_lib` (or an included private
header at that exact seam), not introduce another loader/global registry. Keep
new public records in a focused Go file; thin Go crossing functions in native.go
can bridge them without exposing cgo types publicly.

Rust needs a focused `slider_model` module with private repr(C) wire records and
existing dynamic/static linkage conventions. Dynamic mode resolves the complete
16-symbol group before any use, validates V1 plus all five sizes, and imports no
new required symbol. Static mode may name matching bundled symbols directly;
this is not compatibility with arbitrary older static archives. No dependency,
loader, Core, dispatcher or native host change is needed.

## Owned records and exact values

Both languages expose tagged Single(end)/Range(start,end), domain, complete
config, receipt, request/evidence, presentation and typed part selection. Use
float64/f64 for every value/domain/numeric payload, uint64/u64 for EVERY C uint64
(including event index and its UINT64_MAX absence sentinel), uint32/u32 handle
pairs/enums/masks, and checked float32/f32 only for actual geometry. Never use
int/usize, float, JSON numbers or scalar storage for identities/ranges.

Private wire layouts match C value24/domain32/config72/receipt56/input72/request296/
presentation32/queue16 bytes; test sizes and offsets against C on actual supported
targets. Validate schema/reserved/tag/mask/finite/active fields before publishing.
Authored parent length is ignored/canonicalized; a nonzero inactive native parent
length rejects. Copy all nested records before returning from FFI; no slices,
references to C memory, borrowed native buffers or callback-held C stack values.

Go/Rust records are owned value copies, not a promise that application-local
variables cannot be mutated. Such mutation cannot alter the C submission snapshot
or manufacture authority. Receipt fields are private, with read-only snapshot
accessors; ordinary assignment/Clone preserves an issued receipt. A private
per-App namespace token contains no App/native pointer. Go's token must have
nonzero storage (distinct empty-struct pointers are not guaranteed distinct).
Rust may use Rc identity for authority, deliberately keeping receipts local;
explicit numeric snapshots can be transferred as inert data. Neither scheme is a
security or cross-loaded-image uniqueness mechanism.

Defaults initialize one complete config; reject active nonfinite/invalid values
atomically. Preserve explicit whole-pair acceptance, current/gesture-domain
distinctions, immutable C entry current and ordered evolving requested intent.
No automatic owner write, retry, scalar CHANGE or receipt retagging.

## Proposed Go surface

- `DefaultSliderConfig() SliderConfig`, `NewSliderSingle(end float64)` and
  `NewSliderRange(start,end float64)` return validated values/errors.
- `SliderModel(key string, config SliderConfig, confirmed SliderValue) *Node`
  follows existing builder error accumulation; Mount rejects bad records and
  non-Row/Column ancestry before creating nodes/enabling the FIFO.
- `App.SupportsSliderModel`, `EnableSliderRequests(capacity uint32)`,
  `PollSliderRequest() (SliderRequest,bool,error)`,
  `AcceptSliderRequest(SliderReceipt,SliderValue) error`,
  `DispatchSliderRequests() (int,error)`.
- New typed node methods `ConfigureSlider`, `SetSliderValue`, `SliderValue`,
  `SliderConfig`, `SliderPartBounds`, `SliderPresentation`,
  `OnSliderRequest(func(SliderRequest) error)`; existing `Disabled/Text/Remove`
  policy/retained ownership remains. Do not overload scalar Value/OnChange.

After legacy Dispatch reaches EMPTY, drain the typed FIFO only when enabled.
Pop/copy once, capture handler, invoke after C returns; guard only the new typed
callback phase against Step/Run/Dispatch recursion. defer clears the guard on
error/panic; manual dispatch propagates panic normally, native trampoline resumes
it only after leaving C. Callback may capture App, mutate external owner state,
accept, configure, remove, queue later input, RequestClose or Close. Clear only
new typed handlers on successful removal/Close, stop before another C call when
closed, preserve OS-thread lock/unlock and all old callback behavior.

## Rust callback decision required before production

Proposed base surface: `SliderValue::{Single{end},Range{start,end}}`,
`SliderConfig::default`, `App::slider_model(parent:Node,key:&str,config,value)`
returns Result<Node>; explicit parent required, C validates existing ancestry
atomically. Add supports/enable/poll/accept/config/value/part/presentation methods
with Result and owned Option results. Existing imperative authoring has no whole
unmounted tree to preflight; rejection must not remove previously authored nodes.

Registered typed callbacks need acceptance and label/policy updates, but existing
legacy `FnMut(&ChangeEvent)` cannot access the mutably dispatched App. Do not
silently expand every old callback or use application raw pointers/RefCell tricks.
Recommended new signature:

```text
on_slider_request(node,
    impl for<'a> FnMut(&mut SliderRequestContext<'a>, SliderRequest) -> Result<()> + 'static)
```

The new context privately borrows App and forwards explicit accept(receipt,pair),
configure/set-value, text, remove, queued pointer/key/text input and close-request
operations. It has no Deref/as_app/owned App escape. This permits external owner
mutation and enqueueing without handing a callback `&mut App` that could replace
or drop protected live App storage. Context does not dispatch input or own extra
state. Pop/copy first; clone the callback registration, release map borrows, invoke
with the context, and release it before the next poll. A private guard resets on
panic; existing native catch_unwind boundary remains authoritative. Handler
replacement/removal is allowed without borrowing the map across user code.

Rust close remains request-only; destruction remains Drop after the caller's App
borrow ends. Do not invent Python-style destroy-within-borrow semantics. Manual
poll+explicit accept remains available without registered callbacks. Root review
must approve the narrow context and its exact forwarding methods/signature before
Rust production; unrestricted new `FnMut(&mut App,...)` is an alternative requiring
an explicit protected-storage/replacement policy, not an unnoticed convenience.

## Independently reviewable implementation/proof slices

1. Go complete optional group, owned records, kind81 authoring/dispatch and full
   authored windowless UI. Reuse cached current/old bridge with `-tags cui_dynamic`,
   GOMAXPROCS=1, `go test -p1`; later one matching static/source closure check.
2. Rust records/FFI plus approved context and full authored windowless UI. Reuse
   cached bridge through CUI_BRIDGE_LIB_DIR, Cargo offline/locked/jobs1; later one
   matching bundled-static closure check without rebuilding an engine unnecessarily.
3. Coalesced human/LLM metadata/guides, canonical export and exact installed-source
   examples; all78 remain. No all-five claim until both actual runtimes pass.

Each language needs actual Single/Range × both axes, explicit accepted/rejected
full pairs, double extremes and 64-bit identities, ordering/legacy autoaccept,
capacity/error atomicity, expiry/config/readonly/disabled/omission, copied lifetime,
handler removal/replacement, callback error/panic/close and recursion tests. Prove
Go wrong-thread rejection and Rust !Send/!Sync compile rejection. Use synthetic
wire faults only as protocol evidence. Old-library fallback and all16 missing
symbols must run in distinct processes against an actual old module, with missing
historical-fixture omissions explicit. Native automated tests use private Xvfb;
no desktop, dependencies, registry publishing or physical AT inference.

Native numeric delivery, typed SET/touch injection, V2 gradients, arbitrary direct
C Styled styles/motion and vertical registered-scroll admission remain separate
open requirements after these wrappers; this does not close the whole family.

## Go implementation evidence (unreleased, pending integration)

Go source implements the proposed independent optional group, owned copies and
new typed callback phase. Actual dynamic typed UI tests passed; the first broad
legacy wrapper run failed only native lifecycle/visibility fixtures because no
FontPath had been supplied. That failure log is retained. Repeating with absolute
bundled Inter passed, including those native fixtures; nine unrelated historical
notification/shadow/visibility methods retained their explicit fixture skip gates.

The next dynamic batch passed42/42 private-wrapper cases: font-correct legacy,
actual old-module fallback, all16 missing symbols, support/size failures and
synthetic malformed receipt/input/canonical absence evidence. The matching default
Go source build passed TestSliderStaticClosure with no dynamic library override,
actually resolving all optional entries and producing nested typed thumb geometry.
It compiled required embedded SDK objects rather than assuming weak presence.
Final metadata/skill gates and remaining example modes are still separate gates.
Rust context production remains the next independent slice.
