# Component events: bridge and binding audit

Status: read-only source audit, not an approved event design or a completion
claim. Baseline: `integration/next39-components`, commit
`e9d2c1a561ced3254e606b0206f412af8b156317`. This addresses the binding boundary
of [the request](COMPONENT_EVENTS_REQUEST.md); the separate family matrix owns
all 78 families. No production, generated SDK or manifest changes accompany
this document. Tests below were inspected, not rerun for this documentation-only
audit. Their existence is evidence of intended coverage, not a fresh pass report.


## Implemented additions since the checkpoint

[Shared metadata](../COMPONENT_EVENTS.md) and the direct C
[Button/Check facades](../CONTROL_EVENTS.md) are implemented after this audit.
Button preserves legacy callback order and adds typed activation metadata;
Check offers immutable typed requests with per-invocation counts. These are
submission-borrowed synchronous handlers. The original rows below remain the
checkpoint baseline, not an assertion that later additions are absent.
No Python, JavaScript/TypeScript, Go or Rust wrapper for these synchronous
handlers is implemented; owning manual queued views were added later. See the
current dated snapshot below. Original sections retain their historical scope.

## What exists at the C boundary

The canonical public ABI is [cui_bridge.h](../../include/cui/cui_bridge.h).
`cui_bridge_event_ex` has an event discriminator (`CLICK`, `CHANGE`, `CLOSE`),
origin node slot/generation, `uint64_t frame_sequence`, `uint32_t checked`,
`double value`, text length and selected node slot/generation. Text is copied
separately into the caller's buffer. There is no public per-component C callback
registration in this bridge; applications poll queued events. `cui_bridge_run`'s
callback is an after-frame host hook, not a component event callback.

[The bridge implementation](../../src/bridge/cui_bridge.c) owns retained node
records and component state, including edit models, checked/value fields and
selection/surface state. Language builder objects retain references and initial
properties; mounted readback uses C getters. The ordinary checkbox, switch,
toggle and slider bridge paths call mutating legacy C component facades, update
the bridge-owned value and then call `bridge_change`. Input/editor paths inspect
`result.changed`. This is a **confirmed post-mutation notification**, not a
controlled proposed-value request. Registering no language callback does not
prevent the underlying mutation. Callback returns cannot veto it. Native
source-controlled C component facades are not automatically exposed by these
bridge paths.

`bridge_click` uses `cui_button_options.on_click_event`, queues an event per
accepted button callback and ignores the component's returned aggregate boolean.
The bridge input path does not forward native `cui_input_result.submitted`
(see [cui_base.h](../../include/cui/cui_base.h)); no shared SUBMIT channel exists.
Slider begin/update/commit/cancel phases are likewise not represented by the
bridge's single legacy value-change channel. Other paths translate their
particular result flags into notifications; a
`CHANGE` is not a universal one-to-one record of raw input events. Dialog close
uses `bridge_surface_close`, putting accepted/cancelled/closed in `value`.
That is a surface outcome; it does not implement event acceptance, propagation
stopping or platform default prevention.

The event payload is a generic record, not a typed union:

| Meaning | Current representation and limits |
| --- | --- |
| Button activation | `CLICK`, node, frame. No origin/evidence payload survives. |
| Boolean / numeric state | `CHANGE`, `checked` / `value`; applicability depends on node kind. |
| Editable text | Copied UTF-8 snapshot in `text`; it can differ from the latest getter after later frames. |
| Selected item | Full selected slot/generation snapshot for supported selection paths; not an owning node reference. |
| Surface outcome | `CLOSE`, numeric `value` 1 accepted / 2 cancelled / 3 closed. |
| Time, color, date | Formatted text and/or numeric fields, plus separate typed state getter APIs. Not typed date/time event variants. |
| Data table / questionnaire | Component-specific JSON text plus overloaded generic fields. `bridge_data_table_change` stores the column in `checked` and row in `value`. |

These are actual adapters, not a promise that every family emits every channel.
Some explicit programmatic commands also enqueue events (for example
questionnaire/speech/dock paths); do not assume all setters are event-free or
that every setter produces a notification. Audit the particular setter before
specifying compatibility.

## Timing, ownership and event evidence

`cui_bridge_step` sets `stepping`, polls native input, builds the C frame, ends
it, renders SDL if applicable, then returns status. Component callbacks used by
the bridge run during frame construction but only enqueue snapshots; foreign
language callbacks are invoked afterwards. A callback's state update can affect
later callbacks/getters immediately, but is painted by a subsequent step. It
cannot retroactively change the frame just rendered.

The queue is FIFO **in enqueue order**. Components are visited in scene traversal
order and can aggregate their own input. Neither FIFO nor `frame_sequence`
promises global chronological ordering across components. Multiple events can
share a frame; no event index is exported to recover their original input order.

The bridge defaults to 256 event slots, allows at most 4096, and reports
`CAPACITY` on queue exhaustion. Snapshot text uses owned heap allocations;
allocation can fail with `OUT_OF_MEMORY`. There is no allocation-free handler
contract. Rendering paths can mutate state before an enqueue failure: step
failure is not a transaction that rolls back component state or old queued
events. Some explicit commands preflight capacity separately.

`cui_bridge_poll_event_ex` copies a snapshot and consumes it once. On insufficient
buffer it reports required capacity and leaves that event queued. It frees
internal text on consumption, stale-origin discard or app destruction. The
caller owns copied text/output storage. The legacy `poll_event` consumes and
discards all non-CLICK events while searching for a click; mixing the two poll
APIs loses notifications. Polling/readback never invokes language handlers.
Calling a language dispatcher after manual polling cannot replay consumed events.

Origin handles are generation-checked; removing an origin causes its queued
events to be skipped. A selected handle is a historical value and can become
stale independently. Removing a subtree does not mean every language closure
capturing a former child is immediately released; retention differs below.

[Native C button snapshots](../../include/cui/cui_components.h),
`cui_button_click_event`, already include origin, target, event index/type,
key/modifiers/repeat, pointer position/type/button/device/ID/click count and a
copied `cui_activation_snapshot`. `bridge_click` preserves **only node and frame**.
Therefore keyboard/pointer interaction tests do not prove evidence export.
Accessibility-specific queuing/evidence is not a shared public bridge event API.
Never infer origin, pointer coordinates, timestamp, trusted status or modifier
state from the sparse bridge event or current global input.

## Language interfaces actually implemented

| Boundary | Dispatch and handler shape | Payload / ownership |
| --- | --- | --- |
| Python [canonical module](../../bindings/python/cui_bridge/__init__.py) | `App.step()` only steps. `poll_events()` consumes snapshots; `dispatch()` calls `Widget.on_click/on_change/on_close` handlers. `run()` dispatches from native after-frame hook. | Frozen `Event` dataclass, `type: str`, widget reference, integer frame, bool checked, float value, copied string, selected handle tuple. One stored callable per channel; new registration replaces it. Return object ignored. |
| JS [index.cjs](../../bindings/node/index.cjs) and [addon](../../bindings/node/addon.c) | `App.step()` steps **and dispatches**. `_dispatch()` is internal; `run()` dispatches after each native frame. `start(fps)` is a separate timer/manual-step loop. | JS event objects vary by channel: click `{node,frame}`, change adds checked/value/text/selectedHandle, close adds result. Stored closures live on nodes; returns ignored, promises not awaited. |
| TypeScript [index.d.ts](../../bindings/typescript/index.d.ts) | Declarations over the JS implementation, not another runtime. Interfaces restrict supported builders/methods and declare callback returns `void`. | `ClickEvent<T>`, generic `ChangeEvent`, `CloseEvent` with result `1 \| 2 \| 3`. Change payload is not discriminated by component/value kind; no static request/notification distinction. |
| Go [native.go](../../bindings/go/cui/native.go) | `Step` and `PollEvent`/`Dispatch` are separate. `Node.OnClick/OnChange/OnClose(func(Event))` store functions. `Run` dispatches through the native runner. | Value `Event` owns Go string and scalar copies, references `*Node`, includes Type, Frame, Selected, raw SelectedHandle and CloseResult. Callbacks have no return decision. |
| Rust [lib.rs](../../bindings/rust/src/lib.rs) | `step` and `dispatch` separate; `frame` combines them. `on_click` is a no-payload `FnMut()`; `on_change` also has a no-payload convenience form, while `on_change_event/on_close_event` take `&ChangeEvent`. `run_with` adds an after-frame app hook. | Callback map keyed by raw node/channel, `Rc<RefCell<Box<dyn FnMut(&ChangeEvent)>>>`, closures `'static`. Payload owns String and copyable Node tokens; handler borrows payload only for the call, clone if deferring. `checked_raw` preserves the full C integer, unlike boolean conversion. |

Python's type hints/dataclass are not runtime component-specific payload typing.
JS uses runtime kind checks; TypeScript adds compile-time interface restrictions,
not runtime validation of all event payload meanings. Go has one general Node
builder with sticky builder errors and runtime kind checks. Rust's Node token
checks app identity and C validates generation on operations; callback registration
itself does not validate that the node kind can emit that channel. A successfully
registered callback is not proof an event will ever be emitted.

C `checked` is converted to bool in Python, JS and Go. For the data-table column
overload this loses the raw integer; JSON text still includes the column and Rust
also exposes `checked_raw`. This concrete mismatch rules out treating the current
generic record as a lossless typed schema. Rust's `surface_result()` decodes
numeric value, not an independently discriminated surface event. JS frame IDs
are converted from uint64 to Number, losing exactness above `2^53-1`; Python,
Go and Rust retain integer-width precision appropriate to their representations.

All four runtimes use a 4097-byte event text buffer rather than implementing
arbitrary poll-capacity retry. This matches present common text bounds, not an
extensible payload-size guarantee. If future payloads grow, a capacity error can
leave the event queued until a capable consumer drains it.

Dispatch counts are not uniform: Python and Go count consumed events even
without a matching handler; JS and Rust count handler invocations. A step/result
count must not be specified as a cross-language event cardinality contract today.

## Mutation, removal, shutdown and exceptions

Handlers are retained language objects, not C `void *` handlers retained by Core.
They can capture application data for later dispatch; that data must remain
valid according to the language's ownership rules. C's after-frame `user` is
borrowed for the synchronous `cui_bridge_run` duration. The button adapter's
stack `bridge_click_binding` survives its synchronous component invocation only;
its pointer is never published as queued payload.

Python clears/invalidate wrappers on close; JS disposal clears its app node map,
but externally retained nodes can still retain their handler closures. Go removal
marks descendants removed and deletes lookup entries, but a retained `*Node`
continues owning its functions. Rust `remove` removes callbacks for the explicit
node token, while C removes its subtree; descendant callback-map entries can
remain until app teardown. C drops stale queued origins, so retention does not
entitle an old closure to delivery for a reused slot.

Callback-driven property updates and supported wrapper close/disposal have
existing tests. They occur after C step, not inside Core component dispatch.
This does **not** authorize destroying a Core context inside a synchronous native
component callback. Direct C bridge destruction frees the app; a freed pointer
cannot be destroyed again. During `run`, bridge destruction requests close and
defers the single release until runner exit; do not reuse that pointer afterwards.

[cui_bridge_run's public contract](../../include/cui/cui_bridge.h) rejects nested
run/step from its callback. [The SDL runner](../../src/bridge/cui_bridge_sdl.c)
invokes it on the UI thread after a successful completed step; it releases the
native host on exit. Window close is normal `CLOSED`. Python/JS/Go/Rust runner
wrappers catch foreign exceptions/panics, stop C, then rethrow/resume on their own
side. Go retains runner userdata with `cgo.Handle` for that call and pins/checks
the owning OS thread; Rust marks App local with `PhantomData<Rc<()>>`.

Standalone manual `dispatch` is synchronous and exceptions/panics propagate in
the language after the event has been consumed. Remaining events need not be
lost, but the throwing event is not retried automatically. No uniform guard
for recursively dispatching or stepping from a manual handler was found; do not
promise identical nesting or listener-replacement behavior across languages.
Rust's closure borrowing imposes additional restrictions. Main-thread use is
required; this audit does not establish uniform cross-thread enforcement in all
bindings.

`run` is a blocking native callback loop. JS `start` yields through timers but
has different modal-resize behavior. Async callbacks/promises, Python coroutines,
worker completion delivery, cancellation, thread pools and idle-loop wakeups are
not implemented as a common event facility. Blocking work in a handler blocks
its UI thread. Existing speech host-report APIs are not a generic URL-fetch
executor or a guarantee of async shutdown/cancellation semantics.

## Examples and test evidence

The examples under [bindings/bridge/examples](../../bindings/bridge/examples),
[bindings/node/examples](../../bindings/node/examples),
[bindings/go/examples](../../bindings/go/examples) and
[bindings/rust/examples](../../bindings/rust/examples) use the actual retained
bridge/builders. The older [foreign_languages host](../../examples/foreign_languages/host.h)
exports only `cui_example_run(font, frames, count_callback, user)`; its wrappers
run the fixed C host and observe a count. They are host/FFI demonstrations, not
proof of arbitrary component builders or the requested shared event API.
The Node README's initial “row, column, label, button only” statement is stale
against its canonical JS source and component tests; do not derive coverage
from that sentence or from launcher examples.

Relevant existing regression sources (not new execution results):

- [C bridge core](../../bindings/bridge/tests/test_bridge_core.c) and
  [components](../../bindings/bridge/tests/test_bridge_components.c): real node
  operations, queued events, copied state snapshots, capacities and lifecycle.
- [Python bridge tests](../../bindings/python/test_bridge.py): explicit separation
  of step/dispatch, callback mutation, disabled suppression, callback close,
  exceptions outside C and cross-app/disposal checks.
  [Component tests](../../bindings/python/test_components.py) include two text
  changes across frames before draining, proving snapshots rather than getters.
- [JS binding tests](../../bindings/node/test/binding.test.cjs) and
  [components](../../bindings/node/test/components.test.cjs): auto-dispatch from
  step, mutation, close/dispose, disabled controls and change payloads.
- [Go native tests](../../bindings/go/cui/native_test.go):
  `TestChangeSnapshotsAndInput`, `TestCallbackCloseAndThread`,
  `TestKeyboardAndCallbackPanic`, plus component-specific scenarios.
- [Rust builders](../../bindings/rust/tests/builders.rs),
  [callback panic](../../bindings/rust/tests/callback_panic.rs) and
  [TypeScript typecheck fixture](../../bindings/typescript/components.typecheck.ts):
  current builder/payload behavior and negative type examples. Compilation of
  declarations is not proof of runtime dispatch timing.
- Runner lifecycle/error/nesting sources:
  [Python](../../bindings/python/test_run_native_virtual.py),
  [JS](../../bindings/node/test/run-native.test.cjs),
  [Go](../../bindings/go/cui/run_native_test.go),
  [Rust](../../bindings/rust/tests/run_native.rs). Portable unavailable-runner
  tests are separate; neither substitutes for all native host acceptance.

## Cross-check of proposal commit `84376caa`

The separately committed `docs/design/COMPONENT_EVENTS_CONTRACT.md` is explicitly
a proposal, not a shipped API. Its choice of synchronous component-local native
callbacks is consistent with preserving existing C facades. The following
binding qualifications are necessary at the design checkpoint:

- “Callbacks run synchronously at the family's reduction/submission point” must
  not be read as describing current Python/JS/Go/Rust component callbacks. Their
  existing notifications run after step/render. Preserve that timing as a
  documented compatibility adapter; moving them inside C is an architectural
  boundary with exception, reentrancy and ownership implications.
- Submission-borrowed handlers/userdata and no hidden allocation describe new
  immediate facades. Existing language closures are retained, and bridge
  snapshots allocate and queue. They need their own explicit exception, bounds
  and release contract rather than being relabeled as immediate callbacks.
- The proposed request/notification/result distinctions do not yet cross the
  generic bridge record. Native controlled requests cannot be implemented by
  treating an existing post-mutation `CHANGE` handler's return as acceptance.
- Native pointer-free metadata can reuse the existing Button snapshot; present
  bridge payloads omit it. Mark unavailable facts honestly until a new bridge
  payload transports them. No current binding exposes the proposed shared
  propagation/default-prevention/consumption controls.
- Proposed recursive-frame/dispatch restrictions are sensible for new facades,
  but manual language dispatch does not presently have a uniform nesting guard.
  Wrapper close-after-frame behavior also differs from unsupported native
  context destruction during component dispatch. Keep these scopes separate.

No new input-dispatch architecture is established by this audit. The proposal
can retain native local dispatch and existing deferred notifications; the exact
foreign controlled-request boundary still needs a separately reviewed slice.

## Practical future slices for the integrator's proposal

These are implementation boundaries to consider, not an approved contract:

1. Preserve existing notification queue/timing and add a versioned event schema
   with explicit payload tags, owned/copyable strings and complete selected
   handles. Add optional, truthfully marked button evidence copied from the
   existing C snapshot. Specify queue/byte limits, unsupported capabilities and
   retry behavior. Do not reconstruct missing historical evidence.
2. Add component-specific typed language events over that schema, keeping
   compatibility adapters for generic `checked/value/text`. Cover lossless
   table columns, close outcomes, uint64 frame precision, stale selection and
   descendant callback retention. Reading a result must never dispatch a handler.
3. Add proposed-value requests only where the native controlled component
   contract supports them. Acceptance/rejection must execute at its defined
   synchronous decision point; the existing post-frame language notification
   cannot veto past mutation. Reusing it as a request would change semantics.
   Decide architectural/FFI reentrancy requirements before crossing that boundary.
4. Specify aggregation/counts, handler replacement/removal and mutation ordering
   independently from any capture/bubble architecture. Keep legacy timing and
   exception behavior explicit; test multiple interactions, absent handlers,
   queue failure and callback/result coexistence before claiming consistency.
5. Implement the requested optional async host example outside Core: handlers
   enqueue copied requests, UI-thread completions apply state, and shutdown,
   stale replies, logical/transport cancellation and wakeups have deterministic
   injected-transport tests. Current runners alone do not supply this contract.

The binding-specific decision remains open: how to add a controlled request
path while preserving deferred legacy notifications, and whether any such path
requires a new synchronous foreign boundary with explicit ownership/reentrancy
rules. Changing existing callback timing to “make it consistent” is observable
and cannot be treated as a documentation-only or backward-compatible normalization.

## Implemented notification follow-up

The additive [V1 manual notification API](../BRIDGE_NOTIFICATIONS.md) now has owning Python/JavaScript/TypeScript views and [Go/Rust views](../BRIDGE_NOTIFICATIONS_GO_RUST.md). Its initial typed allowlist is activation, applied boolean changes, table changes and surface close outcomes; other payloads remain legacy. It preserves the existing shared queue and delivery timing. These views do not expose synchronous C value requests, Slider phase callbacks or Base Input observers to foreign handlers. Those omissions and the architectural decision above remain applicable.

The separate [opt-in C value poll](../BRIDGE_VALUE_NOTIFICATIONS.md) additionally
supports scalar, plain-text and selection snapshots for its explicit producer allowlists.
[Python](../BRIDGE_VALUE_NOTIFICATIONS_PYTHON.md) and
[JS/TypeScript](../BRIDGE_VALUE_NOTIFICATIONS_JS.md) and
[Go](../BRIDGE_VALUE_NOTIFICATIONS_GO.md) and
[Rust](../BRIDGE_VALUE_NOTIFICATIONS_RUST.md) owning value polling are implemented;
Rust static builds require the paired SDK, while dynamic builds negotiate optional exports;
existing V1 methods keep their initial allowlist and receive LEGACY snapshots for
these categories. Consumers must negotiate the new optional symbols and category
mask rather than enabling reserved tags through the old method.


## Current source snapshot — 2026-10-18 (`9ba9c78c`)

This section supersedes historical **current availability** claims above, not the
baseline's evidence. Read-only inspection of canonical C, Python, JS/TypeScript,
Go and Rust sources/tests/docs; no suites rerun for this documentation update.
All 78 manifest entries remain present. SDK header distribution, a component
builder and a passing legacy UI test each establish different facts; none means
all direct-C observer semantics are implemented in a foreign runtime.

### Two manual protocols over one queue

| API boundary | Complete typed producer eligibility | Not supplied |
| --- | --- | --- |
| Initial `cui_bridge_poll_notification_v1` | Existing CLICK → ACTIVATE/NONE; Checkbox/Switch/Toggle → CHANGE/BOOLEAN; DataTable existing selection/sort enqueue → CHANGE/TABLE; existing surface/Notification CLOSE → LIFECYCLE/SURFACE | Every other CHANGE remains LEGACY, including producers that the separate value method can decode. No input evidence/index/source or synchronous veto. |
| Opt-in `cui_bridge_poll_value_notification_v1` | Initial categories plus requested/negotiated SCALAR (Slider, NumberInput, Rating, Pagination), TEXT (Input, Textarea, Editor, Input Group), SELECTION (Radio group, Tabs, Select, List, Stepper) | Unrequested categories, unsupported producers and mask0 stay complete LEGACY. No automatic mapping from native typed facades. |

The exact [initial contract](../BRIDGE_NOTIFICATIONS.md), [value contract and
allowlists](../BRIDGE_VALUE_NOTIFICATIONS.md), [C declarations](../../include/cui/cui_bridge.h)
and [enqueue implementation](../../src/bridge/cui_bridge.c) are authoritative.
Both methods consume the same FIFO as old extended/filtered polling and dispatch;
there is no duplicate queue, subscription/fanout, implicit getter or replay.
Polling only owns a snapshot and never invokes a handler. Filtered old CLICK polling
still discards non-CLICK records. Capacity failure preserves the live record;
stale-origin cleanup before it is not rolled back. Queue-full error can follow
already-applied model mutation. Neither API is a transaction or request decision.

Initial schema/size/version checks remain mandatory; value polling additionally
negotiates optional value-category and poll exports and requests only compiled-known
advertised bits. Old libraries remain usable through old methods; missing/incompatible
optional exports fail explicitly **before draining**, without fallback to another
consumer. Malformed records detected after successful consuming C reads cannot be
restored. Genuine older-library no-drain tests and injected malformed-provider tests
prove different properties and must be reported separately.

Typed table row/column/sort values are exact uint32 one-based bridge indices (zero
absent), not Core IDs or C observer operation keys. BOOLEAN reports already-applied
state, not immutable entry/requested-value controlled Check/Switch/Toggle semantics.
SCALAR copies binary64: NumberInput retains its actual double, while Slider/Rating
cannot regain precision lost in their float models. TEXT is the existing aggregate
plain UTF-8 snapshot, not each edit/commit/submit, and inherits no native secure
redaction. The bridge exposes no secure/password configuration; OTP remains Legacy.
SELECTION holds historical Item slot/generation, not an owned live node or a
per-transition native selection event. Select's net-zero frame can remain silent;
List can repeat the same item on activation. Menu/Dropdown/Combobox retain Legacy
classification even when old fields contain text or selected handles.

### Canonical owning wrapper entrypoints

| Language | Initial manual API / opt-in value API | Ownership, precision and stepping |
| --- | --- | --- |
| Python | `App.poll_notifications_v1()` / `App.poll_value_notifications_v1()` iterators | Frozen dataclass variants with owned strings, exact Python integer frame and uint32 handle tuples; optional live Widget references are not immutable model ownership. `step()` does not dispatch; manual iterator consumes only when advanced. |
| JavaScript | `App.poll_notification_v1()` / `App.poll_value_notification_v1()` → record or `undefined` | Frozen discriminated records, copied strings/handle objects, exact uint64 `frame: bigint`; optional Node references can later become invalid. Use `step_only()` to avoid legacy automatic dispatch by `step()`/`run()`. |
| TypeScript | `NotificationV1` / separate `ValueNotificationV1` unions over those JS methods | Runtime is the Node addon, not another implementation. Readonly/discriminated declarations require BigInt-capable tooling; typecheck evidence is not runtime proof. |
| Go | `App.PollNotificationV1()` / `App.PollValueNotificationV1()` → value, present bool, error | Separate kind enums and kind-selected typed fields; strings/uint64 frame/uint32 handle are copied, optional `*Node` remains live. `Step` and `Dispatch` remain separate. |
| Rust | `App::poll_notification_v1()` / `App::poll_value_notification_v1()` → `Result<Option<...>>` | Separate exhaustive owning enums, `String`, u64 frame and u32 handles; tokens do not own live native nodes. `step` and `dispatch` remain separate (`frame` combines them). Static Rust requires paired SDK; dynamic optional exports are negotiated. |

Representative canonical references:
[Python methods/types](../../bindings/python/cui_bridge/__init__.py),
[JS manual methods](../../bindings/node/index.cjs),
[Node FFI/protocol checks](../../bindings/node/addon.c),
[TypeScript unions](../../bindings/typescript/index.d.ts),
[Go owning decoder](../../bindings/go/cui/native.go),
[Rust owning decoder](../../bindings/rust/src/lib.rs).
Language-specific [Python](../BRIDGE_VALUE_NOTIFICATIONS_PYTHON.md),
[JS/TS](../BRIDGE_VALUE_NOTIFICATIONS_JS.md), [Go](../BRIDGE_VALUE_NOTIFICATIONS_GO.md)
and [Rust](../BRIDGE_VALUE_NOTIFICATIONS_RUST.md) guides specify exact errors.

All four owning manual decoders start with 4097 bytes and allow one strictly larger
capacity retry, capped at 1 MiB **including NUL**. This replaces the historical
fixed-buffer limitation for these manual methods only; it does not rewrite old
legacy dispatch buffers, bool conversions or JS Number frame behavior. No silent
truncation. Copies outlive queue reuse, later model edits/removal and app teardown;
optional node resolution uses complete slot+generation and must not resolve a
historical selection to a replacement slot occupant. Record immutability does
not freeze a referenced node. Handler retention/exception/timing differences in
the historical dispatch audit remain separate from manual owning records.

### All 78 families: explicit omission boundary

This partition checks the exact names in [gpui-parity.json](../gpui-parity.json),
not every possible enqueue branch. The rows below account for all 78 once. Common
initial ACTIVATE/NONE can also originate from existing simple CLICK adapters in
other rows; it does not grant an extra typed component-specific payload. A row
with no dedicated typed value can still emit Legacy/CLOSE/CLICK through existing
adapters, host interactive children, be noninteractive, or lack a foreign node
representation. **No notification emission promise is inferred from membership.**

| Manifest families (disjoint accounting) | Current dedicated typed view / omission |
| --- | --- |
| Button | Initial ACTIVATE/NONE for existing CLICK; native accepted pointer/key evidence and per-input handler semantics are not exported. |
| Checkbox, Switch, Toggle | Initial applied BOOLEAN; native controlled request/readonly/entry snapshot facades are not exported. |
| DataTable | Initial TABLE at existing selection/sort enqueues only; newer native set/span workspace, per-transition multiplicity, stable uint64 keys, column width/move, activation/context/mode requests are not exported. |
| AlertDialog, Dialog, Sheet, Popover, Notification | Initial SURFACE at existing CLOSE enqueues; not the retained native completed-CLOSED observer registration/update/lifetime contract, synchronous decide/veto or general lifecycle stream. |
| Slider, NumberInput, Rating, Pagination | Opt-in SCALAR only; native phases/reasons/before-after/counts/commit policies are not exported. |
| Input, Textarea, Editor, Input Group | Opt-in aggregate plain TEXT only; native per-edit payloads/selection/redaction, Input Group addon observer details and submit/commit are not exported. |
| Radio, Tabs, Select, List, Stepper | Opt-in SELECTION applies only to the bridge's RadioGroup producer (catalog Radio), Tabs, Select, single List and Stepper; not every radio item, Listmulti/ToggleGroup arrays or native per-transition facade. |
| Accordion, Alert, Attachment, Avatar, Badge, Bubble, Clipboard, Collapsible, DropdownButton, Empty, Focus Trap, GroupBox, HoverCard, Icon | No additional dedicated typed value allowlist. Existing producer-specific Legacy/common initial records only where implemented; native family observers/results/host services do not automatically cross this bridge. |
| Image, Kbd, Label, Marker, Message, MessageScroller, Progress, Shimmer, Skeleton, Spinner, Tag, Tooltip, Calendar, ColorPicker | No additional dedicated typed value allowlist. Existing producer-specific Legacy/common initial records only where implemented; native family observers/results/host services do not automatically cross this bridge. |
| Combobox, DatePicker, Form, OtpInput, Questionnaire, Speech, TimeField, Carousel, DescriptionList, Dock, Menu, Resizable, Root View, Scrollable | No additional dedicated typed value allowlist. Existing producer-specific Legacy/common initial records only where implemented; native family observers/results/host services do not automatically cross this bridge. |
| Settings, Sidebar, StatusBar, Theme, TitleBar, Toolbar, Tree, VirtualList, Chart, Command, Plot, Table, TextView | No additional dedicated typed value allowlist. Existing producer-specific Legacy/common initial records only where implemented; native family observers/results/host services do not automatically cross this bridge. |

Command, Toolbar, Sidebar, Scrollable, HoverCard and Tooltip in that remainder now have direct-C typed facades,
as do numerous other families audited in their guides. That does not add foreign
handlers, direct-C synchronous execution timing or callback veto. Tree activation,
retained surface completion, Command early query/confirmed-path semantics, Toolbar
aggregate ambiguity and Sidebar collapse details all remain native contracts.
Shared C metadata's input source/index/activation evidence is absent from bridge V1
beyond captured frame presence. No capture/bubble/default-prevention layer, foreign
controlled request, synchronous lifecycle veto or async completion protocol is
implied. Native render/match/content and retained lifecycle callbacks additionally
have distinct lifetime rules that cannot be replaced by a queued notification.

### Inspected tests versus executed historical evidence

Current real-queue C tests:
[initial V1](../../tests/unit/test_bridge_notifications.c),
[scalar](../../tests/unit/test_bridge_value_notifications.c),
[text](../../tests/unit/test_bridge_text_notifications.c),
[selection](../../tests/unit/test_bridge_selection_notifications.c).
Owning wrapper tests are
[Python initial](../../bindings/python/test_notifications_v1.py)/[values](../../bindings/python/test_value_notifications_v1.py),
[Node initial](../../bindings/node/test/notifications-v1.test.cjs)/[values](../../bindings/node/test/value-notifications-v1.test.cjs),
[Go initial](../../bindings/go/cui/notifications_v1_test.go)/[values](../../bindings/go/cui/value_notifications_v1_test.go),
[Rust initial](../../bindings/rust/tests/notifications_v1.rs)/[values](../../bindings/rust/tests/value_notifications_v1.rs).
They distinguish real queue FIFO/interleaving, copied text, table columns above 1,
stale selected generations and later setters from synthetic protocol values.
Node [initial](../../bindings/node/test/notifications-v1-protocol.test.cjs)/[value](../../bindings/node/test/value-notifications-protocol.test.cjs)
and language-specific providers cover malformed negotiation/capacity/union output;
maximal uint64 frames and oversized synthetic payloads are **not** real UI proof.

Observed historical evidence, not a fresh run: integrator's offline Linux
[`test_notification_bindings.py`](../../scripts/test_notification_bindings.py)
log `/tmp/cui-next39-rust-value-bindings.log` records source
`4aa3c3d02bac4a5b3b24320d08790a667fbc6ce3`, shared bridge SHA256
`fa9cb17db1317ac3c3ae23959f4091ca12f5f85dc5db80cdb5ed4f1d1082b946`,
**52/52 passed, 122.78 s**. The integrator separately recorded unchanged library
hash before/after. This precedes later native component-event slices; it is not
validation of current `9ba9c78c` binaries, Python/Node runtime cases, macOS/Windows,
native SDL lifecycle windows or all 78 family semantics.

That 52-case configuration includes four Go/Rust static/dynamic real+legacy suites
with `CUI_VALUE_REAL=1`, fourteen initial protocol cases (seven modes per language),
two genuine pre-V1 unavailable cases, 26 value protocol cases (13 separate provider
modes per language), and six genuine initial-V1/optional-unavailable no-drain cases.
Older-library flags are required for those optional cases; a shorter run with
skips/omitted flags is not 52-case proof. Rust static uses paired SDK; injected and
older-library interposition remain dynamic only. The runner disables downloads,
checks SDK source freshness, builds separate strict C11 providers and uses private
`test_local.py`; it does not run the separate native window lifecycle suite.

No suite was executed and no manifest status was promoted by this documentation
refresh. Future foreign typed/controlled/lifecycle work needs its own reviewed
protocol, exception/reentrancy/storage and real-input tests; native source-header
availability alone is insufficient.

## Avatar observer addition

Direct C single-Avatar and visible-stack activation observers are implemented in
[cui_avatar_events.h](../../include/cui/cui_avatar_events.h). Python, JS/TS, Go and
Rust do not yet expose these synchronous observer handlers or stack payloads.
Avatar style setters are a separate retained binding feature, not event parity.
See [the observer guide](../AVATAR_EVENTS.md) for timing and borrowed lifetimes.

## Alert completed-dismiss observer

Direct C `cui_alert_events` is a synchronous coalesced observer after the existing
optional visibility assignment. Python, JS/TS, Go and Rust do not expose this
handler/result facade or per-input Dismiss events. The retained bridge Alert is
presentational; its arbitrary child actions remain separate. See
[the guide](../ALERT_EVENTS.md); no foreign close/veto support is implied.

## Bounded Button styles and extended HoverCard policy

Direct C `cui_button_styled_ex`/`cui_button_styled_paint` add borrowed pure paint handlers; these are not interaction observers and are not exposed by Python, JS/TS, Go or Rust wrappers. Existing event wrappers and Group do not accept these new style records. See [Button styles](../BUTTON_STYLES.md).

Direct C `cui_begin_hover_card_extended` combines the existing lifecycle observer with an opt-in caller-accepted tap policy and semantic relationship snapshots. Existing foreign HoverCard wrappers retain their legacy behavior; no policy workspace or synchronous callback bridge is exposed. See [HoverCard extended policy](../HOVER_CARD_EXTENDED.md).
