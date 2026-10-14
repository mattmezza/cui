# Button activation snapshot and propagation proposal

This is a read-only proposal based on CUI `ee9947b4`, component source commit
`87d10ae5e1299d1be18670a37570c5402f952e34` (v0.7.1), and its exact published
`gpui-pre = 0.3.8` dependency. It authorizes no production changes and certifies
no family or platform parity. The dependency was inspected from
https://crates.io/api/v1/crates/gpui-pre/0.3.8/download .

## Actual source contracts

| Contract | Pinned evidence | Consequence |
| --- | --- | --- |
| Public Button callback | component `button/button.rs:447–451, 840–851` | Handler gets ClickEvent; loading suppresses it and stops propagation. |
| Mouse payload | dependency `interactive.rs:148–162, 185–200, 220–226` | Both down/up positions, modifiers, button and counts; down includes first_mouse. |
| Mouse down latch | dependency `elements/div.rs:2960–2968` | Each hovered listener latches a qualifying bubbling down, not just the deepest target. |
| Release and callback phase | dependency `elements/div.rs:3083–3128` | Capture phase removes pending down if no longer hovered; bubble phase invokes listeners with copied down/up. |
| Mouse classification | dependency `interactive.rs:348–402` | Helpers compare both buttons; actual div callback selection uses the down button. Do not silently infer a matching-button rule from the helper. |
| Keyboard activation | dependency `elements/div.rs:3018–3079` | Unmodified Enter/Space down records focus generation; unmodified activation-key up fires if generation matches. Other key events clear/replace pending state. |
| Keyboard payload | dependency `interactive.rs:264–271, 308–332` | Button and rendered hitbox bounds; helper position is bottom-left, modifiers are empty. |
| Focus ABA guard | dependency `window.rs:2338–2369` | Actual focus changes/blur increment generation; moving away and back cannot restore a pending press. |
| Signed tab order | component `button/button.rs:472–485, 816–818`; dependency `tab_stop.rs:34–57, 78–98, 108–170` | Signed lexicographic group paths, insertion-order tie break, independent stop boolean and cyclic next/previous. Negative order is not HTML-style exclusion. |
| Touch type versus runtime | dependency `interactive.rs:275–296`; `gestures.rs:751–766`; `window.rs:5718–5723` | TouchClickEvent exists, but this runtime dispatches taps through synthesized MouseDown/Up with both positions equal to release position and default modifiers. No runtime construction of ClickEvent::Touch was found. |
| Long press | dependency `window.rs:5735–5763`; `interactive.rs:377–402` | Runtime uses separate LongPressEvent dispatch. The TouchClickEvent type's long_press member does not prove Button primary on_click receives it. |
| First focusing click | dependency macOS `window.rs:2832–2845, 3521–3526` | AppKit accepts_first_mouse sets host state, then the next left down carries it. It is not a Core semantic-focus-change heuristic. |
| Propagation ordering | dependency `window.rs:5473–5477, 5805–5826` | Each raw event resets propagation/default state; mouse listeners run registration order in capture, reverse order in bubble, stopping on the live propagation flag. This is not merely an ancestor callback list. |

Two source quirks matter. The keyboard pending value stores a generation, not the
activation key: qualifying Enter then Space downs can leave a generation that an
Enter up uses. Mouse click listener selection follows the down button even if an
up button differs. A CUI policy that improves these rules must be documented as
a deliberate difference, not silently called source equivalence.

The source keyboard hitbox is captured by listeners during paint and used from
`rendered_frame` during input dispatch. It is the actually rendered event-time
rectangle, not geometry from a subsequent composition. Consequently, Core's own
previous rendered routing snapshot is the correct authority for this field;
calling a new layout or asking the host for approximate bounds is unnecessary.

## Current CUI boundary

CUI routes the entire queued batch during begin_frame against previous clipped,
ordered regions (`src/core/cui.c:1238–1400`). Primary capture records only an ID.
Enter activates on nonrepeat key-down; Space activates on matching focus at
key-up. Modifiers remain compatible with existing CUI behavior. Ordinary Tab
uses submission order and treats semantic tab_index as an enabled marker
(`979–997`, `3779–3786`), not signed source ordering.

Current semantic registration accepts individual activation slots; the read-only
accepted query validates current disabled/modal/accessibility metadata
(`2180–2234`, `3429–3450`). `ee9947b4` adds copied release/key/accessibility Button
payloads and preserves every accepted event index, legacy-then-new order, and
Group on_change override. It does not retain a mouse down sample, source keyboard
bounds, first_mouse, long press, or event-local capture/bubble dispatch. Its
payload already copied before legacy notification remains valid even if the
legacy callback cooperatively consumes that accepted event.

## Slice 1: bounded snapshot API, recommended next production step

Keep existing activation timing, focus routing, event consumption and callback
policy unchanged. Add a pointer-free Core value and read-only query:

```c
typedef struct {
    float x, y;
    unsigned modifiers, click_count;
    cui_pointer_type type;
    cui_pointer_button button;
    uint64_t contact, device;
    bool first_focus_known, first_focus;
} cui_pointer_activation_sample;
typedef struct {
    cui_id target;
    uint64_t frame_sequence;
    size_t event_index;
    cui_event_type event_type;
    cui_key key;
    unsigned modifiers;
    bool has_press, has_release, has_key_bounds;
    cui_pointer_activation_sample press, release;
    cui_rect key_bounds; /* Actual rendered routing hitbox, not next layout. */
} cui_activation_snapshot;
bool cui_event_activation_snapshot(const cui_context *, size_t event_index,
                                   cui_activation_snapshot *out);
```

Names are proposals, not shipped declarations. Successful output is a fully
owned copy; no pointers, callback registration, heap/VLA or layout submission.
Null output, invalid index, unaccepted/currently rejected target, nonordinary
phase or current Core error returns false without touching output. The getter
uses the same current acceptance guard as cui_event_accepted_activation. Outside
ordinary submission, measurement, prepaint, inert/layout-only and after end_frame
it returns false. A copied value remains usable after next frame/destruction;
frame/index are identity for that context/frame only, with existing wrap/recreation
boundaries. Equality across recreated contexts is not a global event identity.

Store one private primary capture-start sample/validity/target across frames.
Capture it only when the existing down route actually establishes capture. Copy
that sample and release into the generated activation slot before clearing
capture. Keyboard activation snapshots copy the authoritative routed rendered
focus hitbox. Accessibility snapshots have neither fabricated press nor key
bounds. Keyboard or accessibility origin remains distinguishable by event_type.
The sample preserves raw host count, including zero; Button can retain its
existing public normalized count while exposing raw samples separately.

Use a typed snapshot array indexed by the existing activation slot, bounded by
CUI_CORE_EVENT_MAX (256), rather than a new 512-entry dispatch array. Existing
event_activation_slots already map reordered dispatch indices to slots, including
synthetic cancellation/scroll ordering. Acceptance consumes the pending ID but
must not destroy its immutable snapshot. The getter revalidates acceptance and
uses that slot; cancellation before acceptance invalidates it. Reset all slot
validity at successful begin_frame. A snapshot must never alias previous-frame
slot storage. Target and event_index in output are filled from the finalized
dispatch mapping, avoiding an index captured before synthetic event swaps.

The compiled illustrative declarations occupy 152 bytes per snapshot and 48
bytes per pointer sample on native x86-64; 256 slots use 38,912 bytes plus the
capture sample and
small validity/keyboard state. The same declarations measured 152/48 bytes and
38,912 slot bytes under Zig0.16 wasm32-wasi/Wasmtime49 too. Implementation must record actual sizeof values
on native and WASI, not enforce a native-size fixture. Typed context storage
avoids payload-arena alignment/overflow concerns. No new per-event allocation;
existing create failure remains atomic NULL. Slot capacity never silently
truncates an accepted snapshot: the activation slot and its snapshot are reserved
together, and a failure reports CUI_ERROR_CAPACITY before creating an activation.
Existing public queue256/internal dispatch512 limits remain unchanged.

Every capture-clear site must clear start validity: explicit CANCEL, touch-pan or
multicontact takeover, replacement down, Escape, disabled/removal/modal cleanup
and destruction. LEAVE preserves capture/start/coordinates; ENTER creates none.
Captured outside MOVE then LEAVE preserves data until an actual up/cancel; leave
must not be treated as physical cancellation. Match type/device/contact identity so a release cannot borrow a different
contact's start. If legacy routing accepts an unmatched release, preserve that
activation and expose has_release=true/has_press=false rather than silently
changing routing in this snapshot slice.

No adapters need to fabricate first_focus. Initially both booleans are unknown/
false. A separately approved append-only host event metadata field can carry
known first-focus information; SDL/browser support must remain explicitly unknown
unless the actual host supplies authoritative evidence. This is a later host
slice, not part of Core sample reconstruction.

Standalone Button should copy the snapshot before invoking legacy callbacks.
Group needs the same freeze-before-legacy guarantee: Base obtains a local copied
snapshot at its accepted dispatch site, then runs legacy and forwards that value
to a new optional snapshot hook. Keep its existing event-index hook compatible.
This avoids asking the guarded getter after legacy has consumed an accessibility
event. Preserve Group on_change override and input order; descriptors stay copied,
controlled model values reread between events. Do not add borrowed Core pointers
to the public Button payload.

## Slice 2: opt-in keyboard policy and signed tab paths

A snapshot alone must not claim source keyboard timing. Append an opt-in semantic
keyboard policy, default CUI legacy behavior, with a Button option forwarding it.
SOURCE_UNMODIFIED_KEYUP uses a private focus generation and a pending generation
per eligible owner, without inventing a matching-key constraint. Qualifying down
records generation; another nonqualifying down/up, focus change, blur/cancel,
removal, disabled/modal cleanup or generation wrap invalidates it. Activation-key
up without modifiers fires once if the generation matches. Preserve current
Enter-down/Space behavior for zero options and nonopted controls. A source-policy
change takes effect through the published routing metadata, as existing previous-
frame input routing does. It cannot reinterpret already routed legacy events.

A private generation must be updated by every actual focus transition, including
routing, programmatic focus, trap restoration and removal. Do not reuse focused
text/password host generation: its lifetime/configuration contract differs.
Same-ID focus does not increment. On wrap clear pending owners before restarting
the generation so no ABA candidate survives. Source's policy for repeated key-
downs differs from CUI immediate activation: repeated downs may refresh pending
state but never themselves invoke callbacks in the source-style policy.

Signed tab ordering needs an independent order field and grouping path, retaining
existing tab-stop exclusion. Proposed additive APIs:

```c
bool cui_set_tab_order(cui_context *, cui_id, int64_t order);
bool cui_begin_tab_group(cui_context *, cui_id, int64_t order);
void cui_end_tab_group(cui_context *);
```

Do not repurpose semantic tab_index=-1 as signed source ordering. Preserve it as
CUI's existing skip marker and append explicit signed order metadata. A negative
custom order sorts before zero while remaining reachable if its tab stop is
true. Tab groups are neutral order scopes, not modal focus traps; do not reuse
focus-scope ownership semantics in a way that changes trapping or restoration.
Lexicographic ancestor-group order plus leaf order plus semantic insertion index
matches source ordering. Nonstop nodes keep their order position for programmatic
focus and subsequent next/previous. Equal order is stable; comparisons must not
subtract signed values (INT64_MIN/MAX are valid). Allocate ordered index buffers
with existing max_elements and bounded group tables/stacks (256); report atomic
capacity/duplicate/lifecycle failure, balanced end, and suppress passive scopes.
Sort once with the published rendered semantic snapshot, using an O(n log n)
comparison sort and bounded path comparisons rather than sorting every Tab press.

## Slice 3: real event-local primary activation propagation

Do not implement this with a Button callback return bool, an ancestor loop after
legacy callbacks, or restoring event_consumed. Those cannot undo callbacks or
interleave later focus decisions. Existing raw/accepted activation APIs remain
single-target and compatible. A complete bounded opt-in primary activation mode
needs a metadata preparation/dispatch transaction before ordinary layout:

```c
typedef enum {
    CUI_ACTIVATION_CONTINUE, CUI_ACTIVATION_STOP
} cui_activation_reply;
typedef enum {
    CUI_ACTIVATION_CAPTURE, CUI_ACTIVATION_BUBBLE
} cui_activation_phase;
typedef cui_activation_reply (*cui_activation_listener)(
    void *user, const cui_activation_snapshot *event,
    cui_id current_target, cui_activation_phase phase);
bool cui_activation_prepare_begin(cui_context *);
enum { CUI_ACTIVATION_LISTEN_CAPTURE = 1, CUI_ACTIVATION_LISTEN_BUBBLE = 2 };
typedef struct {
    cui_role role;
    bool disabled, no_focus, pointer_activatable, blocks_pointer_activation;
    unsigned phases; /* Nonzero subset of the two LISTEN flags. */
} cui_activation_participant_options;
bool cui_activation_prepare_participant(cui_context *, cui_id,
    cui_activation_participant_options, cui_activation_listener, void *user);
bool cui_activation_prepare_scope_begin(cui_context *, cui_id, bool modal);
void cui_activation_prepare_scope_end(cui_context *);
bool cui_activation_prepare_dispatch(cui_context *);
void cui_activation_prepare_abort(cui_context *);
```

These are the concrete shape of a proposed opt-in primary-activation transaction,
not a general replacement input/action engine or an approved final ABI. Metadata
and listener fields are copied; user storage remains borrowed until dispatch
returns. Null callbacks and unsupported phase bits reject atomically. Stable
IDs associate registrations with rendered ordered/clipped nodes. Neutral tab
scopes remain separate. Existing Buttons keep immediate callbacks; staged Button/
Group wrappers must opt into registration and suppress duplicate ordinary
notifications after dispatch, while retaining boolean activation results.

Preparation opens no Clay layout and does not consume the public queue. It
registers the current eligible participant/modal metadata required for acceptance.
Abort, malformed data, duplicate IDs, unbalanced scopes or bounded capacity
failure leave queued input, focus/capture and the prior rendered snapshot intact;
no user callback runs. Once validated, dispatch owns the input batch exactly once
and processes raw events in input order against that rendered snapshot, applying
focus/capture transitions and invoking capture then bubble listeners at each
appropriate event. Callback focus/model changes therefore affect the next event,
not just a later frame. The next ordinary begin_frame reuses already routed
frame events for component reducers and never routes/replays callbacks a second
time. One ordinary Clay_EndLayout still paints the resulting model.

Per-participant mouse-down latches are necessary. A source parent can remain
hovered when the deepest leaf changes between down/up; replaying only ancestors
of CUI's single accepted target misses this. The bounded transaction must track
qualifying hit participants in rendered listener order, their down samples and
validity across frames, then check release hover/clip/modal/disabled eligibility.
Do not equate that list with layout ancestry or with the general GPUI listener
engine. Capture uses forward rendered registration order, bubble reverse order;
callbacks that stop propagation prevent only later participants for that event.
Cleanup of pending capture is performed before notifications so a stopped bubble
cannot leave ghost capture. Top activation blockers still deny underlying
activation; wheel/touch-pan routing and pointer presence remain independent.

After dispatch begins, callbacks cannot be rolled back. Invalid callback reentry
or layout/config mutation reports a lifecycle error and aborts remaining
notifications for the owned batch without replaying already notified events.
The caller must handle that explicit error; it must not silently retry the queue.
Only model updates, explicit event-local stop, and guarded focus updates are
supported in the listener phase. No Core/Clay drawing, measurement, reentrant
prepare/begin/end or arbitrary mutation is allowed. This requires a precise Core
phase guard, not a callback convention alone.

This transaction is larger than snapshot storage and affects queue ownership,
focus and activation routing. Production must be reviewed separately with Input/
focused-text ownership coordination. A smaller post-compose staged ancestor hook
may be useful but cannot be labeled complete source event timing/propagation.
LongPress/aux-click, general mouse listeners, shortcuts/action bubbling and
platform default prevention remain separately scoped; they are not obligations
of this primary Button slice merely because ClickEvent declares related types.

## Required proof and ownership

Snapshot production can be owned narrowly in include/cui/cui.h, private context/
routing reset/capture-clear/slot creation and read-only query regions of Core,
Button snapshot conversion and Base/Styled Group freeze-before-legacy hooks, with
new focused unit/native/browser fixtures. No public platform types, renderer,
Theme, Icon, font engine or previous-frame layout approximation is needed.
Keyboard/tab work needs a separate approved Core focus/semantic ordering slice,
Button option forwarding and focused tests; coordinate focused-text/password
regions explicitly. Propagation requires separately approved queue/phase/input
routing and staged Base/Components APIs, not hidden edits to existing behavior.

Tests must fail the old missing capability and retain existing label/numeric/
passive/presence/capture regressions. Snapshot cases: different down/up positions,
modifiers/counts; same/cross-frame pointer/touch/pen; release-outside; LEAVE then
reenter; cancellation/touch takeover/multicontact; replacement down; disabled,
loading, removal, modal and clipped-away owner; reordered synthetic events; all
256 slots; failed-query untouched output; copied lifetime; measurement/prepaint/
end-frame guards; Group legacy consumption and snapshot notification ordering.

Keyboard cases: Enter/Space only on up; modifiers; repeats; intervening keys;
Enter-down/Space-down/Enter-up source quirk; focus A→B→A; blur/cancel/removal;
generation wrap; rendered bounds under relayout; zero-policy compatibility.
Tab cases: negative/zero/positive extremes, insertion ties, nested groups,
nonstop programmatic anchors, reverse/wrap, disabled/removal, modal trap,
inert/layout-only, capacity and balanced scopes.

Propagation cases: forward capture/reverse bubble order, independent stop per
event, nested/overlapping/clipped/layer/modal participants, down/up leaf changes,
parent listener acceptance, loading/disabled blockers, callback-induced focus
changes before later queued keys, immutable descriptor and controlled-model
updates, abort/capacity atomicity, invalid reentry, no duplicate ordinary callback,
full queue, capture cleanup despite stopped notification, and a following valid
click. Real Chromium PointerEvents/keyboard/CDP touch and actual Canvas state
must verify behavior; native SDL PushEvent evidence must be labeled queued-adapter
validation rather than physical OS/device input. First-focus host claims require
separate platform evidence; long-press primary click must not be invented.

## Evidence status

The existing implementation baseline was warning-built and exercised by the
Button payload work: native CTest159/160 (only generated distribution stale),
new portable and Noto font sanitizer fixtures, Wasmtime49 with80b fuel, and actual
Chromium1208 payload/numeric/label/passive fixtures passed. Those tests verify the
shipped release-derived slice only. No tests or production implementation of the
snapshot, signed-order, source-keyup or propagation proposal have been added here.
