# Numeric and sequence controls

`cui_numeric.h` provides NumberInput, Pagination, Rating and Stepper. Values and changes belong to the application; a true result signals a controlled value/index change. Theme values define typography, surfaces and spacing. The implementation is portable C11 and uses semantic snapshots and common draw commands, without platform or Clay types.

## NumberInput

Create a single-line `cui_edit` attached to an application-owned NUL-terminated buffer, then zero-initialize `cui_number_state` with its borrowed editor. The editor must outlive the state. Drafts are bounded to 127 bytes; a 128-byte buffer accommodates every supported draft. Editor history/scratch allocation remains explicit in its configuration.

`cui_number_input` accepts a finite `double *value`. Initialization and subsequent external value replacements synchronize the draft. Valid typing updates the value immediately; incomplete drafts such as `-` or `.` stay visible without replacing the last accepted value. Default typing admits an optional leading sign, digits and a decimal point; optional exponent typing admits incomplete exponent drafts until they become valid. Full-width digits, signs, decimal point and ideographic full stop normalize to ASCII. Other characters, malformed UTF-8, invalid prospective syntax and oversized insertions are rejected atomically. Existing editor selection, left/right/home/end, shift-selection, backspace/delete and select-all are supported.

Enter submits, and Enter or blur clamps valid drafts to optional finite min/max bounds. Up/Down and plus/minus buttons use fixed positive `step` or an application callback receiving value/direction. Invalid/nonpositive steps do not mutate the value; `step_direction` still reports an attempted step for application-owned handling. Stepping below the minimum downwards or above the maximum upwards does nothing. Crossing a bound clamps in the requested direction; finite overflow without a bound is rejected. Draft formatting failures do not commit a button step. Disabled suppresses editing and focus; readonly keeps readable/focusable text while suppressing editing and stepping. Prefix, suffix and field width are configurable.

Standalone `cui_number_parse` recognizes strict ASCII decimal/exponent grammar without whitespace, hexadecimal, comma, NaN or infinity. It validates before adapting the single decimal point to the current C locale's radix for one correctly rounded `strtod` conversion. `cui_number_format` reverses locale radix adaptation and emits 17 significant digits, preserving finite double round trips, including signed zero and nonzero subnormals. Underflow to zero is rejected. These functions never mutate global locale; concurrent changes of the process locale remain unsupported. Semantic float values/ranges saturate at `FLT_MAX` when the accepted double exceeds semantic storage precision.

Remaining GPUI options include thousands-separator masks, custom pattern validation, fully composed prefix/suffix elements, exact size variants and appearance modes. NumberInput inherits the editor's pending caret/selection painting, pointer text positioning, undo shortcuts, native clipboard and IME gaps. For editing automatically formatted scientific values, enable `allow_exponent`.

## Pagination

Pages are one-based, with zero representing an empty result. Current page clamps to `[1,total]` or zero when no pages exist. Buttons support First, Previous, Next and Last, and visible numbered pages. A compact mode shows the current/total count. Boundaries and the current page disable redundant activation.

The allocation-free item model emits endpoints and a radius-bounded neighborhood, fills single omitted pages, and inserts ellipses for larger gaps. Radius is capped at three, with at most eleven items. Arithmetic remains bounded for `SIZE_MAX` totals. The styled control uses radius two. All page buttons use the ordinary pointer/keyboard button routing; selected semantic details reflect the controlled page. Layout wrapping, configurable label slots, exact GPUI compact typography and arbitrary visible-page budgets remain pending.

## Rating

Rating accepts a controlled floating value, clamps to the configured maximum and quantizes to integer or half steps. The default maximum is five; the bounded renderer caps it at one hundred. Pointer activation selects a star; left/right halves select half/integer values when enabled. Keyboard arrows change by one step and Home/End select zero/maximum. Optional Clear supports both a pointer button and Delete/Backspace. Individual stars use pointer focus with no extra Tab stops; choosing a star returns keyboard focus to the rating group. Disabled and readonly suppress activation; readonly remains identified by semantic details. Full, half and empty indicators use theme-colored text glyphs. Exact upstream SVG-star masks, custom icon/color slots and hover previews remain pending.

## Stepper

Stepper uses a controlled zero-based selected index, bounded to existing items. Each item has title, optional description, explicit pending/active/complete/error status and disabled state. Pointer or Enter/Space activation updates the index. Whole-group disabling suppresses all items. Horizontal and vertical layouts use the same model. Selected semantic details update in the activation frame; status text includes non-color meaning. Empty arrays are supported, and this convenience renderer bounds arrays to one hundred items. Custom icons, connector lines, centered typography and exact size variants remain pending.

Headless tests cover real routed key/pointer events, controlled replacement, draft rejection and CJK normalization, blur/Enter clamping, disabled/readonly behavior, half-star selection and clearing, page navigation, disabled step activation, all four themes, strict finite parsing and extreme double round trips. Exhaustive small pagination windows and `SIZE_MAX` boundaries test overflow/ordering invariants. New sources compile with strict C11 warnings; existing CTest and instrumented ASan/UBSan tests run before submission. Source/test/distribution registration belongs to the integrator.
