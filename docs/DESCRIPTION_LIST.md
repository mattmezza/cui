# Description List

`cui_description_list` composes borrowed label/value strings or custom callbacks
into horizontal or vertical rows. Items have stable nonzero IDs; duplicate IDs
and invalid dimensions reject the call before it submits layout or semantics.
There is no persistent allocation or interaction state.

The frozen GPUI Kit 0.7.1 `description_list.rs` supplies the defaults: three
columns, horizontal bordered cells, a 120px label, medium 8px/4px padding, 14px
labels at the default 16px text size, and full-width 8px separators. Small and
extra-small use 4px/2px padding; large uses 12px/6px. Unbordered lists remove cell
padding and use size-dependent spacing between rows. Label backgrounds and
foregrounds are independent theme tokens.

The headless row iterator groups spans into 1–10 columns. Zero columns select
three; zero spans normalize to one. Separators occupy isolated rows. Oversized
spans retain proportional shrink, without the upstream empty leading row edge
case. In an incomplete row, CSS flex free space is shared equally: spans 1 and 2
in four columns receive 37.5% and 62.5%, respectively. Percentage box sizing
resolves these widths in the same frame, including after a parent resize.

Set `custom_label_width` to use `label_width` (including zero), and additionally
`label_width_percent` to interpret it as a fraction of the cell. Vertical mode
ignores label width. Callbacks receive a theme copy with the appropriate text
size and color; they own their semantics. Plain content exposes passive labels
inside a group and adds no Tab stops.

`tests/unit/test_description.c` covers grouping, oversized spans, separators,
first-frame and resized fractions, all four themes/sizes/layouts/border modes,
custom content, wrapped row height, percentage labels, and invalid IDs.

Remaining parity limits: CUI currently clips the outer list to a rectangle;
rounded clip masks and independent per-cell paint clipping are not implemented.
Custom overflowing children can cross a cell boundary before reaching the outer
clip. Rich Text values use host callbacks until the complete Text component is
available. The source's inherited font weight and application-level typography
are not yet represented by this API. No browser/native visual parity claim is
made from these headless tests.
