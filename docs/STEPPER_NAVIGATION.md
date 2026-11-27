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

This entry uses fixed spacers. The Fill presenter and its retained bindings are
described below. Centered separators, arbitrary child content and exact source
density remain open. Native software checks cover the fixed layout and actual
pointer, touch and keyboard requests. The WASM gallery tests also cover desktop
at DPR 1 and 2 and a touch viewport at 390 px and DPR 2. Physical
assistive-technology acceptance remains open.

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

The Stepper gallery defaults to **Fill available space** and **Auto** orientation.
It lays out steps horizontally at the shared SM breakpoint and vertically
below it. Larger design text increases the width needed for horizontal layout.
Auto uses the current preview content width, so resizing keeps the active step
and changes only layout. Choose Vertical or Horizontal to set the axis
explicitly. Choose **Wrap labels** for equal horizontal lanes and natural-height
vertical rows. Choose Fixed connectors for fixed spacers or Buttons for the
original rendering. Connector and icon settings appear only in the visual
layouts. The design scale starts at Inherit. Leaving the story resets the
controls and keeps the application-owned selection and validation.

The **Details confirmed** checkbox gates later steps. Rejected requests and
requests for the active step both count. A rejected request leaves the active
step unchanged. Disabling the story suppresses interaction and preserves progress.
Each displayed C snippet uses the selected API and includes the validation
callback. In Auto mode, pass your content width each frame. In vertical Fill
mode, also pass the logical height you want to allocate for the steps. Custom
icons use static RGBA bytes kept alive through renderer completion.

Headless Chromium exercises Fill at desktop DPR1/2 and mobile DPR2. The checks
cover reflow on resize and larger type, actual connector and hover/press pixels,
request rejection, keyboard and touch selection, the fixed-layout variants and
route reset. The wrapped layout keeps all three steps reachable in a horizontal
row on the touch viewport. The checks compile the displayed snippets with strict
C11 warnings. Choose **Centered labels** to see the connectors cross lane
boundaries between markers. The preview exercises both axes and includes copyable C.
Arbitrary child content and native assistive-technology acceptance remain open.

## Foreign builder visual authoring

Python, JavaScript, TypeScript, Go and Rust offer fixed connectors, Fill,
Wrapped Fill and Centered Fill. Retained C uses
`cui_bridge_set_stepper_centered_fill_v1` for centered labels and
`cui_bridge_set_stepper_wrapped_fill_v1` for leading-aligned labels. Both
accept the same options as `cui_bridge_set_stepper_fill`. Probe each
capability independently when deploying to earlier providers. In Python, for
example, choose a layout before mounting the tree:

```python
if app.supports_stepper_centered_fill():
    steps.stepper_centered_fill(vertical=False, width=0)
elif app.supports_stepper_wrapped_fill():
    steps.stepper_wrapped_fill(vertical=False, width=0)
elif app.supports_stepper_fill():
    steps.stepper_fill(vertical=True)
else:
    steps.stepper_visual(True, True, "md")
```

See [binding builders](BINDING_BUILDERS.md#stepper-layouts).

These settings keep the bridge's selected Item and aggregate CHANGE semantics.
Icons are copied from owned RGBA bytes, and groups allow up to 64 Items.
Examples are
[`stepper_visual.py`](../bindings/python/examples/stepper_visual.py),
[`stepper_visual.cjs`](../bindings/node/examples/stepper_visual.cjs),
[`stepper_visual.ts`](../bindings/typescript/examples/stepper_visual.ts),
[Go](../bindings/go/examples/stepper_visual/main.go) and
[Rust](../bindings/rust/examples/stepper_visual.rs).

## Opt-in fill and indicator interaction

Include `cui/cui_stepper_presentation.h` and call `cui_stepper_present` for
non-centered fill layout and source-like indicator feedback. Existing navigation
and visual APIs retain their pixels and ABI. For example, inside a growing parent:

```c
cui_stepper_navigation_result result = cui_stepper_present(
    ui, id, theme, steps, count, selected,
    (cui_stepper_visual_options){.design_size = CUI_DESIGN_LG},
    (cui_stepper_presentation_options){.layout = CUI_STEPPER_LAYOUT_FILL},
    handler);
```

Width0 grows to parent available width; a positive width fixes that extent.
Height0 stays intrinsic, including vertical layout; a positive height supplies an
explicit vertical extent. The first non-last items grow; the last stays intrinsic.
Horizontal fill centers unequal-height items on the root cross axis. Trigger
buttons themselves stay intrinsic. Connectors are decorative zero-flow
underlays inside their own items, clamped empty when space is insufficient.
`CUI_STEPPER_LAYOUT_CONTENT` retains fixed spacer geometry and requires zero extents.
The intrinsic label layouts can overflow a narrow container. Use
`CUI_STEPPER_LAYOUT_WRAPPED_FILL` when labels need to reflow. It divides the
available width equally among horizontal steps, aligns indicators and connectors
at the top, and wraps titles and descriptions. In a vertical Stepper, labels use
the width alongside their indicator. Keep height at zero for rows to grow with
their content, and pick the vertical orientation when the available width cannot
comfortably hold every lane. The mode accepts valid UTF-8 and uses CUI's paragraph
wrapping. Supply enough width for at least one grapheme in each text column.

Use `CUI_STEPPER_LAYOUT_CENTERED_FILL` with the same sizing rules to center
horizontal captions and markers within each lane. In a vertical Stepper, text
wraps beside the marker and the marker centers vertically against its row.
Connectors span the current marker edges across lane boundaries. Keep the height
at zero for content-sized vertical rows or pass a height to distribute the rows
over a region. Connectors leave a gap at each marker and disappear if no space
remains between them. They draw beneath the markers and captions. Hit regions
belong to the steps. Resizing updates the markers, wrap and connectors together.

All shared XS..9XL design sizes remain available. Density selects indicator glyph
and proportions. Arbitrary child styling remains open.

Calling the new entry opts into secondary/secondary_hover/secondary_foreground
unchecked indicators and theme-radius behavior (zero radius makes square
indicators). The pressed default is CUI's secondary **button** active color;
GPUI's global secondary_active can differ. Set `custom_palette=true` and all four
palette fields to supply an exact application palette. Checked primary/on_primary
wins even when disabled. No focus ring is added. Exact source typography and
implicit engine active-state behavior remain partial.

Only the indicator's currently visible, clipped, unoccluded area shows hover.
Pressed feedback observes accepted trigger capture: pointer capture while visibly
over the indicator, or Space capture regardless of pointer position. Label-origin
press dragged onto the indicator follows that CUI policy too; this does not claim
GPUI drag/keyboard equivalence. Disabled/inert/occluded indicators suppress pointer
feedback; decoration creates no new focusable targets or activation callbacks.
Existing request ordering and owner-validation rules still apply, exactly once.
Callback owner updates affect the next submission; selection/configuration used
for painting are captured by value. Do not mutate borrowed steps/theme/icon arrays
inside callbacks. Arrays are borrowed through return, bitmap bytes through renderer
completion. The retained Fill setters reuse the same layout and theme feedback.
The immediate C presenter also accepts the typed custom palette and controlled
request handler.
