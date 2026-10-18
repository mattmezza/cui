# Proposed versioned bridge notification views

Status: design only at `e7202949`. Nothing below is shipped. This extends the [binding audit](COMPONENT_EVENTS_BINDINGS.md) and [event contract](COMPONENT_EVENTS_CONTRACT.md), without synchronous foreign request hooks, callback retiming, global routing or 78-family parity claims. Root reviews ABI/export ownership before implementation.

## Preserve the actual boundary

The [C bridge](../../include/cui/cui_bridge.h) queues snapshots while components reduce/mutate. Foreign callbacks run after the completed step/render, not inside native component dispatch. Preserve FIFO **enqueue order**, scene traversal, per-family aggregation and programmatic notifications. Do not reinterpret notification returns as acceptance or claim chronological input order across components.

Concrete source problems motivating an additive view:

- [`bridge_data_table_change`](../../src/bridge/cui_bridge.c) snapshots row, column, sort column/direction and reason into JSON, while putting column in `checked` and row in `value`. [Python](../../bindings/python/cui_bridge/__init__.py), [Node addon](../../bindings/node/addon.c) and [Go](../../bindings/go/cui/native.go) convert checked to bool, losing columns above one. [Rust](../../bindings/rust/src/lib.rs) retains `checked_raw`, but still lacks a typed table event. Fix the new view; retain legacy bool behavior for compatibility.
- Node uses `napi_create_double((double)frame_sequence)` and TypeScript declares `frame: number`; exact uint64 identities above `2^53-1` are lost. A new `frame: bigint` is required, while old events keep Number. Frames are context-local identities, not wall time or global order.
- Every wrapper currently polls into 4097 bytes. C `poll_event_ex` supports capacity retry, but wrappers do not generally implement it. Text is an owned queued snapshot, not necessarily the latest getter value. Preserve that snapshot across delayed delivery.
- A supported origin handle can remain queued while a selected-item handle becomes stale. Selected slot/generation is historical evidence, not an owning live node reference; never resolve it to a reused slot without generation checks.

## Exact additive C surface proposed

Keep `CUI_BRIDGE_ABI_VERSION`, existing options/layouts, `cui_bridge_event`, `cui_bridge_event_ex`, polling symbols and statuses unchanged. Introduce an independently versioned schema with fixed-width discriminators; public enums are named constants, not ABI-sized enum fields. This V1 layout is frozen once shipped. Future larger layouts use a new suffix and capability bit, not extension writes into old caller buffers.

```c
#define CUI_BRIDGE_NOTIFICATION_V1 1u
#define CUI_BRIDGE_NOTIFY_ACTIVATE 1u
#define CUI_BRIDGE_NOTIFY_CHANGE 2u
#define CUI_BRIDGE_NOTIFY_LIFECYCLE 3u
#define CUI_BRIDGE_PAYLOAD_NONE 0u
#define CUI_BRIDGE_PAYLOAD_BOOLEAN 1u
#define CUI_BRIDGE_PAYLOAD_SCALAR 2u
#define CUI_BRIDGE_PAYLOAD_TEXT 3u
#define CUI_BRIDGE_PAYLOAD_SELECTION 4u
#define CUI_BRIDGE_PAYLOAD_TABLE 5u
#define CUI_BRIDGE_PAYLOAD_SURFACE 6u
#define CUI_BRIDGE_PAYLOAD_LEGACY 7u
#define CUI_BRIDGE_NOTIFY_HAS_FRAME 1u
#define CUI_BRIDGE_TABLE_SELECTION 1u
#define CUI_BRIDGE_TABLE_SORT 2u

typedef struct {
    uint32_t reason, row, column, sort_column, sort_direction;
} cui_bridge_table_notification_v1;
typedef union {
    uint32_t boolean_value; /* exactly 0 or 1 */
    double scalar_value;
    cui_bridge_node selected;
    cui_bridge_table_notification_v1 table;
    uint32_t surface_result; /* existing ACCEPTED/CANCELLED/CLOSED values */
} cui_bridge_notification_value_v1;
typedef struct {
    uint32_t struct_size, schema_version;
    uint32_t kind, payload_kind, flags, node_kind;
    cui_bridge_node node;
    uint64_t frame_sequence;
    uint32_t text_length; /* source bytes, not including copied NUL */
    cui_bridge_notification_value_v1 payload;
    cui_bridge_event_ex legacy; /* complete old scalar/handle snapshot */
} cui_bridge_notification_v1;

CUI_BRIDGE_API uint32_t cui_bridge_notification_v1_size(void);
CUI_BRIDGE_API uint32_t cui_bridge_notification_versions(void); /* bit0: V1 */
CUI_BRIDGE_API cui_bridge_status cui_bridge_poll_notification_v1(
    cui_bridge_app *, uint32_t struct_size, cui_bridge_notification_v1 *out,
    char *text, uint32_t capacity, uint32_t *required_capacity);
```

Caller passes exact `struct_size`; mismatch/null arguments return existing `INVALID_ARGUMENT` before consuming anything or changing outputs. The successful output contains size/version for defensive FFI checks. Empty union members are zeroed; consumers read only the payload selected by `payload_kind`. Size query includes platform ABI padding; do not manually pack structs. Each language FFI checks its native layout against the query before first use. Capability discovery is optional-symbol lookup: old libraries lacking these symbols keep working for old APIs, while explicit V1 registration/poll reports unsupported cleanly rather than silently decoding V1 from legacy values.

V1 uses bridge handles, not Core `cui_id`: they are different identity domains. It deliberately exposes **no event index, pointer/device/key source or activation snapshot** in this first bridge slice. Its only metadata presence bit is `HAS_FRAME`, set when the captured frame sequence is nonzero. Programmatic operations outside a frame leave it clear; do not invent a frame or source. A later evidence transport needs its own versioned layout and enqueue-time capture. The shared native metadata helper does not recover evidence already discarded by the bridge.

`legacy` preserves every existing numeric/raw selected field. The same returned UTF-8 text serves both typed and legacy views. No getter or JSON reparsing reconstructs the new payload at poll time: private queued records capture typed fields and node kind at the original enqueue point, alongside the old snapshot. Array slots remain bounded by existing `max_events` (default256/max4096); typed payloads add fixed per-slot storage, not another queue or hidden event fanout.

Initial supported mapping is intentionally narrow:

| Existing event source | V1 kind/payload | Boundary |
| --- | --- | --- |
| Any existing CLICK enqueue | ACTIVATE/NONE | Preserve each existing enqueue, including simple click adapters lacking input evidence. |
| Checkbox/Switch/Toggle committed CHANGE | CHANGE/BOOLEAN | Already-applied bridge-owned bool, not a request or tri-state. |
| Scalar numeric control CHANGE | CHANGE/SCALAR | Only explicitly audited numeric producers, not every overloaded `value` field. |
| Input/Textarea/Editor/OTP text CHANGE | CHANGE/TEXT | Owned UTF-8 snapshot; does not add submit/completion channels. |
| Single selected-item CHANGE | CHANGE/SELECTION | Full captured handle, which may be stale by delivery. Multi-selection remains legacy until a bounded typed representation is designed. |
| DataTable selection/sort | CHANGE/TABLE | Capture all five fields directly where JSON is currently constructed; preserve sentinel values and raw legacy/JSON unchanged. |
| Existing surface CLOSE | LIFECYCLE/SURFACE | Existing close result, not an application veto. |
| Every other existing notification | Existing channel/LEGACY | Preserve generic fields/text. Date/time/color/questionnaire/speech/dock/scroll and other overloaded producers are not guessed into scalar/text semantics. |

Exact numeric/selection producer allowlists and their fixture names must be committed before enabling those mappings. Unsupported variants still deliver lossless LEGACY payloads. No notification is synthesized where none previously existed, and setters retain their individual event/no-event behavior.

## Polling, copying and failure

V1 and both old poll functions drain the **same** queue. Mixing them consumes whichever event is next; this is not broadcast or independent cursors. `poll_event` retains its existing skip/filter behavior. Typed polling skips stale origin handles just as `poll_event_ex` does; it does not drop a valid origin solely because the selected handle is stale.

Success copies complete text including NUL, publishes the fixed output and dequeues once. `CAPACITY` sets only required capacity and leaves destination/fixed output and the live event unchanged; stale-origin cleanup may already have occurred. Allocation/copy failure must never deliver a misleading partial typed payload. Existing enqueue failures can follow component mutation; step is not transactional and no rollback claim is added.

Preserve existing producer limits (common text payloads fit4096 bytes). Do not impose a new4096 truncation in polling merely because wrappers historically used4097. A V1 wrapper starts with4097 and retries the same event only after `CAPACITY`, using the returned required size and a documented new-view safety ceiling proposed as **1MiB including NUL**. This is an explicit wrapper allocation policy, not a claim about all existing producer limits. Greater required sizes raise a typed payload-limit error without consuming that event; users can drain it through a capable C consumer or close the app. Reject zero/overflowing required size and a second inconsistent capacity response rather than spinning. Queue count, transient wrapper allocation and text bytes must be reported separately.

C text belongs to caller storage after polling. Python/JS/Go/Rust publish owning strings and scalar/handle copies; callbacks receive owned immutable/read-only records or callback-borrowed references to them. Deferred consumers retain/copy the language object as appropriate; no queued C text pointer crosses FFI. Node references are optional convenience resolution and never replace the captured handle.

## Exact language entry points proposed

Use a discriminated owning `NotificationV1` in each language with variants `Activation`, `BooleanChange`, `ScalarChange`, `TextChange`, `SelectionChange`, `TableChange`, `SurfaceClose`, `Legacy`. Every variant includes captured origin handle, optional live node, exact frame plus presence, and complete `legacy` fields/text. TableChange exposes integer `row`, `column`, `sort_column`, `sort_direction`, `reason`; selection exposes the raw selected handle plus optional validated live node.

```python
# Frozen dataclass union; Python ints preserve uint32/uint64, text is str.
NotificationV1 = Activation | BooleanChange | ScalarChange | TextChange | SelectionChange | TableChange | SurfaceClose | LegacyNotification
App.poll_notifications_v1(self) -> Iterator[NotificationV1]
Widget.on_notification_v1(self, callback: Callable[[NotificationV1], None] | None) -> Widget
```

```ts
// JS runtime implements the same names; uint32 fields use exact Number.
type NotificationV1 = ActivationV1 | BooleanChangeV1 | ScalarChangeV1 |
  TextChangeV1 | SelectionChangeV1 | TableChangeV1 | SurfaceCloseV1 | LegacyV1;
interface NotificationBaseV1 {
  readonly schemaVersion: 1; readonly kind: string;
  readonly handle: Readonly<{slot: number; generation: number}>;
  readonly node?: Node; readonly hasFrame: boolean; readonly frame: bigint;
  readonly legacy: Readonly<LegacySnapshotV1>;
}
interface TableChangeV1 extends NotificationBaseV1 {
  readonly kind: 'table-change'; readonly reason: 'selection' | 'sort';
  readonly row: number; readonly column: number;
  readonly sortColumn: number; readonly sortDirection: number;
}
// Other union members declare only their typed bool/number/text/handle/result.
App.prototype.poll_notification_v1(): NotificationV1 | undefined;
Node.prototype.on_notification_v1(callback: ((e: NotificationV1) => void) | null): this;
```

```go
// Tagged owning value; typed payload structs, uint64 Frame, uint32 table fields.
type NotificationV1 struct {
    Kind NotificationKindV1; Handle Handle; Node *Node; Frame uint64; HasFrame bool
    Legacy LegacySnapshotV1
    Boolean *BooleanChangeV1; Scalar *ScalarChangeV1; Text *TextChangeV1
    Selection *SelectionChangeV1; Table *TableChangeV1; Surface *SurfaceCloseV1
}
func (a *App) PollNotificationV1() (NotificationV1, bool, error)
func (n *Node) OnNotificationV1(callback func(NotificationV1)) *Node
// Exactly the Kind-selected field is nonnil; constructors are wrapper-owned.
```

```rust
// Owned enum with common MetadataV1 { handle, node: Option<Node>, frame: u64,
// has_frame: bool, legacy: LegacySnapshotV1 }; strings are owned String.
pub enum NotificationV1 {
    Activation(MetadataV1), BooleanChange(MetadataV1, bool),
    ScalarChange(MetadataV1, f64), TextChange(MetadataV1, String),
    SelectionChange(MetadataV1, RawHandle), TableChange(MetadataV1, TableChangeV1),
    SurfaceClose(MetadataV1, SurfaceResult), Legacy(MetadataV1),
}
pub fn poll_notification_v1(&mut self) -> Result<Option<NotificationV1>>;
pub fn on_notification_v1<F>(&mut self, node: Node, callback: F) -> Result<()>
where F: FnMut(&NotificationV1) + 'static;
pub fn clear_notification_v1(&mut self, node: Node) -> Result<()>;
```

These are new methods; do not change existing Event/ChangeEvent fields or callback signatures. JS BigInt cannot be JSON.stringify'd directly: document a caller-selected decimal-string serializer for the new view, never silently coerce it to Number. TS fixtures target an explicit BigInt-capable library/runtime. Python requires no arbitrary-integer shim. Go/Rust retain native uint64.

## Dispatch, handlers and compatibility

Old manual poll methods remain unchanged and consume without callback dispatch. New manual poll does likewise. Registering a new notification handler enables typed draining for normal dispatch, but not a second queue: one native V1 dequeue produces the existing legacy language object plus the typed object. Invoke a currently registered legacy channel callback first, then the new notification callback for that event, then advance to the next event. If no new handlers exist, preserve the old drain path. Typed registrations on an old library fail immediately before consuming anything.

This preserves existing Python/Go step-versus-dispatch separation, JS step auto-dispatch and Rust step/dispatch/frame distinction. Run remains after successful frame/render. Existing dispatch return counts retain each language's current legacy meaning; do not add typed listener invocations into them or claim they are portable event counts. If a future caller needs typed-delivery counts, add a separately named result later.

One retained new callable per node; registering replaces it, passing None/null/nil or Rust clear removes it. Snapshot the chosen callable before invoking it. Legacy callback may alter the new registration: lookup the new handler **after** the legacy callback, so replacement/removal applies to the same event, matching ordinary sequential application mutation rather than invoking a freed closure. A legacy exception stops before the new callback; the consumed event is not replayed. A new callback exception likewise stops dispatch after consumption. Promises/coroutines are not awaited; callback returns cannot reject already-applied state.

After each callback, revalidate app-open and origin handle before invoking another callback. A legacy removal/close suppresses typed delivery for that event; do not resolve a replaced slot to a new node. During native run retain its existing deferred-close/rethrow contract. Standalone dispatch close retains each wrapper's supported behavior; raw C destruction is never called twice.

New-handler closures are retained language objects until replacement, explicit clear, removal or app teardown. Implementation must clear new registrations for the entire removed subtree; do not copy Rust's existing descendant-retention omission into the new map. Do not silently change legacy handler ownership in the same commit. External references to removed Node objects may still retain their *legacy* handlers per existing behavior. App identity/generation checks precede registration. Active callback local ownership keeps the current callable alive through return even if it unregisters itself.

Manual recursive dispatch/step semantics currently differ. Do not change legacy calls to add a global guard in this slice. New typed-only APIs reject nested typed polling/draining during their own dispatch with the language's existing invalid-state error; this is a local adapter guard, not proof every legacy nesting path is safe. The exact mixed legacy-dispatch nesting behavior needs explicit compatibility tests before enabling typed handlers there. If preserving it cannot be demonstrated, split automatic fanout integration for review; ship manual V1 polling first rather than retiming callbacks or weakening ownership rules.

## Independently reviewable implementation slices and proof

1. **C V1 queue snapshot/poll only:** ABI size/offset probe for unchanged old structs; exact new-size negotiation; unknown/malformed arguments; old/new poll interleaving, stale origin/selected handles, CAPACITY retry/no partial outputs, FIFO snapshots across later value mutations, queue/allocation failure without false rollback. DataTable columns0/1/2/large index and sort transitions prove no bool loss. Table typed snapshots match old JSON at enqueue time without parsing it at delivery.
2. **One wrapper at a time, manual typed poll:** each existing legacy test remains unchanged. Offline native/test-double payloads exercise uint64 frames at `2^53-1`, `2^53`, `2^53+1`, `UINT64_MAX`, full handles/generations, empty/multibyte/4096-byte text, capacity retry/limit/malformed-required failures and retained snapshots after later frames/removal. Node must assert `typeof frame === 'bigint'` and exact value; no loop running billions of frames. C test doubles/injected conversion fixtures provide the edge values without production test hooks.
3. **Retained typed registration/fanout per wrapper:** legacy-first order, replacement/clear/self-clear, subtree removal, app close, exception/panic, missing handlers and independent legacy counts. Preserve programmatic event/no-event behavior and run/manual-step differences. Explicitly test mixed legacy/new nested calls and stop for architectural review if current behavior cannot remain safe.
4. **Typed producer expansions later:** date/time/color, multi-selection, questionnaire/speech/dock and optional Button input evidence require separate bounded schema/capture tests. Do not advertise them from generic JSON or latest getters. No synchronous requests in this plan.
5. **Docs/export proof:** update human and LLM guides, wrapper declarations, native symbol/version capability checks, distribution/SDK generator manifests and consumer examples together. Native tests use the invisible wrapper; browser tests run headless. Freeze dependencies, no desktop sessions or network required.

A manual V1 poll and two concrete lossless conversions are useful before all wrappers or payload families are complete. Track those omissions explicitly; notification typing does not grant source-controlled rejection, uniform callback counts, cross-language reentrancy or full family parity.
