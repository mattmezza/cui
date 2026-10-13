# Radar custom-label tooltip title

Opt into `CUI_CHART_RADAR_TOOLTIP_TITLE_SOURCE_LABEL` through the appended `cui_chart_options.radar_tooltip_title_mode` field. This follows the pinned GPUI component 0.7.1 `RadarLabel::Element` rule: paired custom dimension label measure/paint callbacks supply no automatic tooltip title. The series rows and hover marks still render. Plain text labels retain their datum-label fallback title.

```c
options.kind = CUI_CHART_RADAR;
options.radar_tooltip_title_mode = CUI_CHART_RADAR_TOOLTIP_TITLE_SOURCE_LABEL;
options.radar_label_measure = measure_dimension;
options.radar_label_paint = paint_dimension;
```

Zero is `CUI_CHART_RADAR_TOOLTIP_TITLE_LEGACY` and preserves the existing fallback. The source policy affects only Radar and only when both custom label callbacks are set. An explicit `tooltip_title` callback overrides the fallback, including an empty result that suppresses visible title text under the existing CUI callback contract. Legacy `tooltip_content` retains complete precedence and skips automatic title/row formatter construction. `CURRENT_FRAME` continues to reject legacy layout/measure tooltip callbacks; this option does not permit layout during prepaint.

The component resolves the same fallback in legacy composition and current-frame deferred tooltip painting, including retained exit fade. Strings and callback users follow existing caller-owned lifetimes through the frame; no new title data is retained. Invalid enum values and a nonzero policy on another Chart kind are rejected before styled UI/state mutation or Base raster mutation. The option changes tooltip metadata, not raster geometry, and does not add cache-key terms.

This is a bounded title fix. It does not provide typed arbitrary tooltip rows, reproduce popover Gaussian shadows, add a keyboard/legend contract, or certify native accessibility announcements. Source `Some(empty_title)` layout presence versus CUI's existing empty string suppression remains an independent typed-content contract.

The native test covers legacy/current-frame paths, paired/partial/plain labels, explicit and empty overrides, legacy custom-content precedence/exact callback counts, exit retention/settling, copied options/state, invalid atomicity, and unchanged raster-cache keys. It runs through CTest and the portable WASI runner. The standalone browser check uses Zig 0.16.0 WASM, current-frame immutable pointer snapshots, and actual Chromium canvas text/raster pixels for stationary hover across legacy/source/explicit title toggles; it also runs in the website deployment workflow.
