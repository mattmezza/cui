# Bounded Button paint styles

Include `cui/cui_button_styles.h` for `cui_button_styled_ex` and the passive
`cui_button_styled_paint`. These additive APIs use the existing Button paths.
Zero styles preserve legacy pixels and results. Activation callbacks, input,
semantics, sizing and slots retain their contracts. There is no event-facade
or group bridge. This bounded API does not complete arbitrary Styled parity.

The record is named `cui_button_style_paint` because the proposed name
`cui_button_paint` conflicts with the existing legacy C function, which keeps
its name. It contains initialized root box and foreground values. The dropdown
caret retains its independent normal-ink rule.

This real C11 owner example is compiled by the CMake consumer fixture:

```c
#include "cui/cui_button_styles.h"

typedef struct {
    bool selected, disabled;
    unsigned saves;
    cui_color hover, selected_fill, disabled_fill;
} save_owner;

static void save_style(void *user, const cui_button_style_info *info,
                       const cui_button_style_paint *current,
                       cui_button_paint_patch *out) {
    const save_owner *owner = user;
    (void)current;
    if (info->phase == CUI_BUTTON_STYLE_INSTANCE) {
        out->fields = CUI_BUTTON_PAINT_RADIUS;
        out->paint.root.radius = 0;
    } else {
        out->fields = CUI_BUTTON_PAINT_BACKGROUND;
        out->paint.root.background = info->phase == CUI_BUTTON_STYLE_DISABLED
            ? owner->disabled_fill : info->phase == CUI_BUTTON_STYLE_SELECTED
            ? owner->selected_fill : owner->hover;
    }
}

bool save_owner_draw(cui_context *ui, cui_id id, const cui_theme *theme,
                     save_owner *owner) {
    bool activated = cui_button_styled_ex(ui, id, theme, cui_string("Save"),
        (cui_button_options){.selected = owner->selected, .disabled = owner->disabled},
        (cui_button_styles){.instance = save_style, .hovered = save_style,
                           .selected = save_style, .disabled = save_style,
                           .user = owner});
    if (activated)
        ++owner->saves;
    return activated;
}
```

Each handler receives a stable submission snapshot and zeroed output patch.
Only `fields` grants presence: transparent black, zero opacity, zero radius and
zero corners are valid explicit values. Missing bits preserve preceding paint.
Supported fields are background, border color, foreground, radius, corners and
opacity. CORNERS copies `custom_corners` plus radii, including false to restore
uniform radius. OPACITY always enables the supplied value, including zero;
it does not require a `custom_opacity` flag in the patch.

Handlers are synchronous borrowed pure paint calculations. Functions, `user`
and owner data survive submission; nothing is retained across frames. Temporary
`info`, `current`, `out` must not escape. Do not call CUI, queue/consume input,
submit children, mutate owner/options/styles, or reenter submission. The callback
has no context parameter. Update the owner after activation returns or through
existing activation callbacks, never in a style calculation.

Resolution follows Component caller-instance replay:

1. Normal defaults, then instance callback; capture its patch once.
2. Eligible active or hover defaults and callback. Pressed suppresses hover.
3. Selected/open defaults and selected callback, then replay instance values.
4. Disabled defaults and disabled callback, then replay instance values.
5. Loading multiplies resolved root opacity by 0.8 unless disabled.
6. Existing focus paint, then focused callback when focus-visible and ring shown.

Selected/open, disabled and loading suppress pointer callbacks. Selected runs
once when either or both flags are true. Loading can retain selected paint,
does not invoke disabled, and composes explicit opacity (0.5 becomes 0.4).
Disabled skips loading fade. Instance is called once even when replayed twice.
An explicit instance background wins over selected/disabled background patches;
leave its background bit absent to allow those layers to choose different
colors as the example does. This intentionally differs from Base/Checkbox
semantic-state-over-instance order. Loading composition is a documented CUI
rule, not a claim about every GPUI arbitrary opacity conflict. Focus geometry
is unchanged and its existing gates still apply through state transitions.

All root geometry is inspection-only. Width/height, padding/gap, border
thickness/edges, clipping, alignment/offsets, input/semantic flags and every
other unlisted member are ignored in patches. Radii change decoration, not
rectangular hit testing. This keeps paint placement, hit targets, semantic bounds
and content measurement aligned. Fonts, child layout, shadows, gradients,
private compound content styling and unrestricted GPUI modifiers are outside
this API. A callback does not intercept or manufacture activation.

Unknown mask bits or invalid present numeric values discard the whole patch
without setting a Core error; later eligible callbacks still run. Present
radii/corners must be finite and nonnegative; present opacity finite 0..1.
Invalid values in unselected or inspection-only fields are ignored. Presence
is never inferred by diffing values or treating zero as unset.

The passive API paints an independently registered Button without semantics
or input reduction. Existing registration and frame-resource lifetimes still
apply. Style lifetimes end at submission return; text/image renderer lifetimes
remain unchanged.

The standalone fixture compiles this exact example and checks zero/no-op pixel
equivalence, mask presence, replay/eligibility, invalid patches, protected
geometry, software pixels and pointer/keyboard/AT callback order. Its checks
stay active under NDEBUG. Software comparisons omit text rasterization in
portable builds; text colors and placement are checked from real draw commands.

```sh
cmake -S tests/fixtures/button_styles -B /path/to/private-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build /path/to/private-build --target test_button_styles button_styles_guide
python scripts/test_local.py --build-dir /path/to/private-build -- -R '^button_styles$'
```

Root owns canonical registration, metadata and SDK/distribution/skill refresh.
Consumers adopting this header must rebuild CUI and bindings from matching
headers/source; there is no promise of compatibility with an older library.

Worker validation: strict Release build and eight affected Button tests passed
through the private wrapper. The new fixture also passed AddressSanitizer plus
UndefinedBehaviorSanitizer with leak detection enabled, and the header passed
strict C++11 syntax checking. The test compares legacy descriptor mutation and
per-event callback multiplicity as well as passive zero-style equivalence.

## Shadow-aware Button surface facade

Development additions after alpha.3: use a matching current source checkout and
headers/facades. These APIs are absent from the published alpha.3 packages.

`cui_button_surface_ex` and passive `cui_button_surface_paint_only` are separate
additive C APIs in the same header. Existing styled records, signatures, numeric
rules and callbacks keep their behavior. The surface facade adds an owned fixed
array of at most `CUI_BOX_SHADOW_MAX` (eight) mixed DROP/INSET layers. It adds no
builtin/default shadow and does not add gradients, fonts, border styles or source
focus rings. Complete source state-style refinement remains incomplete; see
[the source audit](design/BUTTON_STYLE_COMPLETION.md).

Prepare typed layers outside the pure style callback:

```c
#include "cui/cui_button_styles.h"

typedef struct { cui_box_shadow elevation; } raised_button;

static void raised_style(void *user, const cui_button_style_info *info,
                         const cui_button_surface_paint *current,
                         cui_button_surface_patch *out) {
    const raised_button *owner = user;
    (void)current;
    if (info->phase == CUI_BUTTON_STYLE_HOVER) {
        out->fields = CUI_BUTTON_SURFACE_SHADOWS;
        out->value.shadow_count = 1;
        out->value.shadows[0] = owner->elevation;
    }
}

bool raised_button_draw(cui_context *ui, cui_id id, const cui_theme *theme,
                        raised_button *owner, cui_button_surface_status *status) {
    if (!cui_box_shadow_from_css(0, 3, 8, -1, (cui_color){0, 0, 0, 64},
                                CUI_BOX_SHADOW_DROP, &owner->elevation))
        return false;
    return cui_button_surface_ex(ui, id, theme, cui_string("Save"),
        (cui_button_options){0},
        (cui_button_surface_styles){.hovered = raised_style, .user = owner}, status);
}
```

`cui_button_surface_paint` contains initialized scalar `paint`, `shadow_count`
and owned `shadows`; patch scalar bits are the existing six bits, with scalar
values under `out->value.paint`. Root geometry stays inspection-only.
`CUI_BUTTON_SURFACE_SHADOWS` grants complete-list presence: absent inherits the
preceding phase, present zero count clears both DROP/INSET, present nonempty
replaces rather than appends. Zero scalar values remain explicit values.
Every submission recomputes defaults; no callback or list persists next frame.

Phase eligibility/order is the existing styled order above. Disabled clears the
shadow list before its callback, then replays the captured instance patch.
Consequently an explicit instance list (including empty) wins selected and
disabled lists, while an absent instance list allows their own lists. Instance
runs once even if replayed twice. Pointer color defaults leave the list alone;
loading fades the complete root; the focused callback runs after existing focus
paint. Final root corners determine the shadow shape. This does not change the
existing rectangular hit targets or claim complete source focus-ring parity.

Callbacks follow the same borrowed pure-calculation rules above. The facade
copies descriptors before accepted action callbacks, and copies each returned
patch before later phases/content. Do not call CUI from a style calculation;
construct presets/CSS layers beforehand. Content callbacks retain their normal
submission rules. Root decoration encloses actual custom-slot geometry without
submitting children twice or recoloring arbitrary child paint; the caret keeps
its independent normal-ink rule.

The NEW facade bounds present radii/corners to finite `[0,16384]`, opacity to
finite `[0,1]`, count to eight, sigma to `[0,256]`, offsets/spread to
`[-4096,4096]`, with valid DROP/INSET kind. A nonempty list also requires the
effective options-derived root radii to fit the shadow bounds. Unknown bits or
invalid present values discard the whole phase patch, preserve preceding paint,
and continue later eligible callbacks. Unused layer entries, absent values and
inspection-only geometry writes are ignored. Existing styled radius rules remain
finite/nonnegative without this new upper bound. CSS blur radius `[0,512]`
converts explicitly to `sigma = radius / 2` as described in [ELEMENT_SHADOWS.md](ELEMENT_SHADOWS.md).

The optional status output is cleared on entry, even when no submission occurs.
Each rejected callback patch sets `1u << phase` in `rejected_phases`; this reports
authoring errors independently of the ordinary bool activation result. Passing
NULL suppresses this report. Rejected phase patches do not poison the Core frame;
ordinary frame/renderer failures still follow their existing error paths. The
passive API paints an independently registered owner without reducing input or
adding semantics. It does not manufacture action callbacks.

Decoration uses the existing Core copied-layer path and its frame/resource,
clipping, opacity and rendering limits. Layout-only work allocates no shadow
paint. A later named-element setter replaces the list, and a retained bridge
root override is applied after owner paint and also wins. This C facade has no
event/group overload and no Python/JS/TS/Go/Rust phase authoring binding. Those
builders' existing static root shadows remain their separately bounded API.

Focused tests freeze 60 pre-change old-library command/software/event-state
triples across six legacy routes. New tests cover phase replay, explicit clear,
transactional rejection, options-derived shape, eight mixed layers, copied
lifetimes, movement/resize, ancestor clips/opacity, ordinary-only independent
software pixels, passive behavior and descriptor mutation during real accepted
callbacks. Text command content is checked separately from fontless software
paint. Linux portable software proof does not establish hardware GPU, browser
or complete source paint parity.
