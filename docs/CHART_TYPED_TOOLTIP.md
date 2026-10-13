# Copied Chart tooltip content

Append `tooltip_build` and `tooltip_build_user` to opt into structured tooltip content. The builder copies title, label, and value bytes synchronously; local formatting buffers can be changed or leave scope immediately after a setter returns. No builder or string pointers escape into owner state.

```c
#include "cui/cui_chart.h"
#include <stdio.h>

static bool tooltip(const cui_chart_tooltip_info *info,
                    cui_chart_tooltip_builder *builder, void *user) {
    (void)user;
    char value[48];
    int written = snprintf(value, sizeof value, "%.0f", info->values[0]);
    if (written < 0 || (size_t)written >= sizeof value)
        return false;
    if (info->has_fallback_title &&
        !cui_chart_tooltip_set_title(builder, info->fallback_title))
        return false;
    cui_chart_tooltip_row row = {
        .has_swatch = true, .swatch = info->series_colors[0],
        .label = info->series_labels[0],
        .value = {value, (size_t)written},
    };
    if (!cui_chart_tooltip_add_row(builder, &row))
        return false;
    return cui_chart_tooltip_add_row(builder, &(cui_chart_tooltip_row){
        .label = {"Total", 5}, .value = {value, (size_t)written},
    });
}
/* options.tooltip_build = tooltip; */
```

Choose **Custom tooltip rows** in the Chart gallery to see a swatched series row
and a plain reporting-period row. The application callback receives real values
and fills its rows with these copying helpers. Configure `options.tooltip_build`
before submitting the Chart; retain any `tooltip_build_user` data through frame
completion. This callback describes paint content and should not perform
application actions or change the UI during prepaint.

The immutable callback info contains Chart kind, retained visual datum/index, at most eight raw values (four Open/High/Low/Close values for candles), default series labels/colors, and optional fallback title. Sankey supplies current retained throughput. Pie supplies raw values rather than formatted share percentages; callers wanting ratios have their caller-owned data/user context. Automatic title/value/label/value-color/pie formatter callbacks are skipped when the builder is selected. The builder supplies the complete title/row content. `tooltip_content` or `tooltip_measure` together with a builder is rejected before mutation; existing custom layout callbacks retain their original behavior when no builder is supplied.

`AUTO` plus the new builder selects current-frame pointer/snapshot painting. Existing zero/no-builder AUTO behavior stays unchanged. Explicit `LEGACY` preserves the legacy manual/event pointer path and stages a local next-state, the copied builder, and the shared measured row layout before layout/semantic output. Bounds tracking targets the persistent original owner state. Current-frame builds once during prepaint and uses immutable current bounds, hover/occlusion state, and fade progress. Legacy builds once during composition. Both rebuild while retained exit content is visible and stop when the hover channel settles.

A row's `has_swatch` is presence, including an alpha-zero color. If any row has a swatch, all labels reserve the same 14-unit slot (8 swatch plus 6 gap); a plain row leaves that slot empty. With only plain rows, labels start at the tooltip padding. Values align to one shared right edge; `has_value_color` selects an explicit color, including transparent, otherwise foreground. Calling `set_title` with an empty string creates a present title row and retains its space. Without a setter, there is no title row. An empty builder renders the existing empty padded box.

Limits are 32 rows, 4096 aggregate copied bytes, and 1024 bytes per string. Replacing a title still consumes copied byte capacity. Strings must be valid UTF-8; NULL data is allowed only for length zero. NULL rows, malformed UTF-8, over-capacity strings/rows, or a failed setter/add-row latch a failure. Ignoring a returned false cannot publish partial content. Callback false also rejects the whole builder.

Legacy builder failure leaves caller state and component layout/semantic output unchanged. A failed current-frame callback rolls back the complete owner paint output and does not commit next-state. Core reports a current-frame callback failure as `CUI_ERROR_LAYOUT`; clear the reported error before retrying corrected input. Writer command-budget and copied-text limits remain bounded. Legacy commits after successful synchronous emissions. Frame requests already generated while stepping appearance/hover, borrowed pixel-buffer writes, callback/measurement host side effects, and later global `end_frame` capacity/layout failures are outside the component's transactional scope; they cannot be restored by this API. Data, callbacks, and user remain caller-owned and must be live through composition/prepaint and renderer completion as required by existing Chart contracts. Callback info/builder are synchronous-only; callbacks must not retain them or perform layout/input/semantic work during prepaint.

This slice uses the existing tooltip box styling: font 11, radius 4, minimum width 150, padding/cursor gap 8, row height 16, title step 18, label/value gap 12, background/foreground/grid border, and muted labels at .7 foreground alpha. It adds no speculative theme/style API and does not claim source popover style parity. Pinned 0.7.1 source uses popover colors/radius, compact rem typography, optional structured/freeform content, unboxed appearance, and an outside ring with Gaussian shadows; those contracts remain separate. Arbitrary custom layout/content in current-frame painting and accepted mobile long-press phase/prevention also remain gaps. This is no full Chart-family or native accessibility certification.

Native coverage checks raw callback values/labels/colors for all seven Chart kinds, copied overwritten stack buffers, plain/mixed/transparent slots, value alignment/color, title-only/empty-present/empty content, exact row/byte limits, invalid UTF-8 and latched failures, formatter precedence, both consumers, owner relocation and persistent bounds, unknown pointer, deferred clipping/ancestor opacity/modal exclusion, and finite retained exit. The standalone Zig 0.16.0 WASM test checks actual Chromium canvas text/raster pixels for both consumers, copied rows, failed-owner rollback, and stationary pointer under controlled owner move/resize. It is a custom proof renderer, not adapter-wide styling certification. GCC ASAN/UBSAN/leaks with the runtime use-after-return option passed; a separate Clang always-use-after-return attempt was unavailable because no Clang executable was installed. The headless test runs through CTest and the portable WASI runner; the standalone pixel check also runs in the website deployment workflow.
