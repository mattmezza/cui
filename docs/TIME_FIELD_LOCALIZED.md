# Localized TimeField widget

The C widget supports explicit locale tokens, segmented editing and inclusive
same-day bounds. See the
[complete Base/Styled program](TIME_FIELD_LOCALIZED_EXAMPLE.md) and
[verification receipt](validation/time-field-localized.json).
Include `cui/cui_time_field_v1.h` for the new caller-owned state/options route.
Use [pure text values](TIME_FIELD_TEXT.md) for the exact locale grammar and bounds.

Initialize options, configure text locale/cycle/precision and inclusive same-day
bounds, then initialize one state per widget/context. Keep state and time alive
between frames. Options, locale tokens, names, theme and observer are borrowed
only through each synchronous call. Change nonzero edit_generation when changing
locale/name/domain bytes in place. Mode/value changes reset unfinished input;
identical owner echo preserves it. Minute precision requires second0 explicitly.
Owner values outside bounds render invalid without being clamped.

```c
#include <cui/cui_time_field_v1.h>

typedef struct {
    cui_time value;
    cui_time_field_state_v1 state;
    cui_time_field_options_v1 options;
    unsigned notifications;
} reminder;

static void reminder_changed(void *user, const cui_time_change *event) {
    reminder *owner = user;
    /* External application bookkeeping is allowed; active value/state/options
     * remain borrowed until composition returns. Copy event if retaining it. */
    if (event->value.hour >= 9)
        ++owner->notifications;
}
static bool reminder_init(reminder *owner) {
    owner->value = (cui_time){9, 30, 0};
    owner->notifications = 0;
    if (cui_time_field_options_init_v1(&owner->options) != CUI_TIME_TEXT_OK)
        return false;
    owner->options.text.flags = CUI_TIME_TEXT_HAS_MINIMUM_V1;
    owner->options.text.minimum = (cui_time){9, 0, 0};
    return cui_time_field_state_init_v1(&owner->state, owner->value,
                                       &owner->options) == CUI_TIME_TEXT_OK;
}
static cui_time_field_result_v1 reminder_compose(
    cui_context *ui, const cui_theme *theme, reminder *owner) {
    return cui_time_field_with_size_v1(ui, 20, theme, cui_string("Reminder"),
        &owner->state, &owner->value, &owner->options, CUI_DESIGN_BASE,
        (cui_time_handler){reminder_changed, owner});
}
```

Call compose during an ordinary frame. First/gap/recovery submissions establish
geometry without applying queued edits. Disabled, readonly, inert and layout-only
submissions freeze edits and notifications. Editing uses original-order targeted
Core events, copied UTF-8 text payloads, token prefixes across TEXT events and
atomic full formatted paste up to 160 bytes.

## Size and appearance

`cui_time_field_with_size_v1` accepts the shared `CUI_DESIGN_XS` through
`CUI_DESIGN_9XL` scale. Use `CUI_DESIGN_BASE` for a 32 logical pixel control at
the default theme density. Sizes scale with `theme.font_size / 16`; Base and MD
share metrics. The field fits its locale, cycle and precision, and centers
each segment in a cell measured to fit the widest token pair or period label.
The font's individual glyphs retain their own widths.

Explicit sizes use theme input colors, a selection background on the active
segment, and an outer focus ring that can coexist with the validation border.
`theme.hide_focus_ring` suppresses the outer ring. Disabled fields are dimmed
and lose the selected paint. Pass `CUI_DESIGN_INHERIT` or call
`cui_time_field_v1` for the existing appearance. Editing, state and callbacks
are shared across both entry points. The language bindings currently expose
their existing localized appearance.

## IME composition

In working source, set `options.flags |= CUI_TIME_FIELD_IME_V1` to enable
composition on the direct C widget. The gallery enables it by default. Styled
shows preedit inside the focused segment and registers that segment's candidate
anchor with the host. Owners using Base can call
`cui_time_field_preedit_v1(ui, root_id, &segment_id)` after submission, paint
the borrowed text in that segment, and register the anchor with
`cui_set_text_input_geometry_rect`. Copy the text before the next frame if it
must outlive composition.

Start or update opens a session for the targeted segment. Preedit is bounded to
160 UTF-8 bytes and leaves the value and callbacks alone. Commit accepts a
complete formatted time, one or two locale digits for a numeric segment, or the
complete locale AM/PM token for the period. Syntax, value or bound failures
leave the time intact. Cancel, focus loss, gaps, read-only or disabled
submission, and changes to the owner value or edit generation clear the
session. The host delivers one commit for each accepted composition. The widget
ignores plain text while that segment is composing.

Headless Chromium covers localized preedit, commit, rejection and cancellation.
An SDL empty editing event hides preedit immediately. A later commit applies
once if it explicitly targets the same segment and the session is still
eligible. Ordinary edits, Escape, a terminal cancel or focus transfer end
eligibility. Private Xvfb checks the SDL adapter with injected same-poll and
cross-poll events. Physical IME input and accessibility still need platform
qualification. See the [host event contract](IME.md). The
optional bridge and Python, JavaScript, TypeScript, Go and Rust facades expose
localized editing through their documented subset and omit C composition
opt-in and preedit. Host clipboard shortcuts and composition range styling are
outside this route.

## One Tab stop per field

Use the ordered frame route when keyboard focus should enter and leave a
TimeField as a whole. Each field remembers its selected segment. Tab and
Shift-Tab move within the field and continue to surrounding controls at its
boundaries. Moving away clears a partial edit and preserves the selection.
A digit can finish a segment before the next key in the same
batch. Input is routed in order even when the application submits its fields
in a different order from the input.

```c
cui_time_field_frame_entry_v1 fields[] = {
    {start_id, &start_state, &start, &start_options},
    {end_id, &end_state, &end, &end_options},
};
if (cui_begin_time_field_frame_v1(ui, width, height, dt, fields, 2) ==
        CUI_TIME_TEXT_OK) {
    cui_time_field_from_frame_v1(ui, start_id, &theme, cui_string("Start time"),
                                CUI_DESIGN_BASE, (cui_time_handler){0});
    cui_time_field_from_frame_v1(ui, end_id, &theme, cui_string("End time"),
                                CUI_DESIGN_BASE, (cui_time_handler){0});
    cui_draw_list draw = cui_end_frame(ui);
    /* Pass draw to the ordinary renderer. */
}
```

Initialize both fields as above. Declare up to eight distinct IDs and mutable
model pairs before input dispatch. The entry array is copied. Keep the models,
options and their tokens alive through frame end. Each model stays unchanged
between admission and its submission. Submit each declared ID once.
`cui_base_time_field_from_frame_v1` also supports optional custom layout with
the root and segment IDs. Geometry from the last completed frame routes
pointer input. A new or re-enabled field needs a frame to publish its geometry
and keyboard eligibility.

Omit a hidden field from the list or set its disabled flag before beginning
the frame. Use ordinary `cui_begin_frame` for a frame with zero ordered fields.
Read-only fields accept navigation and keep their values. Admission is fixed
for the frame, so an admitted field that an application later fails to submit
loses its staged edit. Input already routed to the field or to its neighbors
keeps those targets. Filter unwanted events before queuing them. Consumption
during composition does not undo ordered input.

Invalid entries, duplicate IDs or models, unsupported options and a failed
storage reservation leave queued input available for a corrected begin. The
ordered route reserves one fixed-capacity workspace from the context's
resource budget. The result and synchronous callback see the applied model
after semantics, and after paint submission in Styled. A later layout or
renderer failure leaves an already applied change in the caller's model.
Schedule teardown after the observer returns. Language bindings currently
expose the ordinary route.

Choose **One stop per field** in the gallery's **TimeField navigation** menu
to try both fields and Save. Tab and Shift-Tab leave the preview at its edges
using normal browser navigation. The gallery checks the routing decision
after applying queued input and before the browser's default action.

For ordered C editing with IME, set `CUI_TIME_FIELD_IME_V1` in each field's
options before admission. Keyboard focus stays on the field root. After a
completed frame, `cui_get_text_input_config` returns the editable segment and
its generation. Custom hosts pass this pair with composition events using
`cui_queue_text_input_event_v1`. Recheck it after completing each frame. The
selected segment supplies the candidate rectangle through `cui_text_input_rect`
in Styled. With Base, register the rectangle for the selected segment when
laying out your own field.

The SDL3 adapter routes ordered preedit, empty preedit clearing and the later
commit automatically. The gallery supports composition in **Per segment with
IME** and **One stop per field**. In the ordered preview the hidden text proxy
follows the selected segment, and Tab and Shift-Tab leave the preview at its
edges. See the [host event contract](IME.md).

Run the [complete two-field form](../examples/time_field_ordered.c) from a
configured repository. It queues one batch that edits both fields and
activates Save, using the same frame calls a native host can use.

```sh
cmake --build build --target time_field_ordered_example
./build/time_field_ordered_example
```

## Results and focus

`changed` reports entry-to-final value difference independently of a handler.
`change_count` counts actual aggregate callbacks0/1, **not accepted edits**.
A net-zero event batch, pending draft, rejected value or focus-only change never
notifies. The callback runs after final semantics or complete Styled submission;
it receives copied previous/value and UNKNOWN aggregate cause. It cannot mutate
active UI/model/state/options/theme/token resources or recursively submit.

Validation rejects user candidates without clamping. Inspect result.validation
or state.validation; rejected_count counts original rejected events, not digits.
A valid edit clears transient validation. Explicit
`cui_time_field_clear_validation_v1` clears only that transient state;
OWNER_INVALID and an out-of-domain current owner still paint a danger border.
Unset OWNER_INVALID only when the application's separate validation clears.

On the ordinary route, all visible segments are global Tab stops.
The opt-in ordered frame above gives each field one logical stop instead.
The copied gallery example follows the selected size. See the
[original design](design/TIME_FIELD_LOCALIZED_WIDGET_V1.md)
and [binding contract](design/TIME_FIELD_BINDING_CHECKPOINT.md).

The gallery's TimeField source uses this route by default with Arabic token,
12-hour/seconds, inclusive-bound, readonly and invalid-appearance controls, plus
live value/change/validation labels. The headless test drives the gallery's
actual WebAssembly draw list and Chromium input.

Choose one side-effect path: observe the copied aggregate through
`cui_time_handler`, or branch on result.changed after submission. Doing both
should not apply the same application side effect twice. The widget has already
mutated the time through its pointer; do not reapply the callback payload. Check
result.status for operation errors and result.validation for rejected-edit state.
This does not imply uniform typed observer support in foreign bindings.
