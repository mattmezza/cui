# Empty source composition

Audited against frozen GPUI Kit source commit
`87d10ae5e1299d1be18670a37570c5402f952e34`,
`crates/component/src/empty.rs`, including every constructor, setter, renderer,
media variant, parent-child implementation and test.

`cui_empty_ex` composes optional header, optional content, and appended children.
Header independently accepts optional media, title and description. The order is
always media, title, description, content, appended children. Assigning another
slot pointer replaces only that slot. A non-null empty slot retains its container;
a null pointer omits it. All pointers are borrowed for the synchronous call.
Each slot accepts text followed by arbitrary application children; use a callback
for multiple custom children, icon/image/avatar compositions, rows and controls.
Native buttons retain their standard pointer, keyboard, focus and disabled rules.
Callbacks receive a temporary theme carrying their slot's font size and foreground;
do not retain its pointer. Child IDs must be stable and distinct from the internal
IDs derived from the root ID.

Source defaults use an explicit 16 pixel rem, overridable independently of theme
font size. Root grows in both axes, centers children, uses 24 pixel padding and
16 pixel gap. Header and content grow to available width with a 24rem cap and
8/10 pixel gaps. Title uses medium 14 pixel text; description uses muted 14 pixel
text with 22.75 pixel line spacing. Media is an intrinsic column; icon variant is
a centered 32 pixel muted square with large theme radius and 16 pixel inherited
font. An additional 8 pixel media bottom space is represented by an empty spacer
plus the header gap, keeping the normal media-to-title distance at 16 pixels.

The root has **no background and no visible border**. Source sets a dashed border
style/color but no border width. CUI preserves this invisible default. The legacy
`cui_empty` wrapper keeps its existing visible border, sizing and typography.

Call `cui_empty_default_style`, change desired fields, then attach the resulting
style to a root/header/slot. This is independent style replacement with all values
explicit, rather than a CSS cascade. Dimensions of -1 request growth; zero fits
unless fixed flags request an explicit zero. Other negative/nonfinite metrics,
invalid media variants and malformed nonempty spans reject before any callback,
layout or semantic mutation. Callbacks themselves must emit balanced layout and
valid controls; their application side effects cannot be rolled back.

Remaining source gaps: CUI core lacks dashed border paint and paragraph alignment,
so custom visible borders are solid and wrapped paragraphs remain left aligned
inside centered containers. Generic `StyleRefinement` flex-wrap, text decoration,
font inheritance into arbitrary external descendants and every CSS-style property
are not reproduced by this typed box/text style surface. Root XL radius maps to
CUI's large theme radius because the theme has no distinct XL token. Therefore this
is a functioning named composition API, not a claim of complete visual/style parity.

`tests/unit/test_empty_source.c` checks ordering, optional omissions, replacement,
24rem bounds, default paint/metrics, Unicode narrow wrapping, native keyboard and
pointer actions, disabled actions and invalid preflight atomicity. The test is registered in CMake and the standalone distribution includes this API.
