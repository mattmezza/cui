# Component events: proposed contract and implementation slices

Status: committed design checkpoint. The shared metadata utility is now
[implemented](../COMPONENT_EVENTS.md); component-family facades and other
contracts proposed below remain unimplemented unless explicitly linked to an
implementation guide. The committed
[78-family inventory](COMPONENT_EVENTS_COVERAGE.md) and
[binding audit](COMPONENT_EVENTS_BINDINGS.md) accompany this checkpoint. Their
source evidence establishes existing contracts and identifies missing proof;
it is not a claim that proposed behavior is implemented. Existing headers remain
the authority for implemented behavior.

## Architectural decision

Keep immediate submission and synchronous component-local dispatch. Do not add
capture/bubble routing, an end-of-frame callback queue, a scheduler, a thread
pool, or networking to Core. New optional facades should preserve existing
ownership models rather than replacing them. A family that cannot adopt this
contract without changing public timing, mutation, or ownership must be split
out for a reported architectural decision before production implementation.

Preserve explicit exceptions established by the audits: Speech's pump and
foreign-language notification queues retain their current delivery timing.
Surface, Notification, Carousel, Resizable, Root, Questionnaire and Dock keep
their retained registration/descriptor lifetimes. Plot/Chart callbacks deferred
to layout/prepaint/paint remain valid through their existing current-frame
boundary. Omitting a drawing call does not cancel retained lifecycle work.
No broad synchronous-dispatch or submission-borrowing claim covers these APIs.

The bridge currently reports post-mutation notifications, not synchronous typed
value requests. Preserve that path. A future opt-in foreign request hook needs
its own reviewed retention, cross-language exception, reentrancy and value-owner
design; adding fields to a queued notification cannot implement rejection. No
new request hook or retiming is authorized by calling this checkpoint complete.

Current source examples establish why a blanket replacement is unsafe:

- `include/cui/cui_components.h`: Button has a legacy callback followed by a
  typed click-evidence callback. Its boolean result still reports activation.
- `include/cui/cui_base.h`: controlled Check emits a typed request without
  writing the value; a rejected request can still return accepted activation.
- The controlled double Slider has begin/update/commit/cancel phases and an
  externally owned value; its boolean means submission, not value change.
- `include/cui/cui_edit.h`: editing already has transaction validation,
  mutation guards, and borrowed text lifetimes. Do not bypass those mechanisms.
- `cui_event` contains a borrowed optional text pointer. Copying the struct alone
  is not a general deferred-event copy operation.

The audit documents must record other family-specific differences, including
legacy functions that mutate caller storage directly. These differences are
compatibility obligations, not reasons to reinterpret existing results.

## Vocabulary and typed payloads

Use these meanings where a component actually supports the interaction:

| Concept | Meaning |
| --- | --- |
| Activation | An accepted attempt to invoke an action; no implied value change. |
| Change request | A typed proposed value offered to the value owner. |
| Change notification | A typed value transition that has actually been applied. |
| Commit | Completion of a value-editing gesture/transaction; may have no net change. |
| Submit | Explicit submission of entered data, distinct from typing or blur. |
| Lifecycle | Open/close or begin/cancel/end with a component-specific reason. |

Avoid a generic `void *value` payload. Boolean, check-state, scalar, selection,
date, text-edit and other values retain typed declarations. `void *userdata`
is solely an application context convention. Use a shared, pointer-free metadata
envelope only for common facts; preserve component-specific reason/value fields.
Final public names and layouts require the audits and ABI review first.

Metadata should identify the stable component ID and available event sequence,
input origin and evidence. Represent unavailable facts explicitly. Do not invent
a pointer position for a key or accessibility action, a physical device ID, a
host timestamp, or an event index for programmatic lifecycle work. Existing
`cui_activation_snapshot` and Button evidence should be reused where their
documented evidence is sufficient, not reconstructed after mutable callbacks.

## Four independent decisions

1. Value acceptance: the owner accepts/rejects a proposed typed value. A request
   alone is never a committed-value notification.
2. Input consumption: existing reducer/host handling determines whether the
   input has been handled. Value rejection does not automatically unconsume it.
3. Propagation: no new general routing is proposed. A facade cannot advertise
   stop-propagation semantics that Core does not implement.
4. Platform defaults: retain explicit existing host facilities. Callback return
   values must not pretend to cancel browser/native defaults already performed.

New controlled facades should take a value snapshot and emit requests without
mutating it. The owner may assign external state synchronously, and the next
documented snapshot point observes it. Existing pointer-mutating entry points
keep their exact behavior. An optional acceptance hook for an existing
transaction is distinct from a notification handler. Do not promise a committed
notification for external owner assignment that CUI cannot observe.

## Timing, ordering and results

Callbacks run synchronously at the current family's reduction/submission point.
Do not silently defer them to end-frame. Freeze required evidence before a
callback can consume input or mutate application state. Preserve existing
legacy callback order; in particular, Button legacy `on_click` precedes its
existing typed click handler. New per-family adapters need explicit source and
legacy regression tests before claiming this ordering contract.

Process multiple supported events in original reducer order. A result may
aggregate counts and flags and retain a documented last value/reason, but must
not imply that only one callback occurred. Overflow behavior must be explicit;
prefer bounded counters with an overflow flag to an unbounded event log.
Reading a result never dispatches, reapplies mutations, or repeats callbacks.
Applications choose callbacks or frame results for each side effect.

The proposed new result vocabulary distinguishes submitted, activated,
requested, applied, committed and cancelled rather than overloading one
`changed` boolean. Families expose only meaningful fields. Legacy result types
and booleans keep their documented meanings through separate adapters.

Disabled and inert components emit no user activation/value requests. Loading
suppresses activation only where the family's contract defines that behavior;
do not globally change legacy loading semantics. Read-only may allow focus,
selection or copying while suppressing changes. Omitted immediate components
cannot generate new submission callbacks; retained resources follow their
explicit release rules. Define cancellation on disable/removal for each
active-gesture family, without fabricating an input origin.

## Lifetimes and callback mutation

For new immediate facades, handlers and userdata are borrowed through that
submission call. They are not stored for the next frame. Retained APIs must
declare registration/replacement/release lifetimes separately; do not relabel
existing retained userdata as submission-borrowed.

Payloads are callback-borrowed unless explicitly documented as copyable values.
Pointer-free snapshots can be copied by value. Borrowed text, selections or
arrays require caller-provided storage and checked length/capacity for deferred
copies; no hidden allocation or unbounded queue. A copy operation must fail
explicitly for insufficient capacity and leave no misleading partial payload.
Numeric resource limits must come from audited existing limits or an explicit
new bounded type, not undocumented assumptions.

Callbacks may update unrelated application state or the value owner's external
storage according to the family's snapshot rules. Recursive submission of the
same component, recursive frame entry, destruction of the UI context, and
freeing active callback/state storage are unsupported. Existing edit mutation
guards remain effective. New facades need a clear failure/guard contract rather
than allowing use-after-free. Removal/shutdown is requested in app state and
performed after the active submission/dispatch returns. Whether a later
component is omitted in that same frame remains normal application control
flow; it must not cause a queued callback to run on freed userdata.

## Alternatives considered

- One untyped universal callback: rejected because it erases value types and
  makes payload ownership difficult to check.
- Force all controls into internal retained state: rejected because it breaks
  immediate caller-owned values and controlled requests.
- End-of-frame callback batching: rejected because callback ordering and
  synchronous mutation are observable behavior.
- DOM-style capture/bubble: deferred; no present requirement justifies new Core
  dispatch architecture. Explicit local/host consumption remains independent.
- Rename legacy callbacks globally: rejected; additive facades and compatibility
  wrappers allow independently verifiable migration.

## Independently reviewable slices

1. Commit completed source/test coverage, this contract, binding coverage,
   justified exceptions and any unresolved architectural decisions. No production
   event changes before this checkpoint.
2. Add only shared metadata/copy contracts with bounded headless tests. Preserve
   existing Button callbacks and legacy callback timing/mutation regressions.
3. Introduce optional typed facades for activation and discrete controlled
   Boolean/check/selection requests. Verify rejection separately from input
   consumption and preserve old mutation entry points.
4. Adapt continuous editing and transactions independently: Slider first,
   then numeric/text families through their existing acceptance machinery.
   Explicitly test gesture commit/cancel, multiple events and callback mutation.
5. Adapt menus, selection composites and dialog lifecycle independently. Record
   noninteractive and unsupported lifecycle exceptions instead of manufacturing
   callbacks for every family.
6. Update bridge/bindings per supported slice with real UI examples and lifetime
   tests; document omissions until implemented. Regenerate distributions,
   source metadata, SDKs and consumer skill through repository generators.
7. Commit the separate async host example and deterministic offline tests.

Each slice requires warnings-enabled builds, meaningful tests and an isolated
commit. Linux native tests use `python scripts/test_local.py --build-dir build`;
browser tests are headless; unsupported GPU hardware remains deferred. Keep all
78 manifest entries. A completed slice is not full component uniformity.

## Separate async example design

Proposed optional stack: existing SDL3 host plus a libcurl-backed worker. Verify
dependency availability/pinning and repository conventions before implementation;
neither stack becomes a Core dependency. A synchronous UI handler copies the URL
into bounded worker-owned storage, assigns a monotonically increasing request
generation, marks loading, enqueues one job and returns promptly. Reject a
duplicate request while loading.

The worker calls only transport/synchronization facilities. Completion data
crosses a bounded synchronized mailbox and wakes the idle SDL host with its
supported user-event/wakeup facility. Only the UI thread drains completions,
checks the active generation, changes UI state and calls CUI. Failures display
retry; retry receives a new generation. Late completions cannot overwrite a
newer request or a removed view.

Minimum cancellation guarantee is logical cancellation: invalidate the active
generation and ignore its eventual result. If libcurl progress cancellation is
implemented, document its cooperative transport guarantee separately; do not
claim immediate cancellation of DNS or other blocking host operations. Shutdown
signals cancellation, wakes the worker, joins it, drains/frees mailbox data and
only then destroys synchronization/host state. Timeouts must bound real network
work; the injected offline transport controls completion explicitly.

Offline tests must cover prompt enqueue/return, loading/duplicate prevention,
owned request bytes after caller mutation, controlled success/failure/retry,
out-of-order/stale completion, logical cancellation, wake delivery, UI-thread
application, and shutdown with pending work. No test requires network access.

## Queued follow-up

After the overall event API work, reduce consumer-skill output/context cost,
especially example and limitation discovery. A separate Sol 6.1 low audit is
assigned; preserve essential unsupported-API and build guidance while making
long detail explicitly retrievable. This is a subsequent task, not a reason to
delay the event design checkpoint or shrink its documentation obligations.
