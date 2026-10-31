# Explicit Checkbox and Radio indicator IDs

The optional `_with_parts` functions name an existing indicator paint box without
adding an input target. Existing Checkbox/Radio APIs keep their anonymous
indicators and row IDs. Their public options/state structures are unchanged.

```c
cui_selection_indicator_parts parts = {.indicator = indicator_id};
(void)cui_checkbox_ex_with_parts(ui, row_id, theme, cui_string("Receive updates"),
    &check_state, (cui_checkbox_options){0}, parts);
/* After successful composition, target the square instead of the label row. */
bool styled = cui_element_set_shadow_preset(ui, parts.indicator,
    CUI_SHADOW_SM, (cui_corner_radii){3, 3, 3, 3});
```

Keep stable, nonzero and distinct row/indicator IDs. Explicit descriptor0 is
invalid; it does not select legacy anonymous rendering. Descriptors are values
captured before callbacks. The indicator retains the existing projected size,
style, glyph/custom-content callback, clipping and motion behavior. It owns no
new semantic record, focus stop or pointer target. Disabled/checked/mixed state
and source pointer-focus policy remain the chosen facade's existing contract.

Checkbox offers named variants of ex, source, bool convenience and immutable
events. Radio offers named ex and convenience variants; Checkbox-only fields in
its aliased options remain ignored. Each reaches its original reducer once.
The event result keeps its original request/activation counts and callback timing;
calling both an old and a new facade for the same owner would submit it twice.

RadioGroup has separate `_with_parts` and `_events_with_parts` entry points.
Supply one descriptor per item, count equal to item count and at most64. Use
stable IDs associated with item identity when reordering. Group/item/indicator
full IDs must all be distinct. A zero-count group accepts NULL parts/count0 under
its existing empty-group rules. The group copies all part IDs before its owner
request and observer callbacks. Existing item-array borrowing and callback
mutation restrictions still apply; the copied parts do not make item strings or
other borrowed model storage owned. Group painting still uses one group reducer,
not individual Radio reducers for every indicator.

Local invalid descriptors/cardinality/full-ID duplicates reject before reduction,
callbacks, semantics or layout, report INVALID_ACTION and leave model values
unchanged. Bool returns retain their existing **changed** meaning; they are not
validity or completed-rendering results. Event/group result.valid describes
argument validation, not renderer capacity or final layout.

There is deliberately no global identity reservation. A collision with another
already submitted box, an internal conditional part, or a different full ID with
the same folded Core identity is diagnosed during normal box submission. Base
reduction and application callbacks may already have occurred. Such frame failure
**does not roll back input or owner state**. Capacity/layout failures keep that
same ordinary composition boundary. Check frame diagnostics, allocate unique IDs
and do not infer collision safety from previous bounds or semantic lookup.

Style callbacks remain confined to refinement. Indicator/content callbacks may
compose balanced children under their existing contract; do not reuse owner/part
IDs, recursively submit the same owner, free active model/context/resources or
mutate borrowed item/theme/string storage. Changing external descriptor storage
inside an owner callback cannot redirect the captured indicator ID for this call.

Shadow lists use the existing copied eight-layer API after submission. Supply
matching indicator radii yourself: Checkbox styles can change corners; Radio uses
its circular size/2 caster. This is rectangular-box shadow styling with rounded
corners, not an alpha-mask shadow around arbitrary glyphs. Ancestor masks/opacity,
DROP/INSET phase ordering and renderer budgets remain in
[ELEMENT_SHADOWS.md](ELEMENT_SHADOWS.md).

Named composition uses existing nodes but registers additional explicit IDs and
changes descendant auto-ID derivation in the opted-in branch. Legacy routes keep
their original anonymous identity/capacity behavior. Re-submit named parts each
frame; omission creates no retained paint target. No pure getter invents a named
indicator for an old anonymous facade.

These entry points are C-only. Typed bridge/builder indicator-target mapping and
remaining78-family part coverage remain pending; packaged C headers alone do not
establish Python, Node, TypeScript, Go or Rust authoring support.
