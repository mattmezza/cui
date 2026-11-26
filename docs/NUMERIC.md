# Numeric and sequence controls

`cui_numeric.h` provides NumberInput, Pagination, Rating and Stepper. Values and changes belong to the application; a true result signals a controlled value/index change. Theme values define typography, surfaces and spacing. Each styled function calls its corresponding `cui_base_*` reducer exactly once, then paints without adding semantics or mutating state. Base reducers are independently usable with custom layouts and emit no draw commands. `cui_numeric_part_id` exposes stable child IDs for these layouts. Button/page/star/step activations are processed in input order, including multiple activations in one frame. The implementation is portable C11 and uses semantic snapshots and common draw commands, without platform or Clay types.

## NumberInput

Create a single-line `cui_edit` attached to an application-owned NUL-terminated buffer, then zero-initialize `cui_number_state` with its borrowed editor. The editor must outlive the state. Drafts are bounded to 127 bytes; a 128-byte buffer accommodates every supported draft. Editor history/scratch allocation remains explicit in its configuration.

`cui_number_input` accepts a finite `double *value`. Initialization and subsequent external value replacements synchronize the draft. Valid typing updates the value immediately; incomplete drafts such as `-` or `.` stay visible without replacing the last accepted value. Default typing admits an optional leading sign, digits and a decimal point; optional exponent typing admits incomplete exponent drafts until they become valid. Full-width digits, signs, decimal point and ideographic full stop normalize to ASCII. Other characters, malformed UTF-8, invalid prospective syntax and oversized insertions are rejected atomically. Existing editor selection, left/right/home/end, shift-selection, backspace/delete and select-all are supported.

Enter submits, and Enter or blur clamps valid drafts to optional finite min/max bounds. Up/Down and plus/minus buttons use fixed positive `step` or an application callback receiving value/direction. Invalid/nonpositive steps do not mutate the value; `step_direction` still reports an attempted step for application-owned handling. Stepping below the minimum downwards or above the maximum upwards does nothing. Crossing a bound clamps in the requested direction; finite overflow without a bound is rejected. Draft formatting failures do not commit a button step. Disabled suppresses editing and focus; readonly keeps readable/focusable text while suppressing editing and stepping. Prefix, suffix and field width are configurable.

Standalone `cui_number_parse` recognizes strict ASCII decimal/exponent grammar without whitespace, hexadecimal, comma, NaN or infinity. It validates before adapting the single decimal point to the current C locale's radix for one correctly rounded `strtod` conversion. `cui_number_format` reverses locale radix adaptation and emits 17 significant digits, preserving finite double round trips, including signed zero and nonzero subnormals. Underflow to zero is rejected. These functions never mutate global locale; concurrent changes of the process locale remain unsupported. Semantic float values/ranges saturate at `FLT_MAX` when the accepted double exceeds semantic storage precision.

NumberInput renders the decrement button, centered editable value and increment button inside a single themed frame. Its `name` is an accessible label rather than a painted legend. Source Minus/Plus SVGs use the theme asset provider; no textual +/- stand-ins are painted. Step buttons remain pointer-only and route pointer focus to the editor, with accessible Increment/Decrement names. They stay enabled at configured bounds; an attempted step reports its direction while the clamped value stays unchanged. Arrow-key stepping still belongs to the focused editor. Step buttons have only the outer corners rounded, one pixel inside the frame radius, and fill the frame's inner height. Normal is transparent, hover uses input at40% alpha and held uses60%; glyphs use secondary foreground. Disabled dims the whole frame to50%, with the source disabled input/border tokens inside. `no_appearance` suppresses the frame surface/border. The renderer shares Input's selection and caret painting, with retained horizontal scrolling.

`cui_number_options.size` defaults to `CUI_NUMBER_MEDIUM` (zero); SMALL, LARGE, XSMALL and CUSTOM are available. `cui_number_measure` exposes the complete metrics. Preset metrics scale by positive `density` or `theme.font_size / 16`: XS/S/M/L heights20/24/32/44, fonts12/14/14/16 and line box20. Step widths24/24/32/32 and SVG sizes12/14/16/24 follow the frozen source. Custom size is the frame height, step width and icon size, with font87.5% of that size. The frame width defaults200; negative width grows. Fixed-width values center correctly on the first frame. The inner field uses Source Input padding without another border or surface.

Remaining GPUI options include thousands-separator masks, custom pattern validation and fully composed prefix/suffix elements. NumberInput still has pointer text positioning, undo shortcuts, native clipboard and IME gaps in its base reducer. Its semantic field is currently Textbox rather than a native SpinButton role; there is no outer focus shadow or caret blinking. For editing automatically formatted scientific values, enable `allow_exponent`.

## Pagination

Pages are one-based, with zero representing an empty result. Current page clamps to `[1,total]` or zero when no pages exist. Buttons support First, Previous, Next and Last, and visible numbered pages. A compact mode shows the current/total count. Boundaries and the current page disable redundant activation.

The original `cui_pagination` uses the theme's sizes in a single row. Its allocation-free numbered model emits endpoints, a radius-two neighborhood and ellipses, filling single omitted pages. The reusable `cui_pagination_items` model supports radius zero through three and at most eleven items. All controls retain the application's `size_t page` and use the ordinary pointer, keyboard and accessibility button routing.

Set `.visible_pages = 10` in `cui_pagination_options` to show ten numbered buttons, including the first and last pages. The contiguous middle window shifts near each end so it can use the whole budget. Positive budgets clamp to 5 through 32, and up to two extra ellipses label the gaps. Zero selects the existing radius-two window. Use `cui_pagination_visible_items` with `CUI_PAGINATION_MAX_ITEMS` caller slots when painting a custom view. The window arithmetic supports totals through `SIZE_MAX`.

Set `.icon_only = true` to show just Previous and Next with SVG chevrons. Both buttons keep their accessible labels and boundary disabling. This takes priority over `.compact`, the numbered budget and `.navigation`. Set `.navigation = CUI_PAGINATION_NAV_CHEVRONS` to keep the visible Previous and Next labels and add inline chevrons. This layout omits the separate First and Last controls and works with both numbered and counter layouts. The gallery offers **Page counter**, **Numbered pages**, **Arrows only** and **Numbered with chevrons**.

Use `cui_pagination_ex` with `cui_pagination_options` for the shared XS through 9XL sizing and single-line labels. BASE and MD are equivalent. INHERIT uses theme metrics. A positive `available_width` wraps between whole items in navigation order. Supply the current container's inner width in logical pixels each frame. Zero keeps one row. An item wider than the available width keeps its natural width, so provide a sufficiently wide or horizontally scrollable parent for extreme sizes and long page counts. The counter aligns with the controls in its row.

```c
#include <cui/cui_numeric.h>

/* Call in a frame. The application retains page and loads data on a true result. */
bool draw_results_pages(cui_context *ui, cui_id id, const cui_theme *theme,
                        size_t *page, size_t total, float content_width) {
    return cui_pagination_ex(ui, id, theme, page, total,
        (cui_pagination_options){.compact = true, .design_size = CUI_DESIGN_BASE,
                                 .available_width = content_width});
}
```

Use `cui_pagination_events_ex` for the synchronous aggregate change observer and `cui_base_pagination_ex` for the Base behavior in custom layouts. Numbered actions use the window computed from the normalized entry page and the options supplied in that submission.

### Jump to an omitted page

In Styled numbered mode, pass the current overlay viewport in absolute logical pixels through `.ellipsis_viewport`. The visible ellipses become **Earlier pages** and **Later pages** buttons. Each opens a scrolling menu of up to 100 omitted pages nearest the visible window. The pager keeps one menu open at a time and allocates its bounded state on the first open through the context's existing resource budget. The state is collected after omission. Base-only layouts and zero-sized viewports leave ellipses passive.

```c
#include <cui/cui_numeric.h>

bool draw_numbered_results(cui_context *ui, cui_id id, const cui_theme *theme,
                            size_t *page, size_t total, float content_width,
                            cui_rect viewport) {
    return cui_pagination_ex(ui, id, theme, page, total,
        (cui_pagination_options){.visible_pages = 5, .design_size = CUI_DESIGN_BASE,
                                 .available_width = content_width,
                                 .ellipsis_viewport = viewport});
}
```

Opening, scrolling and dismissing the menu leave the page unchanged. Select a row by pointer, touch, keyboard or accessibility activation to commit in that frame. Escape or a pointer press outside dismisses the menu. Focus returns to the ellipsis when it is still present, or to the corresponding Previous or Next control after a jump removes it. Use a stable pager ID across frames. Changing the owner page, total or budget closes the existing menu and rejects its queued row actions. The [retained C bridge configuration](BRIDGE_COMPONENTS.md#configure-a-retained-pager-from-c) exposes layout and labels with copied storage. [Python](BINDING_BUILDERS.md#configure-pagination-from-python) and [JS/TS](BINDING_BUILDERS.md#configure-pagination-from-javascript-and-typescript) expose these options. Go and Rust currently provide the numbered single-row presentation.

### Translate the navigation

Pass a `cui_pagination_labels` through `.labels` to name the group, navigation buttons and ellipsis menus in your application's language. Each empty field falls back to its English default. Supply single-line UTF-8 spans and keep both the descriptor and spans valid and unchanged through the call, including its synchronous callback. CUI copies the submitted text and accessible names into the frame. No application strings are held by persistent menu state. Icons keep these accessible names even when no caption is visible. The Base API uses the same labels for group and button semantics in custom layouts. Numbered pages keep their decimal labels.

```c
static const cui_pagination_labels french_pages = {
    .group = CUI_STR("Pages de résultats"),
    .previous = CUI_STR("Précédente"),
    .next = CUI_STR("Suivante"),
    .earlier_pages = CUI_STR("Pages précédentes"),
    .later_pages = CUI_STR("Pages suivantes"),
};

/* Use these options inside the UI frame with the current available space. */
cui_pagination_options options = {
    .design_size = CUI_DESIGN_BASE,
    .available_width = content_width,
    .visible_pages = 5,
    .ellipsis_viewport = viewport,
    .navigation = CUI_PAGINATION_NAV_CHEVRONS,
    .labels = &french_pages,
};
cui_pagination_ex(ui, id, theme, &page, total, options);
```

## Rating

Rating accepts a controlled floating value, clamps to the configured maximum and quantizes to integer or half steps. The default maximum is five; the bounded renderer caps it at one hundred. Pointer activation selects a star; left/right halves select half/integer values when enabled. Keyboard arrows change by one step and Home/End select zero/maximum. Optional Clear supports both a pointer button and Delete/Backspace. Individual stars use pointer focus with no extra Tab stops; choosing a star returns keyboard focus to the rating group. Disabled and readonly suppress activation; readonly remains identified by semantic details. Full, half and empty indicators use antialiased SVG star masks, tinted by the theme. Half values crop the filled star mask. Custom icon/color slots and hover previews remain pending.

## Stepper

Stepper uses a controlled zero-based selected index, bounded to existing items. Each item has title, optional description, explicit pending/active/complete/error status and disabled state. Pointer or Enter/Space activation updates the index. Whole-group disabling suppresses all items. Horizontal and vertical layouts use the same model. Selected semantic details and the visible Current label update in the activation frame. The selected button uses the primary variant; other buttons use the outline variant. Explicit complete/error statuses remain visible on other steps; an inactive active status displays Pending. Status text includes non-color meaning. Empty arrays are supported, and this convenience renderer bounds arrays to one hundred items. Custom icons, connector lines and exact size variants remain pending.

Headless tests cover real routed key/pointer events, controlled replacement, draft rejection and CJK normalization, blur/Enter clamping, disabled/readonly behavior, half-star selection and clearing, page navigation, disabled step activation, all four themes, strict finite parsing and extreme double round trips. Exhaustive small pagination windows and `SIZE_MAX` boundaries test overflow/ordering invariants. New sources compile with strict C11 warnings; existing CTest and instrumented ASan/UBSan tests run before submission. Source/test/distribution registration belongs to the integrator.

Compile `tests/unit/test_numeric.c` with `-DCUI_NUMERIC_BASE_ONLY` to run the same routed-event assertions against base reducers, bare geometry, and no theme/components link. The worker validates this separate executable as well as the styled path, including a Last-then-Previous activation sequence in one frame.

For per-activation requests with application-owned validation, use the additive [controlled Stepper navigation API](STEPPER_NAVIGATION.md). It preserves the legacy mutating reducers and aggregate change observers; the guide provides a compiling owner-validation example.
