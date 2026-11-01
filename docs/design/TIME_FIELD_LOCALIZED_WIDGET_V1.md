# Localized TimeField widget V1 proposal

Status: design checkpoint, not an implemented API. Base `63f0b5e7`, including the
pure `cui_time_text.h` helpers and unchanged legacy TimeField/DateTimePicker.
Authority: [completion audit](TIME_FIELD_COMPLETION.md), frozen upstream commit
`87d10ae5e1299d1be18670a37570c5402f952e34` and its four recorded file hashes.
This slice delivers actual C widget editing/validation. It does not close the
whole TimeField family or expose the new route in any foreign language.

## Exact proposed public header

New `include/cui/cui_time_field_v1.h` includes `cui_time_text.h` and
`cui_picker_events.h`. Reuse `cui_time`, pure text statuses/options and copied
`cui_time_change`/borrowed `cui_time_handler`; no duplicate event vocabulary.
These are C records, not a wire ABI. Concrete target size/offset receipts are
required before shipping; no assumed enum/pointer width is encoded in records.

```c
#define CUI_TIME_FIELD_VERSION_V1 UINT32_C(1)
#define CUI_TIME_FIELD_NAME_MAX_BYTES_V1 64u
#define CUI_TIME_FIELD_PENDING_MAX_BYTES_V1 CUI_TIME_TOKEN_MAX_BYTES_V1

enum {
    CUI_TIME_FIELD_DISABLED_V1 = 1u << 0,
    CUI_TIME_FIELD_READONLY_V1 = 1u << 1,
    CUI_TIME_FIELD_OWNER_INVALID_V1 = 1u << 2
};
typedef enum {
    CUI_TIME_FIELD_HOUR_V1 = 0,
    CUI_TIME_FIELD_MINUTE_V1 = 1,
    CUI_TIME_FIELD_SECOND_V1 = 2,
    CUI_TIME_FIELD_PERIOD_V1 = 3
} cui_time_field_segment_v1;
typedef struct {
    uint32_t struct_size, version, flags, reserved;
    cui_time_text_options_v1 text;
    uint64_t edit_generation;
    cui_str segment_names[4];
} cui_time_field_options_v1;
typedef struct {
    uint32_t struct_size, version, reserved[2];
    uint32_t segment, digit_count, draft, validation;
    uint32_t last_cycle, last_precision, pending_length, reserved_state;
    uint64_t edit_generation, last_frame_sequence;
    cui_time last_value;
    char pending[CUI_TIME_FIELD_PENDING_MAX_BYTES_V1];
    uint32_t reserved_tail;
} cui_time_field_state_v1;
typedef struct {
    cui_time_text_status_v1 status;
    bool changed;
    size_t change_count;
    size_t rejected_count;
    cui_time_text_status_v1 validation;
} cui_time_field_result_v1;

cui_time_text_status_v1 cui_time_field_options_init_v1(
    cui_time_field_options_v1 *out);
cui_time_text_status_v1 cui_time_field_state_init_v1(
    cui_time_field_state_v1 *out, cui_time value,
    const cui_time_field_options_v1 *options);
cui_time_text_status_v1 cui_time_field_clear_validation_v1(
    cui_time_field_state_v1 *state);
cui_time_field_result_v1 cui_base_time_field_v1(
    cui_context *, cui_id, cui_str name, cui_time_field_state_v1 *, cui_time *,
    const cui_time_field_options_v1 *, cui_time_handler);
cui_time_field_result_v1 cui_time_field_v1(
    cui_context *, cui_id, const cui_theme *, cui_str name,
    cui_time_field_state_v1 *, cui_time *,
    const cui_time_field_options_v1 *, cui_time_handler);
```

Records require exact size/version1 and zero reserved fields/unknown bits.
Initializer supplies H23/minute ASCII text defaults, flags0, generation1 and
empty names. Names `{NULL,0}` select English Hour/Minute/Second/AM/PM;
otherwise require valid UTF-8 without NUL, length1..64, including hidden names.
There is no locale-name registry. `edit_generation` must be nonzero; the owner
changes it when token bytes, domain or semantic names change in place. Cycle/
precision changes are also detected directly. Disabled/readonly/owner-invalid
flag changes do not require a generation change. Options, nested locale/token
storage, names, theme and handler/user are borrowed through the entire call;
no pointer is retained in state. Output/input aliasing is unsupported.

State is caller-owned per widget/context, initialized explicitly with a
structurally valid, precision-compatible value and valid configuration. The
initializer accepts OUT_OF_RANGE owner values, records that validation, and
returns OK; it never clamps. Other failures leave output bytes unchanged. The
public fields are inspectable diagnostic values, not owner-writeable controls;
only init/clear/compose mutate them. State cannot be shared across widgets or
contexts or concurrently accessed. Removing/recreating a context requires init.
No heap/context storage or new input owner is added.

## Admission, value and error rules

Preflight validates mandatory pointers/nonzero id, state header/invariants,
options/text configuration/names, then full value/precision, before input,
state changes, semantics, paint or callbacks. Invalid argument/config/value
returns its pure status, changed=false/counts0, and preserves state/value/input.
Styled additionally requires a theme and usable finite nonnegative layout
metrics/positive font size before reduction. Out-of-domain owner values are
**valid submitted models**: render them, mark them invalid, allow edits toward
an in-domain value. They never generate CHANGE simply because the owner set them.
Minute mode with a nonzero second is INVALID_VALUE, not hidden truncation; the
owner must explicitly set second0 when changing precision. This supersedes the
audit's tentative truncating-new-route suggestion and matches the landed helper.

Only ordinary painted, non-inert, writable composition with a valid previous
Core input snapshot may reduce input. Reuse `cui_frame_is_submitting`,
`cui_frame_paint_enabled`, `cui_frame_input_snapshot_ready`, current modal scope
policy and event-consumption/target checks; no new Core observer/dispatcher.
State records the last nonzero admitted submission sequence. First submission,
omission/gap, sequence wrap, or failed prior geometry gets a geometry-only pass
without input. No fabricated generation at sequence0. Outside submission is
INVALID_ARGUMENT and untouched. Layout-only/inert/disabled/readonly submissions
may submit appropriate semantics/paint but freeze edit state, value and callbacks;
model/config changes are synchronized only on the next ordinary writable pass.
Disabled still lets existing Core disabled-focus cleanup run. No pending token
or draft survives a generation/mode change, owner value change, omission/gap or
loss of focused ownership. Identical owner echo preserves an unfinished draft.
No callback/input side effect is rolled back if a later Core/end-frame error occurs.

`result.status` reports operation/preflight/Core failure: pure OK on a healthy
submitted widget, INVALID_ARGUMENT for lifecycle, INVALID_CONFIG/INVALID_VALUE
for preflight, and INVALID_ARGUMENT if current Core readiness/error invalidates
input mid-operation (already applied effects remain). It is not the user's last
validation error. `result.validation` and state.validation hold OK,
INVALID_SYNTAX, INVALID_VALUE or OUT_OF_RANGE from the latest edit/owner validation.
Each rejected TEXT transaction or key candidate increments rejected_count once;
a multi-token digit transaction may stage several candidate checks but counts
at most one rejection for that original event. A valid edit, including an equal
value or valid separator navigation, clears transient validation. External owner
value/generation changes replace validation with the new owner's domain result.
`clear_validation` clears only transient validation, not draft/focus/value or
OWNER_INVALID; an out-of-domain current owner is still visibly invalid. Invalid
state header leaves clear output untouched. No synthetic validation callback.

## Original-order editing and atomic paste

Use `event.text_data/text_length` when supplied; otherwise bounded inline text.
Core already copies queued payloads and guarantees dispatched storage until next
begin_frame (`cui.h:190–207`, `cui.c:1480–1495`). Never retain a view in state or
across a callback. Recognize exact configured tokens, not ASCII fallbacks for a
custom locale. TEXT is character input; modifier-based shortcuts are not turned
into text and IME composition/preedit events remain unsupported/ignored.

A complete formatted payload accepted by `cui_time_parse_v1` is one atomic
replacement independent of selected segment. It may exceed64 bytes up to160.
Success replaces the whole time, clears draft/pending, keeps selected segment,
and clears validation; syntax/value/domain rejection leaves whole value, draft
and selected segment unchanged, sets validation and consumes the owned TEXT
once. A non-single-token payload containing a separator/period token is treated
as a complete formatted attempt, never partially applied as segmented digits.
This is an input payload rule, not a claim of host clipboard shortcut integration.

Other TEXT is a stream of configured digits or one exact separator/period token.
An incomplete UTF-8/token prefix is kept in the owned16byte pending buffer;
next TEXT continues it. Prefix-free locale tokens determine completion uniquely.
A byte prefix matching no token rejects the transaction atomically and clears
pending; any staged time/draft/selection edits from that original event are
restored. No more than160 bytes from one event are scanned. Full-value paste
must start at an empty pending buffer; otherwise the event is treated as stream
input, and the owner can cancel pending with navigation/Delete/Escape. Separator
moves to the next visible segment without wrapping (period_separator selects
period in H12); a token in an inappropriate position rejects without movement.
Period segment accepts exact locale AM/PM; ASCII a/A/p/P shortcuts exist only
with the NULL default locale. Digits do not edit the period.

Numeric editing follows source restart/early-advance rules: a valid two-digit
pair completes; invalid pair restarts from second digit; a first digit whose
next digit cannot fit completes immediately. H12 display1..12 maps into existing
half-day; Up/Down wraps within that half-day without carry. Numeric candidate
outside domain leaves model unchanged, but the numeric draft remains so a next
digit can form a valid candidate. Completion advances only to the next visible
segment, stopping at the last. Backspace/Delete clear pending/draft and reset
selected numeric value to0 (H12 maps0 to12 in the same half-day), period toAM,
subject to bounds. Home/End select segment minimum/maximum as a documented CUI
extension. Left/Right move without wrap, reset pending/draft, and focus that
segment; Escape discards pending/draft only. Unknown/foreign/consumed/modal-blocked
inputs are untouched; owned recognized edit attempts consume once, including
rejections. Pointer primary-down selects its visible segment and resets draft;
no capture/drag ownership is introduced. Same-batch events retain original Core
target metadata; current writable/visibility policy is rechecked per event.

## Focus and representation boundary

Legacy functions retain all previous Tab behavior and IDs. New route reuses
`cui_time_segment_id(id,0..3)` and the same compound root, rejecting id0 only for
new entrypoints. New semantics remain GROUP + four possible TEXTBOX children
with localized names, current values/ranges and readonly/disabled flags.

Publish all visible segments as ordinary global Tab stops in visual order;
never set handles_tab or consume Tab. Core determines actual forward/backward
entry/exit. Reconcile selected segment to the actual focused visible child.
This means forward entry selects Hour, reverse entry the last visible segment;
forward Tab from last/backward Tab from first exits normally. Multiple Tab events
are already globally routed before reduction, so do not simulate another step.
**Full source parity remains unfinished**: source uses one root focus handle,
retains selected segment on re-entry, and handles internal Tab before boundary
propagation. Current `handles_tab` is a single bool, and `focus_step_within`
wraps; neither implements directional global exit. This slice does not invent
a global focus API or trap/wrap Tab to conceal the difference. Actual current-
focus/boundary proofs are required before claiming even the supported global path.
Root TimeInput accessibility representation and physical AT remain separate.

## Styled paint and CHANGE timing

Keep ordinary flow, existing root/segment IDs and legacy theme geometry as the
foundation of this slice. Measure the largest of100 localized two-digit strings
at the active font size for stable numeric cell width; period cell measures
max(AM,PM). Include existing segment padding and use actual separator tokens;
no width based on UTF-8 byte count or a hardcoded two-ASCII-glyph assumption.
No pointers into temporary formatting buffers may outlive text submission.
Invalid border uses theme.danger when OWNER_INVALID, current owner OUT_OF_RANGE,
or transient state validation is non-OK; it clears when all three are clear.
Disabled foreground remains muted; owner-invalid remains visible when disabled.
Full source size presets, focus ring/selection styling parity and arbitrary
part decoration remain explicit later work, not silently claimed here.

Reduce once, publish final semantics once, and Styled paint once. Then compare
entry versus final actual hour/minute/second. On net change, call existing
`cui_time_handler.on_change` once after Base semantics or complete Styled paint,
with copied previous/value and UNKNOWN aggregate metadata, proven frame only,
no invented input index/cause. Missing handler still edits; net-zero, draft,
focus and rejection-only calls never notify. change_count counts actual callbacks
0/1, not accepted edits. Callback/user/payload pointer are borrowed synchronously;
copying payload is supported. External application/userdata mutation and queued
future work are allowed; active model/state/options/theme/token storage/UI/input
resources must not be mutated, freed, rebound or recursively submitted. No owner
reread after callback. No change to any legacy observer/foreign scalar timing.

## Cohesive implementation and evidence plan

Own only new TimeField widget header/Base/Component sources, narrowly factored
picker-local helpers if needed, focused tests, docs/metadata and canonical outputs.
No Core/editor/bridge/language production changes. Public observer timing is an
applied aggregate, never a controlled proposal/acceptance protocol.

1. Freeze actual old picker/DateTime/observer commands, IDs, state, values,
   semantics/focus/callback traces before shared-helper edits; compare using
   independent legacy source/binary rather than updated goldens. NULL/default
   legacy route stays byte/fieldwise equivalent, including zero-owner paths.
2. Actual authored Base/Styled UI fixture: all4modes, ASCII and multi-byte tokens,
   restart/early-advance, H12 half-day wrap, delete/source quirks, typed separators,
   prefix split across frames/events, owner echo/change/generation, mode changes,
   omission/reappearance and no-input initial/failed/recovery frames.
3. Actual text_data full paste>64bytes: mutate/free original queued source before
   dispatch, assert copied whole model and callback payload; malformed/oversize/
   out-of-domain paste leaves value/draft/selection unchanged except documented
   validation/pending clear. Inclusive bounds, owner outside bounds, exact min=max,
   no clamp, minute hidden seconds and config/preflight atomicity.
4. Keyboard primary proof with neighboring controls: forward/backward Tab entry,
   each segment/current focus, boundary exit, two Tabs in one batch, Left/Right,
   readonly/disabled/inert/modal/foreign input, no wraptrap or consumption of Tab.
5. Real software draw/text/bounds and pixels: stable cell widths despite localized
   digits/periods; invalid danger border and explicit clear/owner-invalid behavior,
   disabled and focus colors, callback after actual final paint, no fake counts.
6. Strict C11/C++ old/new include orders, selective/umbrella and wasm32 compile
   receipts, old TimeField/DateTime tests, usage/metadata/skill freshness. Private
   Xvfb/windowless software only; no new build tree/engine/desktop test.

| Delivery | This proposal | Remaining authority |
| --- | --- | --- |
| C pure locale/domain helpers | Landed, tested | `cui_time_text.h` |
| C localized widget + validation | Proposed, not implemented | This checkpoint |
| C legacy widget/observer | Preserved | Existing picker headers/tests |
| Python | Scalar legacy subset only | New widget options/events omitted |
| JavaScript | Scalar legacy subset only | New widget options/events omitted |
| TypeScript | Scalar legacy subset only | New widget options/events omitted |
| Go | Scalar legacy subset only | New widget options/events omitted |
| Rust | Scalar legacy subset only | New widget options/events omitted |
| IME/clipboard shortcuts/physical AT | Not established by this slice | Separate host/editor evidence required |
| Source one-root Tab/focus/size styling | Unfinished | Separate compatible review required |

No compiler or test execution for this source-only proposal. All names/signatures
and the ordinary-global-Tab exception require review before production.

## Integrator decision (2026-11-01)

Approved the additive implementation and test scope above. Retain the explicit
ordinary-global-Tab exception; source one-root focus behavior remains unfinished.
`changed` reports net value change independently of handler presence;
`change_count` preserves the existing observer convention of counting actual
callback invocations, not accepted edits. Existing entry points remain unchanged.

Source implementation may proceed in isolation. Runtime, ABI, generated-resource
and gallery acceptance remain required; approval is not implementation evidence.

## Approved source checkpoint

Root approved this additive slice including the ordinary-global-Tab exception.
`changed` remains a net final value difference even with no handler;
`change_count` counts actual copied aggregate callback invocations0/1, not accepted
edits. Initial implementation/test source is prepared with separate Base/Styled
composition, copied token/paste input and measured localized cells. No shared
legacy picker implementation was edited. Canonical source/distribution/SDK/skill
outputs accompany the checkpoint. **No compilation or runtime evidence yet**;
fixtures, pixel assertions and exact state/option ABI still await the allocated
focused lease. Gallery migration follows geometry proof, not this source commit.

### Same-batch original-target resolution

Core stamps each TEXT target from focus during pre-reducer dispatch (`cui.c:1731`);
four separately queued digits can all retain the entry Hour target. New editing
tracks the last **handled original target**, following the legacy reducer's
pattern: consecutive events with that same target retain evolving local segment/
draft; genuine target transitions and primary pointer selection reset it.
Consumed, foreign or unsupported events do not alter this tracking. No event
is rewritten or retargeted. For an owned Tab, preserve the Core-dispatched focus
captured at entry after any earlier local auto-advance, clear pending draft,
and leave the Tab unconsumed. Later original events still determine edits.

Source-only fixtures now compare four separate TEXT events1/2/3/4 against one
TEXT1234 (both12:34, one aggregate callback), with raw Core targets asserted
unchanged. Actual focus-action witnesses insert foreign or consumed visible-
segment TEXT between1 and2: the final owner value is unchanged12:34, so no
callback despite applied intermediate edits. TEXT→Tab→TEXT and TEXT→pointer→TEXT
must honor actual destination focus, plus existing forward/backward boundaries
and two-Tab batch. These assertions await execution, not assumed proof.
