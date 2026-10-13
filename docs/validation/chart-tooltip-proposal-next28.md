# Chart tooltip proposal, Next28

Audit base: `cb8def7`, pinned upstream `87d10ae5e1299d1be18670a37570c5402f952e34` (0.7.1). This is a read-only proposal; no runtime behavior or full-family status changes are asserted.

## Exact source contracts

`crates/component/src/chart/mod.rs:289–383` owns `TooltipContent<T>`. Its callbacks are optional title `(&T)->SharedString`, value `(&T,row_index,f64)->SharedString`, value-color `(&T,row_index,f64)->Hsla`, and custom content `(&T,&mut Window,&mut App)->AnyElement`. Custom content wins and neither title nor row callbacks are built. Otherwise the explicit title overrides an optional chart fallback. Missing fallback means no title. Rows arrive in chart order as `(swatch,name,number)`; value formatting and optional value color receive that row index and number. A failed optional numeric row collection suppresses the complete tooltip, not a partially assembled set of rows. Source uses owned reference-counted strings and elements; it does not return stack-borrowed strings.

`crates/component/src/plot/tooltip.rs:269–441` defines each row as optional swatch color, label string, value string, and optional value color. `row` includes a swatch; `plain_row` omits it. `value_color` affects only the latest row and is a no-op before any row. At render, a swatch slot exists in every row if any row has a swatch, including transparent swatches; otherwise no slot exists. A plain row's empty slot aligns its label with series rows. The label is muted; the value uses tooltip foreground unless explicitly colored. Title is semibold. A title (including an empty but present title) or nonempty row list selects structured content and overrides freeform children; otherwise freeform children are used.

`tooltip.rs:530–611` lays out title/rows vertically with gap-1, row content with gap-3 between label cluster and right-aligned value, and gap-1p5 between the size-2 swatch and its label. Swatch radius is half theme radius. All content receives compact `text_xs`. Appearance defaults true, glide true, gap zero for generic Tooltip; Chart owners explicitly choose gap 8 pixels. Boxed content has default minimum width 150 pixels only when a caller has not provided one, padding-2, cursor-side flip on each axis at the plot midpoint, and popover styling. Appearance false uses unboxed full-size relative content. Progress defaults to owner hover progress and explicit override clamps to [0,1]. Crosshairs/dots stay local; only the box is deferred, with its own opacity because deferred content leaves the local opacity scope.

`crates/component/src/styled.rs:18–79,186–195` uses theme popover, popover_foreground, and radius with no ordinary border. Its outside ring is foreground alpha .1, spread 1px, zero blur. Two black alpha .1 shadow layers have offsets (0,4)/(0,2), Gaussian sigma 3/2, and spread -1/-2. These are exact source parameters, not evidence that the current software/browser effect implementation can match them. Spacing utilities and `text_xs` are defined by the separately pinned `gpui-pre=0.3.8`; this audit inspected their call sites but did not independently inspect the dependency definitions. Therefore rem fractions/pixel font size must not be certified from names alone.

`crates/component/src/chart/radar_chart.rs:38–44,455–488,675–676` distinguishes Text from Element dimension labels. Text is painted by the plot text layer, honors label_color, and supplies the tooltip fallback title. Element is measured at its natural size, painted as custom content, does not honor label_color, and caches no fallback title. The label closure runs once per dimension per frame; tooltip reads that cache. Explicit `tooltip_title` still overrides a missing fallback. This requires no new keyboard or legend behavior.

## Actual CUI paths and missing behavior

`cui_chart_options` already supplies title/label/value/value-color/pie-share callbacks and legacy custom tooltip layout/measure callbacks. Native tests already cover formatter/custom-content precedence and empty rows; current-frame tests cover default deferred paint and hover motion. Current-frame rejects custom layout/measure callbacks rather than invoking layout during prepaint. Both tooltip implementations in `src/components/cui_chart.c` use `datum.label` as Radar fallback even when the paired Radar custom-label measure/paint callbacks replace the text label. This is a concrete missing source behavior, visible without adding a new row or theme abstraction.

CUI automatic tooltip rows are limited to existing chart data (up to eight series or four candle values); there is no typed arbitrary plain-row/mixed-row builder. Transparent automatic swatches are currently replaced by foreground. The current box uses background/foreground/grid, fixed radius 4, font 11, and an ordinary border. These differ from source popover composition. Raw Plot paint callbacks allow custom commands, but are not a reusable typed tooltip contract.

## Smallest next implementation slice: optional Radar title

Proposed append-only API:

```c
typedef enum {
    CUI_CHART_RADAR_TOOLTIP_TITLE_LEGACY = 0,
    CUI_CHART_RADAR_TOOLTIP_TITLE_SOURCE_LABEL
} cui_chart_radar_tooltip_title_mode;
/* appended to cui_chart_options */
cui_chart_radar_tooltip_title_mode radar_tooltip_title_mode;
```

Zero preserves existing behavior. SOURCE_LABEL is valid only for Radar, and suppresses only the automatic fallback title when both custom Radar label callbacks are present. Plain Radar text remains the fallback. Explicit tooltip_title retains precedence, including an empty returned string. Legacy tooltip_content retains complete precedence and no title formatter is invoked. Invalid enum or nonzero mode on another kind is rejected before UI/state mutation. No strings are retained in state; reuse the private component helper `static cui_str chart_tooltip_fallback_title(const cui_chart_options *, const cui_chart_datum *)` in both composition and current-frame tooltip paths. Explicit title selection remains outside this helper and has precedence. No new public query is necessary; this metadata does not change raster geometry, so raster cache keys remain unchanged. Existing borrowed formatter lifetimes and callback signatures remain unchanged.

Ownership for this slice: appended Chart header, Chart component source, Base Chart option validation/cache fingerprint if the new policy participates in frame keys, new scoped native test, new standalone WASM browser test, and guide only. No Core, Theme, Plot, Gallery, manifest, global registration, or generated-header changes. Root may enable source policy in its Radar story after proof.

Test plan: reproduce currently visible custom Radar title in deterministic native frames; SOURCE_LABEL removes title while retaining series rows/marks and expected smaller tooltip box. Test default text label, explicit title override, empty explicit title, custom-content callback precedence/exact calls, zero compatibility, both legacy composition and CURRENT_FRAME, retained exit fade, invalid enum/wrong-kind state and output atomicity. Copy options/state to different addresses and render again to establish no new borrowed internal storage. WASM actual canvas/text commands must show row value and absence/presence of the Radar title under stationary hover and explicit override; use frame/phase signals rather than sleeps. Native accessibility announcement is not certified by these checks.

## Subsequent bounded typed-content API, not yet authorized

Use an opaque transient copying builder rather than callback-returned borrowed arrays:

```c
typedef struct cui_chart_tooltip_builder cui_chart_tooltip_builder;
typedef struct {
    bool has_swatch, has_value_color;
    cui_color swatch, value_color;
    cui_str label, value;
} cui_chart_tooltip_row;
typedef struct {
    cui_chart_kind kind;
    size_t index, value_count; /* at most 8, validated before callback */
    const cui_chart_datum *datum;
    double values[8];
    cui_str series_labels[8];
    cui_color series_colors[8];
    bool has_fallback_title;
    cui_str fallback_title;
} cui_chart_tooltip_info;
bool cui_chart_tooltip_set_title(cui_chart_tooltip_builder *, cui_str);
bool cui_chart_tooltip_add_row(cui_chart_tooltip_builder *, const cui_chart_tooltip_row *);
typedef bool (*cui_chart_tooltip_build_fn)(const cui_chart_tooltip_info *,
                                        cui_chart_tooltip_builder *, void *);
/* append tooltip_build and tooltip_build_user to options */
```

Proposed hard limits: 32 rows, 4096 aggregate copied UTF-8 bytes, 1024 bytes per string. Builder copies synchronously inside setter/add-row, including local callback buffers; a failed append latches failure so ignoring its bool cannot publish a partial tooltip. Return false rejects the complete tooltip paint and state commit. Builder/info are valid only during the callback and never escape into owner state. Datum/data/user remain caller-owned and live through the frame's prepaint callbacks. Existing formatters are not called when tooltip_build is selected; it replaces automatic title/rows. Reject simultaneous tooltip_build and legacy tooltip_content/measure before mutation, preserving existing legacy behavior when build is absent. Callback host side effects cannot be rolled back and must be documented.

The builder should assemble and measure all copied rows before writer emission, derive `has_swatches` from boolean presence rather than alpha, right-align values to the measured shared width, and emit local marks plus deferred tooltip text with immutable snapshot bounds/progress. It must preserve Core writer command/text-copy budgets and request-frame rules; no layout, semantics, event consumption, focus, or Core invalidation in prepaint. The same pure row layout can feed legacy composition; legacy host layout callbacks remain confined to their existing path.

Style parameters should be a separate value-only projection: popover background/text, muted text, radius, compact font size/line height, padding, vertical gap, label/value gap, swatch size/gap/radius, minimum width, cursor gap, and appearance boolean. Explicit validated overrides need presence flags rather than treating alpha-zero as unset. Exact source compact font/rem utility definitions must be verified before default projection is claimed. Exact Gaussian shadow requires a separate proven Core/adapter contract; do not emulate it with a border and claim parity. Typed-content composition can be delivered first with honest style limits, or source style can follow once dependency/effect evidence exists.

Typed tests must cover plain-only rows (no slot), mixed rows (reserved empty slot), transparent swatch presence, right-aligned values, title-only and empty-present title, copied stack buffers and owner relocation under ASAN use-after-return, capacity/invalid atomicity, callback exact-once/precedence, resize/current snapshot geometry, deferred clipping/opacity/modal behavior, and bounded settling. This is a future proposal, not an unused helper implementation.
