# Proposed Button style facade

Historical design checkpoint. The bounded implementation has since landed in the
integration branch; use [the current API guide](../BUTTON_STYLES.md) for implemented
names, behavior and remaining limitations. The proposal below records its review scope.

**Design only; no API below exists yet.** This checkpoint follows the built-in
state corrections in `2dceccce`. Production approval covers only that earlier
slice. This document proposes an additive portable C11 facade; it neither
changes the parity matrix nor completes arbitrary GPUI `Styled` support.

## Contract selected for review

Model **Component Button caller-instance replay**, not the Base control's
instance → selected → disabled precedence. Frozen authority is GPUI Kit 0.7.1
`87d10ae5e1299d1be18670a37570c5402f952e34`:
[Component Button lines 600–809](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/component/src/button/button.rs)
clones caller instance style, applies it after defaults, and replays it inside
the selected and disabled semantic layers. Consequently an explicit instance
background can keep its color while disabled. A disabled callback is not a
permission or activation override.

[Base Button](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/base/src/button.rs)
uses the [shared state resolver](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/base/src/state_style.rs):
selected then disabled replace instance values. CUI's existing Checkbox facade
also invokes instance, checked/mixed, disabled sequentially. Button will reuse
its initialized paint-record and `user` conventions, **not its precedence**.
Documenting this distinction is necessary; this proposal cannot satisfy both
conflicting contracts simultaneously.

## Proposed public surface

Proposed new `include/cui/cui_button_styles.h`, including existing component
types. No platform or Clay types, heap allocation, ownership transfer or
additional input dispatch. Names remain subject to review. The production record is named
`cui_button_style_paint` because `cui_button_paint` is already a legacy C function.

```c
typedef struct {
    cui_box root;          /* Resolved box; geometry is inspection-only. */
    cui_color foreground;  /* Label and inherited icon/content ink. */
} cui_button_style_paint;

enum {
    CUI_BUTTON_PAINT_BACKGROUND = 1u << 0,
    CUI_BUTTON_PAINT_BORDER     = 1u << 1,
    CUI_BUTTON_PAINT_FOREGROUND = 1u << 2,
    CUI_BUTTON_PAINT_RADIUS     = 1u << 3,
    CUI_BUTTON_PAINT_CORNERS    = 1u << 4,
    CUI_BUTTON_PAINT_OPACITY    = 1u << 5
};
typedef struct {
    unsigned fields;
    cui_button_style_paint paint;
} cui_button_paint_patch;

typedef enum {
    CUI_BUTTON_STYLE_INSTANCE,
    CUI_BUTTON_STYLE_HOVER,
    CUI_BUTTON_STYLE_ACTIVE,
    CUI_BUTTON_STYLE_SELECTED,
    CUI_BUTTON_STYLE_DISABLED,
    CUI_BUTTON_STYLE_FOCUS
} cui_button_style_phase;
typedef struct {
    cui_id id;
    cui_button_style_phase phase;
    bool hovered, pressed, focus_visible;
    bool selected, open, disabled, loading;
} cui_button_style_info;
typedef void (*cui_button_style_fn)(void *user,
    const cui_button_style_info *info,
    const cui_button_style_paint *current,
    cui_button_paint_patch *out);
typedef struct {
    cui_button_style_fn instance, hovered, active, selected, disabled, focused;
    void *user;
} cui_button_styles;

bool cui_button_styled_ex(cui_context *, cui_id, const cui_theme *, cui_str,
                         cui_button_options, cui_button_styles);
void cui_button_styled_paint(cui_context *, cui_id, const cui_theme *, cui_str,
                            cui_button_options, cui_button_styles);
```

`styled_ex` uses the existing Button submit/reduce/metadata path exactly once,
returning its existing aggregate bool. Existing option callbacks run at their
existing activation gates/order. `styled_paint` is the corresponding passive
paint seam: it must not register input, submit semantics or reduce events.
Zero styles give the same output as the corresponding existing API. This is
not an implicit wrapper around `cui_button_ex` followed by a second paint call.
No new event-observer overload is included in this slice; existing event and
composition integrations need a separate explicit compatibility review before
claiming they can accept these style callbacks.

`current` is fully initialized for this phase. `out` starts zero on every call.
Only `fields` grants write presence: a zero field mask means no refinement;
transparent black, zero opacity, zero radius and all-zero corners remain
explicit valid values when their bits are present. Do not infer presence by
comparing the result with defaults, or by testing colors/numbers for zero.

Allowed root writes are background, border color, uniform radius, corner
radii and opacity. CORNERS copies `root.custom_corners` and
`root.corner_radii` together; false explicitly restores uniform radius.
OPACITY uses `root.opacity` as the explicit value and sets
`root.custom_opacity=true`, including for zero. It does not copy an ambiguous
zero-initialized `custom_opacity` flag. If both radius and corners are supplied,
existing box corner precedence applies. Foreground copies one RGBA value.

Every other `cui_box` member is inspection-only and never copied from a patch:
width/height, padding, gaps, border widths/edges, clipping, alignment, offsets,
inertness, layout-only, grow/percentage flags and caps. Neither font sizing nor
content layout is exposed. This prevents a callback from changing the paint
box while measurements, hit targets, semantic bounds and slot layout still
use the old box. Border thickness remains under existing options; border
color is independently customizable. Radius/corners change decoration only;
existing rectangular hit testing is unchanged. The API must not promise
shape-aware hit testing.

Reject a whole patch if it has unknown presence bits or a present numeric
field is nonfinite, negative radius/corner, or opacity outside 0..1. Ignore it
without poisoning the Core frame, preserve the preceding paint, and continue
later eligible callbacks. Read-only writes are ignored, even if their values
are invalid. A patch mask is bounded and captured by value for replay; no
pointer into `out` survives the call.

## Exact resolution and callback timing

A private common resolver must separate defaults from patch application and
must not reconstruct presence by diffing mutable records. A callback runs at
most once per submission, including `instance`; replay reapplies its captured
patch without calling application code again.

1. Initialize normal built-in paint from the existing options/theme, after
   existing sizing/content measurement decisions. Invoke `instance`, validate
   and capture its patch, then apply it.
2. If interactive, not selected/open and pressed, apply built-in active paint
   and the `active` callback patch. Otherwise, if interactive, not
   selected/open and hovered, apply built-in hover paint and `hovered` patch.
   Pressed suppresses hovered. No instance replay occurs in these pointer
   layers: these are explicit opt-in interaction refinements.
3. If selected or open, apply built-in selected paint, then invoke/apply
   `selected`, then replay the captured instance patch. The callback runs once
   when both selected and open are true. `info.selected` and `info.open` retain
   their separate option values; neither implies toggled semantics.
4. If disabled, apply built-in disabled paint, then invoke/apply `disabled`,
   then replay the instance patch. This follows selected when both are true.
   Fields not supplied by instance retain the disabled result. A disabled
   handler's background loses to an explicit instance background; this is the
   chosen Component replay contract, not a bug or Base-style precedence.
5. If loading and not disabled, apply the existing whole-control 0.8 fade to
   the resolved paint. Loading suppresses hovered/active callbacks but does
   not invoke disabled. It can still invoke selected for selected/open.
   Fade composes with explicit root opacity (0.5 becomes 0.4); disabled skips
   this fade. This explicit CUI composition rule is scoped behavior, not a
   claim about every GPUI arbitrary opacity modifier conflict.
6. Apply existing focus paint, then invoke/apply `focused` only when its
   existing focus-visible and hide-focus-ring gates permit. No instance replay
   follows it. Focus remains the final decoration layer; this slice does not
   change focus geometry or add the source outer-ring theme mode.

Here “interactive” means neither disabled nor loading. Raw hover/pressed flags
in `info` are the stable interaction snapshot used by the resolver; they do
not override eligibility. Focus uses existing behavior, including a retained
focus-visible state during loading/disabled transitions. `focused` must not be
advertised as a disabled-eligibility escape. Keep the legacy resolver's focus
gates until the separately reviewed focus slice changes them.

Callbacks are synchronous, borrowed for this submission only. `user`, function
pointers and all pointed-to data must remain valid until the call returns;
none is retained across frames. `current`, `info` and `out` are temporary and
must not escape. Callbacks may read their owner model and fill paint patches.
They must not call CUI APIs, queue/consume events, submit children, change the
owner model, options or styles, or trigger reentrant submission in the same
frame. The callback intentionally receives no `cui_context *`. This is a paint
calculation, not dispatch or an owner event callback. It cannot intercept,
cancel, duplicate or manufacture activation. Existing slot callbacks retain
their existing separate content contract.

## Intended usage (proposed API only)

This C11 example is intended to compile against the future header. It is not
an example of an API available in the current SDK.

```c
#include "cui/cui_button_styles.h"

typedef struct { cui_color hover, selected, disabled; } save_style;
static void save_hover(void *user, const cui_button_style_info *info,
                       const cui_button_style_paint *current,
                       cui_button_paint_patch *out) {
    const save_style *s = user;
    (void)info;
    (void)current;
    out->fields = CUI_BUTTON_PAINT_BACKGROUND;
    out->paint.root.background = s->hover;
}
static void save_selected(void *user, const cui_button_style_info *info,
                          const cui_button_style_paint *current,
                          cui_button_paint_patch *out) {
    const save_style *s = user;
    (void)info;
    (void)current;
    out->fields = CUI_BUTTON_PAINT_BACKGROUND;
    out->paint.root.background = s->selected;
}
static void save_disabled(void *user, const cui_button_style_info *info,
                          const cui_button_style_paint *current,
                          cui_button_paint_patch *out) {
    const save_style *s = user;
    (void)info;
    (void)current;
    out->fields = CUI_BUTTON_PAINT_BACKGROUND;
    out->paint.root.background = s->disabled;
}
static bool save_button(cui_context *ui, const cui_theme *theme,
                        save_style *style, bool selected, bool disabled) {
    return cui_button_styled_ex(ui, 42, theme, cui_string("Save"),
        (cui_button_options){.selected = selected, .disabled = disabled},
        (cui_button_styles){.hovered = save_hover, .selected = save_selected,
                           .disabled = save_disabled, .user = style});
}
```

With no instance background patch, selected/disabled callbacks determine their
respective backgrounds; disabled wins when both are active. Adding an instance
background patch deliberately pins that color across both semantic states.
Owner state updates happen after `save_button` returns true or in existing
activation callbacks, never inside these style handlers.

## Implementation acceptance and limits

Production work requires approval of this exact contract first. A private
Button common hook must serve legacy and styled submissions without copying
or reordering the activation loops. Do not add capture/bubble handling,
retained style state, a new size enum or a theme change. This is a bounded
paint facade: arbitrary GPUI layout/typography/gradient/shadow/child `Styled`
modifiers and private compound `content_style` are explicitly unsupported.

Required tests: zero-style equivalence with legacy draw lists/results;
explicit transparent/zero values and no-mask preservation; instance once with
value replay, selected/open once and disabled ordering; instance collision
wins versus selected/disabled, pointer callbacks win their interaction layer;
pressed suppresses hover; loading/disabled eligibility and recovery; opacity
composition; focus gate/order; rejected numeric/unknown-mask patches; ignored
geometry and border-width writes; identical actual bounds, semantic bounds,
hit targets and slot layout; actual software pixels plus text/border draw
colors; passive paint has no semantics or reduction. Compare pointer, keyboard
and AT multiplicity/callback order with legacy APIs. A real usage fixture must
compile the owner example against the actual new header after implementation.

Run affected native tests only through the private headless wrapper, strict
portable C11 warnings, and headless browser tests if web integration changes.
This docs-only checkpoint runs no runtime suite. Root owns canonical test
registration, public metadata, generated SDK/distribution and skill refresh.
