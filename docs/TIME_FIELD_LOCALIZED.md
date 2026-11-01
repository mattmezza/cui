# Localized TimeField widget (unreleased)

The additive C route has passed focused headless interaction/pixel and portable
compile checks; it remains unreleased. Legacy TimeField is unchanged. See the
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
    return cui_time_field_v1(ui, 20, theme, cui_string("Reminder"),
        &owner->state, &owner->value, &owner->options,
        (cui_time_handler){reminder_changed, owner});
}
```

Call compose during an ordinary frame. First/gap/recovery submissions establish
geometry without applying queued edits. Disabled, readonly, inert and layout-only
submissions freeze edits and notifications. Editing uses original-order targeted
Core events, copied UTF-8 text payloads, token prefixes across TEXT events and
atomic full formatted paste up to160bytes. Preedit/composition and host clipboard
shortcuts are not implemented by this widget. No input dispatcher is added.

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

All visible segments are ordinary global Tab stops. Forward/backward navigation
exits at boundaries; the widget does not consume Tab or trap focus. This differs
from the upstream one-root focus/selection-retaining behavior, which remains
unfinished. Full source size/focus-ring styling, localized DateTimePicker, IME,
physical native accessibility and Python/JS/TS/Go/Rust options/events remain open.
Old scalar bindings and old by-value options/aggregate callbacks retain their
existing behavior. See [design and test contract](design/TIME_FIELD_LOCALIZED_WIDGET_V1.md).

The gallery's TimeField source uses this route by default with Arabic token,
12-hour/seconds, inclusive-bound, readonly and invalid-appearance controls, plus
live value/change/validation labels. Its C translation unit has been compiled;
actual combined WASM rendering and browser interaction are integrator validation,
not established by this worker's native proof.

Choose one side-effect path: observe the copied aggregate through
`cui_time_handler`, or branch on result.changed after submission. Doing both
should not apply the same application side effect twice. The widget has already
mutated the time through its pointer; do not reapply the callback payload. Check
result.status for operation errors and result.validation for rejected-edit state.
This does not imply uniform typed observer support in foreign bindings.
