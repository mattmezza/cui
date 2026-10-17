# Checkbox source contracts

This audit uses GPUI Kit v0.7.1, commit
`87d10ae5e1299d1be18670a37570c5402f952e34`, from the frozen checkout described in
[upstream-baseline.md](upstream-baseline.md). It reads actual Base and Styled
implementations and their tests, not just the existing parity ledger.

| Frozen source | SHA-256 |
| --- | --- |
| `crates/base/src/checkbox.rs` | `faa48acd7e1243ff661b542d753a6aa1327f76058837edb4cc00b3f469409883` |
| `crates/component/src/checkbox.rs` | `45cb50dac2d8194cc958d1776fcc2003cc03af132e3fde94289ff66e7c054e1e` |

The source distinguishes an unstyled Base primitive from the Styled facade.
Base owns controlled checked/unchecked/indeterminate requests, pointer focus,
keyboard activation, roles, tab traversal and accessibility metadata. Its separate
indicator part projects checked/mixed/disabled styles over an instance style and
renders arbitrary children. Styled composes that primitive with an indicator,
label/custom content, sizes, spring mark, tooltip and focus ring; pointer presses
explicitly preserve focus. Disabled activation is inert and can reach its parent.

## Shipped C mapping

| Contract | C API and evidence |
| --- | --- |
| Controlled state requests with activation metadata | `cui_base_check_ex`, `cui_check_options.on_change`, copied `cui_activation_snapshot` |
| Mixed activation becomes checked | `cui_base_check_ex`; semantic numeric value retains `CUI_CHECK_MIXED` |
| Base pointer focus; Styled pointer-focus suppression | Base default versus `cui_checkbox_source` |
| Signed focus order and traversal omission | `tab_index`, `skip_tab`; keyboard/programmatic focus remains available |
| Role override and accessible label | `custom_role`/`role`; Styled `accessibility_label` replaces only semantic name |
| Unstyled indicator and state refinement | `cui_base_check_indicator`, `cui_check_box_style`, `cui_check_box_styles` |
| Styled root/indicator/text refinement | `cui_checkbox_styles`, refining initialized `cui_checkbox_paint` |
| Arbitrary icon/indicator presentation | `indicator` callback receives size, state, disabled flag and spring progress |
| Label and application children | Existing `content` callback; bounded source content clips and uses 1.25-line-height labels |
| Check/mixed mark, spring and tooltip | Existing caller-owned `cui_checkbox_state`; reduced-motion and wake contracts retained |
| Sizes | Four frozen control metrics plus existing fourteen CUI design sizes, at logical densities 1 and 1.5 |

The additive source facade is the recommended entry point for the frozen
controlled/pointer-focus behavior. Existing `cui_checkbox`, `cui_checkbox_ex`,
`cui_base_checkbox` and `cui_base_check` keep their legacy direct-mutation and
pointer-focus defaults. `cui_checkbox_ex` can also use the new callback, style and
indicator seams. Radio aliases ignore Checkbox-only fields.

## Controlled example

```c
#include "cui/cui_components.h"

typedef struct {
    cui_check_state value;
    cui_checkbox_state motion;
    bool permit_change;
} preferences;

static void requested(void *user, cui_check_state value,
                      const cui_activation_snapshot *event) {
    preferences *app = user;
    /* event contains copied keyboard/pointer/modifier evidence for this request. */
    (void)event;
    if (app->permit_change)
        app->value = value; /* Reject by leaving the owner unchanged. */
}

static void scene(cui_context *ui, const cui_theme *theme, preferences *app) {
    cui_checkbox_source(ui, 100, theme, cui_string("Receive updates"), &app->value,
        (cui_checkbox_options){.on_change = requested, .change_user = app,
            .state = &app->motion, .width = 220, .tab_index = 2});
}
```

Call inside an active frame. Keep one motion state per stable ID. Every accepted
activation emits a request; multiple accepted events in one frame each receive
the next value computed from that submission's original value. Callback presence
means the helper never writes the owner. The owner can accept synchronously so
this frame's paint and semantic state reflect its committed value. A source
facade without a callback is read-only. Base callers can select the same behavior
with `controlled=true`; the zero-default Base extended options retain direct
mutation when no callback is supplied.

The callback's event pointer is valid only during the call. Copy the snapshot if
it must outlive the callback. Frame/event indices identify events within the
originating context; they are not global IDs. No host event data is fabricated.
Disabled, inert and layout-only submissions do not dispatch accepted callbacks.
The Base helper draws nothing: put it inside your own same-ID box and compose
children yourself.

## Styles and indicator slots

Styled style callbacks refine initialized defaults in this order: `instance`,
`checked` **or** `mixed`, then `disabled`. Each receives the current committed
state. Modify `paint.root`, `paint.indicator`, glyph/text colors, label font size,
line height or `label_style`. Keep style callbacks confined to refinement; use
existing hover/focus queries for state-dependent adjustments, and put UI content
in `content`/`indicator` callbacks. Those content callbacks run during submission
and must obey the ordinary inert/layout-only contract; do not perform application
side effects there.

A custom `indicator` replaces built-in check/dash painting, while preserving the
indicator box and semantics on the root. Its info includes checked/mixed spring
progress for animated custom icons. Use caller-owned immutable icon resources
through renderer completion. Without an override, the existing built-in marks
remain in use. Base applications use `cui_check_box_style` to project styles onto
their own root box, and `cui_base_check_indicator` for the noninteractive child
part; it creates no redundant accessibility control.

CUI box/text fields are the supported style vocabulary. These seams do not claim
byte-for-byte equivalence to GPUI's entire inherited `StyleRefinement` language.
Platform-specific focus handles map to stable CUI IDs and explicit focus scopes;
there is no Rust/OS focus-handle object in the portable API. Native accessibility
transport depends on the host adapter and is not proved by semantic snapshots.

## Existing motion, labels and tooltip contracts

A zero-initialized `cui_checkbox_state` snaps to the initial value. Keep one state
per stable Checkbox ID through `cui_end_frame`; it allocates nothing and needs no
cleanup. Reusing it for a different ID resets motion and tooltip. Checked and
mixed channels crossfade and reverse from current progress/velocity using
`theme.spring_control` response, damping, epsilon and travel settings. Painted
alpha clamps underdamped overshoot. Response zero, travel false, Core reduced
motion or `options.reduced_motion` snaps immediately; logical selection never
waits for paint. A NULL state retains immediate marks.

`state.next_wake_seconds` is an absolute Core frame deadline while spring motion
or a delayed tooltip is active; zero is quiescent. The component also invalidates
Core. Honor deadlines alongside input redraws. Nonfinite or exceptionally large
Core clocks do not produce invalid deadlines.

Positive `width` bounds the fit-height label column while reserving indicator and
gap space. Text wraps; custom `content` follows the label. Give children intrinsic
height or an intentional constraint. Oversized words need enough width or an
appropriate scroll/clipping ancestor; no ellipsis or arbitrary word-breaking is
introduced.

Tooltip text requires motion state and a valid `tooltip_options.viewport`. The
component supplies trigger, frame delta and disabled state and delegates
placement, delay, keyboard focus and Escape behavior to the existing Tooltip.
Targeted capture cancellation suppresses it until hover/focus leaves. Removing
text clears tooltip state. Legacy touch activation can focus and reveal help;
the source facade preserves focus on touch/pointer activation, so use hover or
keyboard focus for its tooltip. Choose dimensions for the supplied tooltip text;
its existing clipping contract remains in force.

```c
static cui_checkbox_state motion;
static cui_check_state checked;
void preferences(cui_context *ui, const cui_theme *theme, cui_rect viewport) {
    bool changed = cui_checkbox_ex(ui, 42, theme, cui_string("Receive updates"), &checked,
        (cui_checkbox_options){.state = &motion, .width = 240,
            .tooltip = {"Email notification preferences", 30},
            .tooltip_options = {.viewport = viewport, .open_delay = .2f, .close_delay = .1f}});
    if (changed) { /* Persist the legacy directly mutated value. */ }
}
```

The existing motion fixture also has a real WASM/browser contact proof:
`uv run tests/web/test_checkbox_motion.py --chromium /path/to/chrome-headless-shell`.
That fixture exercises marks, tooltip/keyboard behavior, sizes and wrapped labels;
it is not physical-device or new source-callback verification.

## Tests and current limits

`tests/unit/test_checkbox_source.c` checks owner accept/reject/read-only behavior,
per-event keyboard requests, mixed state, mouse/touch metadata, touch cancellation,
current-frame inert/layout-only suppression, disabled parent activation, Base vs
Styled pointer focus, signed tab ordering/omission, role/name/required/invalid
semantics, style precedence, actual slot/indicator/root geometry and text sizing.
It also exercises all fourteen design sizes at two logical densities. Existing
`test_checkbox_motion`, `test_components` and `test_base` continue to check legacy
controls and springs/tooltips.

Headless evidence uses strict portable C11 warnings and the isolated CTest
wrapper. It does not establish full family verification, native accessibility
delivery, screenshot equality across renderers, or new current-frame hit-routing
guarantees beyond Core's documented previous-region event model. Final manifest
status belongs to the integrator after the frozen-source audit and complete
acceptance proof. No dependency or upstream runtime is introduced.

For AI consumers, use `cui_base.h` and `cui_components.h` as signature authority,
this guide as the Base/Styled behavioral distinction, and the generated API index
for exact exports. Do not infer controlled callbacks or pointer-focus suppression
from the older boolean convenience functions.
