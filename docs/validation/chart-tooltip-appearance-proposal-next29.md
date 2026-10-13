# Tooltip appearance API proposal

Historical proposal on baseline f8b7f43, subsequently implemented. The original ring-band/expanded-radius design below was superseded by correction626648e: pinned source paints a filled sigma-zero shadow under the surface with bounds expanded1px and ORIGINAL corner radii. Current behavior and terminal proof are in [the appearance guide](../CHART_TOOLTIP_APPEARANCE.md). Gaussian layers remain separate unfinished work.

```c
typedef enum {
    CUI_CHART_TOOLTIP_BOXED = 0,
    CUI_CHART_TOOLTIP_BARE
} cui_chart_tooltip_appearance;
typedef struct {
    cui_chart_tooltip_appearance appearance;
    float font_size, line_height;
    float padding, min_width, cursor_gap;
    float row_gap, label_value_gap, swatch_gap, swatch_size;
    float radius, swatch_radius;
    cui_color background, foreground, muted_foreground, ring_color;
} cui_chart_tooltip_style;
bool cui_chart_tooltip_style_project(const cui_theme *theme,
                                    float rem_size,
                                    float inherited_line_height,
                                    cui_chart_tooltip_style *out);
/* appended cui_chart_options field: */
const cui_chart_tooltip_style *tooltip_style;
```

NULL style preserves all zero/default legacy commands, spacing, colors and errors exactly. Non-NULL is opt-in and validated before owner mutation/UI registration. No separate mode on options: style.appearance is BOXED zero or BARE. Colors permit transparent alpha; dimensions are explicit, not zero-as-unset. Factory writes output atomically; rem_size and inherited_line_height must be finite positive, theme radius finite nonnegative, bounded projection <=16384; dimension validators reject negative/nonfinite/oversized values and invalid enum. Font/line height must be positive, swatch/radius/gaps/padding/minimum/cursor gap may be zero. Additional measured-content overflow fails before emission/state commit at existing transaction scope.

Factory defaults: font_size=.75*rem_size; supplied inherited_line_height unchanged; padding=.5rem; min_width=150px; cursor_gap=8px (Chart source); row_gap=.25rem; label_value_gap=.75rem; swatch_gap=.375rem; swatch_size=.5rem; radius=theme.radius; swatch_radius=theme.radius/2; colors theme.popover/popover_foreground/muted. CUI theme.muted is the available muted foreground role. Approved extension adds consumed ring_color from Theme.text (source foreground) RGB with alpha.1 overriding inherited alpha. Both boxed consumers emit outside1px band before surface, expanded bounds1px/radius+1/stroke1. Legacy uses Core outline; writer emits equivalent paint-only commands. BARE emits no ring. No shadow fields or Gaussian approximations. Native geometry/order/role/alpha and actual WASM outside/inside clipped-opacity pixels pass; exact native GPUI shader/antialias identity remains unverified.

Line-height proof: pinned gpui-pre0.3.8 styled.rs:545–547 text_xs changes font_size only; source Tooltip does not set line_height. TextStyle.default style.rs:493–494 supplies phi(), geometry.rs:3722–3723 relative1.618034, style.rs:554–555 resolves against current font and rounds pixels. Component Root.prepare root.rs:436 sets rem_size=Theme.font_size, Root.style does not override line height. Thus default compact line height is round(.75rem*1.618034), but an inherited absolute or relative line-height override changes it. Factory requires resolved inherited_line_height; it must not infer measured glyph height from font_size. Host may pass rounded default when no ancestor overrides. Factory comment and guide will state this inherited contract explicitly.

Shared layout uses measured label/value/title widths, explicit line-height for text layout bounds, title semibold, row height=max(line_height, any present swatch_size), title height=line_height; vertical gaps only between existing title/rows (no trailing gap). Empty-present title reserves its line height and following gap. Mixed swatch presence reserves swatch_size+swatch_gap for every row, including alpha0. Rows use right-aligned values and minimum label-value gap. Boxed width=max(min_width,intrinsic rows/title+2padding), height=sum measured line boxes/gaps+2padding, midpoint cursor flip. BARE uses current full plot bounds, no surface/minimum/padding/cursor anchoring; structured rows span full width, start at plot origin, and remain deferred with progress as source. Marks stay local and unchanged. A too-small bare width does not invent wrapping; reject unsupported overflow before emission rather than misrepresent source arbitrary wrapping.

Style pointer is borrowed through synchronous composition/prepaint scheduling and copied by value into the prepaint payload, so later callback never dereferences original pointer. Legacy copies before staging/measurement. Options/state relocation and overwritten original style buffers tested. Both automatic tooltip rows and typed builder consumers receive opt-in style; no-builder/no-style path is left exact. Existing legacy arbitrary tooltip_content uses projected box/bare content dimensions but its host child font cannot be rewritten; caller must use projected font and line height. Current-frame still rejects arbitrary layout callbacks.

Tests: default projection at rem12/16/20 and explicit inherited line heights; source role distinction, radius/swatch radius, atomic invalid factory/style; glyph metrics deliberately different from line-height; empty title, title-only, plain/mixed/transparent rows, exact vertical gaps/right edges, zero gaps, bare full-size placement, boxed midpoint flips; legacy/current shared geometry; copied style relocation; retained fade/opacity/deferred clipping/stationary resize; no-style command compatibility. Real WASM styled boxed/bare text/swatch pixels, no shadow parity claim. External raster/host side effects and later end_frame failure stay outside existing rollback guarantees.

Implementation approved and consumed by both tooltip paths. Measured glyphs are centered in explicit inherited line boxes. Bare oversized single-line content rejects atomically; source wrapping remains a gap. Payload stores style by value and callback rebinds to the copied payload address. Source/legacy boundary and exact validation evidence are in docs/CHART_TOOLTIP_APPEARANCE.md.

Final approved ring extension is included in the copied style value, and overwritten original caller style/state/options tests cover its lifetime. NULL style retains its original border. Final worker full CTest125/126, only stale distribution on f8b7f43; scoped native/GCC ASAN/WASM checks pass.
