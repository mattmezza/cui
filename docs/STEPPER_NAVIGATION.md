# Application-controlled Stepper navigation

Include `cui/cui_stepper_navigation.h` for `cui_stepper_navigation` and its
unstyled Base sibling. They take a selected index by value and report a typed
REQUEST for each accepted item activation. The application decides whether to
update its own selection. The existing Stepper functions still mutate their
selected-index pointer and the existing aggregate CHANGE observers retain their
timing and meaning.

This closes a specific frozen-source gap: upstream's controlled selected_index
and on_click let the application decide navigation, including a click on the
already selected step. Upstream has no built-in validator or next/back controller.
The compiling [`examples/stepper_navigation/main.c`](../examples/stepper_navigation/main.c)
rejects a confirmation request while payment data is invalid, then accepts a
later request after the application validates it.

The callback receives captured `selected`, `requested` and `count`. A same-index
activation is still a request. Several accepted activations produce several
requests in event order, even when their final target equals the entry selection.
All requests use the same captured selected value. The current call registers
semantics and paints that captured selection throughout; owner updates become
visible on the next submission. A rejected request simply leaves the owner's
selection unchanged. There is no veto return, automatic selection, normalization,
async validation, form ownership or automatic error-status mutation.

`result.valid` means argument preflight succeeded, not completed layout or visible
rendering. `request_count` counts accepted requests independently of a handler.
A missing handler leaves selection untouched and still counts input. Preflight
requires nonzero ID, count <=100, structurally valid title/description pairs and
valid status enums. For nonempty steps selected must be in range; for empty steps
it must be zero. Invalid input registers nothing and dispatches nothing. This is
deliberately stricter than legacy clamping and upstream unchecked high indices;
invalid-input equivalence is not claimed.

Current semantics must successfully accept the activation. Current disabled or
omitted items, cancellation, inert/layout-only work and rejected accessibility
actions do not generate requests. Consumption follows Core's exact policy:
consuming accessibility input cancels its acceptance; consuming an ordinary
already generated Enter activation does not universally revoke it. This helper
does not consume input. Each request freezes its genuine accepted activation
snapshot before calling the application. Metadata targets the Stepper; the
embedded activation targets its item. Frame, index and pointer/keyboard/
accessibility source are evidence, not inferred cause. Copied payloads remain
usable after callback, frame and context lifetime.

Handler/user, steps, strings and theme are borrowed through the call and never
retained. Keep the borrowed array, strings and theme stable until return. The
handler and by-value arguments are captured before callbacks. Store validation
errors in separate owner fields, then rebuild step descriptors on the next
submission; changing a borrowed status in a callback would mix old semantics
with new paint and is unsupported. The example follows this owner pattern.
The callback runs after current semantic registration and before styled painting.
It may update unrelated application form/selection state. Do not mutate the
borrowed descriptors or active input routing/options, recursively submit the
component, destroy the UI/storage, or unwind a foreign exception. Updating the
application owner cannot change this call's captured selection. Further callbacks
still carry that captured value. Later mutation cannot rewrite a payload already
delivered; future input acceptance is not promised to survive forbidden routing
changes.

Styled rendering reuses the legacy private paint path, including horizontal and
vertical layout and explicit caller error/complete statuses. This legacy paint path does not
add upstream icons, arbitrary children, sizing, checked separators, ListItem
hierarchy or live AT announcements. The opt-in visual facade below adds a bounded
connector/icon composition. There is no Python/Node/TypeScript/Go/Rust
binding surface for these requests yet. Source and platform parity remain partial.

## Opt-in connectors and custom icons

Include `cui/cui_stepper_visual.h` and call `cui_stepper_visual_navigation` to
choose non-centered indicator/connector composition. The legacy functions above
keep their previous button/status rendering and callback timing. This facade
calls the same Base request reducer once; selection remains application-owned.
It has no loading state. Decorations add no focusable or semantic targets.

```c
#include "cui/cui_stepper_visual.h"
void draw_steps(cui_context *ui, const cui_theme *theme, size_t selected,
                cui_stepper_navigation_handler handler) {
    const cui_step steps[] = {
        {.title = CUI_STR("Start")},
        {.title = CUI_STR("Review"), .description = CUI_STR("Check details")},
        {.title = CUI_STR("Finish")}
    };
    (void)cui_stepper_visual_navigation(ui, 100, theme, steps, 3, selected,
        (cui_stepper_visual_options){.density = CUI_STEPPER_DENSITY_MD}, handler);
}
```

Density selects indicator diameters8/18/24/32 for XS/SM/MD/LG and connector
thickness1.5/2/2/3. A separator follows each item except the last. Passed
connectors use primary when their index is less than selected; passed/current
indicators use primary even when disabled. Caller COMPLETE/ERROR statuses do not
determine passed color. ERROR retains a visible CUI-specific label. Other explicit
status labels remain available through legacy rendering, not this source-style
facade. Source pixels, theme sizes and general style refinements are not identical.

To replace ordinals, pass an array of `cui_stepper_icon` with exactly one entry per
step. Set `present` and a valid `cui_bitmap` on entries to replace. Unset entries
use their ordinal. XS suppresses both icon and ordinal, but still validates all
present bitmaps. Existing Icon state can rasterize an SVG before submission; the
facade does not load/rasterize assets. Icons use foreground tint multiplied by
source RGB and preserve source alpha; use white RGB for a monochrome alpha mask.
All images are straight-alpha RGBA8 under `cui_bitmap_valid`: nonnull pixels,
nonzero dimensions<=16384, stride>=width*4, and height*stride without overflow.
This cannot validate the caller's allocation length; provide that full storage.

Options, descriptor arrays, strings and theme are borrowed through return and
must stay stable during callbacks. Bitmap metadata is copied into draw commands;
no stack icon-descriptor pointer is retained. **Pixel bytes must remain valid
through renderer completion**, and revision must change when they change. Invalid
icons/count/density fail before semantic registration or callbacks. `valid` is
argument preflight, not completed layout/resource-capacity or visibility proof.
The returned request count and callback restrictions match the existing API.

Centered text/separators, responsive flex growth, arbitrary child content/style,
exact source density and language builders remain unfinished. Follow-up slices
should add independently tested centered/flexible geometry and then theme/style
controls without changing the request reducer. Native software tests cover this
bounded composition and actual pointer/touch/keyboard requests. The WASM gallery
fixture additionally covers desktop DPR1/2 and touch390 DPR2 for the bounded
visual facade. Live assistive-technology acceptance remains untested.

`options.design_size` accepts the existing shared XS/SM/BASE/MD/LG/XL/2XL..9XL
scale. INHERIT (zero) leaves the source-density dimensions above unchanged.
Explicit design sizing resolves shared metrics at `theme->font_size / 16`: source
indicator diameter, separator thickness and connector lengths scale by
`metrics.indicator_size / 16`; labels use shared font size and spacing uses shared
gap. Thus `density` selects source proportions/suppression while `design_size`
selects the app's shared scale. All tokens are tested, including9XL; invalid
tokens or invalid resolved density fail before semantics/callbacks. These metrics
do not claim frozen GPUI pixel fidelity or responsive/centered layout completion.


## WASM gallery

The Stepper story defaults to its original legacy button rendering and controlled
validation demo. "Rendering" opts into indicators/connectors. Orientation applies
to both renderers; source density, optional application-owned RGBA bitmap icons
and shared design scale appear only for visual rendering. "Source density" in
the design control keeps INHERIT; other choices use the public shared XS..9XL
metrics. These are independent from the source-density proportions. Leaving the
story hides and resets its controls; selection/validation remain application-owned.

The Details confirmed checkbox gates later steps. Rejected requests still count;
the current step stays unchanged. Same-index requests count too. Disabling the
story suppresses interaction without erasing progress. The displayed module snippet
tracks the chosen API/options and includes the controlled owner callback; the icon
variant includes real static RGBA bytes with renderer-safe lifetime. The application
must set `owner.confirmed` from its form before submission.

`tests/web/test_stepper_visual_gallery.py` verifies actual WASM mouse, touch and
keyboard requests, rejection/disabled behavior, connector geometry and Canvas
pixel composites at DPR1/2 (including fractional strokes), image alpha/tint,
source-density glyph suppression, one explicit shared design token and route
reset. It compiles the exact displayed C snippets with strict C11 warnings.
Desktop checks include both axes; touch390 uses vertical composition. Centered
layout, automatic responsive flex growth, arbitrary styling and exact source
pixel fidelity remain unfinished. The gallery does not establish live AT parity.
