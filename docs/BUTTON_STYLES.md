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
