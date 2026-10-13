# Remaining Chart source audit, next27

Read-only production audit against GPUI Kit 0.7.1 commit
`87d10ae5e1299d1be18670a37570c5402f952e34`. CUI inspection is commit
`12ad7ef7b75f33fb7be2a55e337f4c9fd4d8b114`, including current-frame migration.
Root reports that migration integrated in next26 with 123 native checks; this
worker does not substitute that report for an independently rerun platform matrix.
No production/header/global file was edited in this audit.

## Concrete source contracts and remaining gaps

| Area | Pinned source | Actual CUI path | Assessment |
| --- | --- | --- | --- |
| Point-chart x-axis gutter | component/chart/mod.rs:217–221 derives 18px from label TEXT_SIZE10 and TEXT_GAP2; line/area scales, hover exclusion and crosshair use that gutter | Base and styled Chart use 20px; labels begin at height-14 | Concrete shared geometry mismatch. Source End labels start axis+6, hence height-12 for18px gutter. Must fix raster, coordinates, hit, grid, crosshair and label layout together. |
| Point count smaller than data | component/chart/mod.rs:135–140 takes max(requested,data_len) | Base Chart validators reject nonzero point_count<count | Concrete normalization mismatch; bounded CUI maximum is a separate limit. |
| Tick defaults/label counts | component/chart/mod.rs:249–285: near-integral tolerance .001, otherwise one decimal; explicit counts distribute first-to-last, zero hides, absent uses stride | cui_chart_tick_number, x-label visibility and tests cover these behaviors | Substantive existing implementation; do not mark whole axis family missing. Generic PlotAxis and styled Chart are separate contracts. |
| Default tooltip box/theme | plot/tooltip.rs:578–605 and styled.rs:186–194 use popover, popover_foreground, Theme radius and popover shadow; min width150, padding2, compact text | Both Chart paths use chart background/foreground, chart grid as ordinary border, radius4 and fixed11px | Min width/flip and Chart gap8 exist. Theme surface, compact rem typography, true muted label and source ring/shadow are missing. Default Root rem is Theme font_size16 (root.rs:436; theme/mod.rs:771); CUI hard-coded11 bypasses rem scaling. The external GPUI text_xs numeric mapping was not independently inspected here. |
| Tooltip structured content | plot/tooltip.rs:300–445,518–562 exposes title, colored/plain rows, last-row value color; any swatch reserves blank slots for plain rows; structured content wins over freeform; appearance/progress/glide/gap builders | Chart exposes format/title/label/value/color hooks, bounded automatic series rows and legacy layout custom content; Plot exposes generic paint callback | No portable styled typed row/title parts, plain-row append, swatch omission/alignment, bare appearance or explicit tooltip progress/glide controls. Arbitrary raw writer callbacks do not establish these styled contracts. |
| Tooltip custom layout in current frame | Source PlotElement legally lays/prepaints tooltip after current geometry | CURRENT_FRAME rejects tooltip_content/tooltip_measure; legacy supports them | Documented genuine boundary. A paint writer cannot solve arbitrary layout/semantics after layout; do not claim source-equivalent custom layout from the new paint seam. |
| Hover dot positioning/style | plot/tooltip.rs:250–276 uses offset=size/2-border/2 (8px chart dot =>3.5px), one-pixel stroke; halo20*progress | CUI dots use point-4; halo/radius/one-pixel border exist | Half-pixel positioning mismatch remains; actual GPU raster equivalence is unverified. |
| Crosshair color | plot/tooltip.rs:139–142 mixes Theme border with foreground at .8; band uses foreground opacity.08 | CUI dashed crosshair uses chart grid; bands use foreground*.08 | Band intent implemented, dashed hairline theme projection differs. Exact Hsla/color conversion requires separate explicit evidence. |
| Radar element-label title | radar_chart.rs:440–488 records None for element labels; tooltip:676 uses that record | Custom label measure/paint exists, but default tooltip title still uses datum.label | Concrete default title precedence mismatch: element label should not implicitly produce its text datum title. |
| Static label appearance in current-frame mode | PlotElement invokes appear before paint; Pie/Sankey label paint consumes that frame's appearance | CURRENT_FRAME composes ordinary labels from copied prior rendered_reveal_progress before end-frame callback steps appearance | Code-path indication of prior-frame label fade; no new phase/pixel reproduction in this audit. Custom layout labels cannot simply move into writer callbacks. |
| Legend | Component Chart classes have no legend API; story/chart_story.rs:262–417 composes wrapping swatch legends outside the chart | CUI hide_legend/extra-series corner text is an extension | Do not invent a source Chart legend deficit. Story-card wrapping/swatch demonstration is a separate application composition task. |
| Keyboard and semantics | PlotElement source registers MouseMove/LongPress handlers, no keyboard/focus/accessibility datum model | CUI named IMAGE/no-focus semantic has headless evidence; no datum keyboard navigation | Keyboard datum browsing is a possible CUI accessibility extension, not a missing pinned Chart behavior. Native accessibility remains unverified; source absence is not certification. |
| Mobile touch | PlotElement:169–197: accepted LongPress Started requires hovered and not default-prevented, prevents default; Moved follows; End/Cancel clears | Base cursor_long_press and legacy Plot supplied-touch are tested; styled Chart has no long-press input policy; current Plot rejects touch | Required source behavior remains partial. Core event metadata can identify pointer type, but immutable prepaint snapshot lacks pointer kind/long-press acceptance/prevention. No OS detector/native mobile proof. |

Source shadow is precise: styled.rs:59–79 uses an outside1px foreground-alpha.1
ring plus Gaussian layers at offsets4/2, sigma3/2, spreads-1/-2. CUI effects offers
separable box blur, not that box-shadow contract. An ordinary border is not proof
of this appearance. This audit does not propose silently replacing it with one.

## Proposed next bounded vertical slice

Implement explicit source **Line/Area point-axis layout**, preserving zero/default
legacy behavior. Proposed append-only public enum policy is `LEGACY=0` and
`SOURCE_POINT`; selected on Chart options. Source mode derives18px gutter and uses
max(point_count,count), while legacy keeps20px and its existing rejection of a
smaller point_count. Restrict source mode to Line/Area initially; validate enum,
kind, limits and geometry atomically. No arbitrary label-font API is implied.

One Base helper must compute effective slots/gutter for raster scales, point
coordinates, domain/grid/ticks, hit exclusion and frame cache keys. Styled labels,
measured y gutter and local/deferred hover decorations consume the same geometry.
Explicit legacy hide_axes/hide_x_axis and manual pointer contracts remain intact.
This completes a coherent axis slice rather than changing only a paint constant.

Proposed ownership: include/cui/cui_chart.h, src/base/cui_chart.c,
src/components/cui_chart.c, new scoped unit/browser tests and new guide. Root owns
registration, distribution and host-gallery opt-in. No Core/Theme/Motion/global
edits. Headless tests should prove18/20 baselines, source label top/end alignment,
point-count normalization/data growth, pinned-domain/outside-gutter clipping,
stationary-pointer resolution at the18px boundary, invalid-output/state atomicity
and unchanged legacy outputs. Standalone actual WASM should verify pixels and
phase/geometry signals after resize/scroll; no fixed sleeps. Do not promote the
whole family after this slice.

A later typed tooltip slice should project existing Theme tokens by value and
support caller-bounded rows including plain slots. It requires separate approval
for typography/lifetime/output limits and honest shadow/custom-layout boundaries.
A mobile slice requires a shared immutable accepted-long-press contract or an
explicit host-supplied phase path; it cannot infer OS gesture acceptance from raw
pointer position in this paint-only owner.

## Evidence boundary

Inspected pinned files, CUI public headers/components/Base, and existing tests.
Existing test_chart covers tick geometry, measured gutters, formatter/custom
content precedence, named IMAGE semantics, hover fade and specialized families.
test_plot_styled and test_plot cover host-driven long-press phases.
test_plot_chart_prepaint covers current pointer geometry/occlusion and motion.
No new runtime experiment or native mobile/a11y test was performed for this audit.
