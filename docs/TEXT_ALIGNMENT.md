# Contextual paragraph text alignment

`cui_text_paragraph_colored_aligned` adds physical left, center and right alignment
without changing existing function signatures. It accepts the same text, style,
wrapping and full-text color spans as `cui_text_paragraph_colored`, followed by a
`cui_text_align`. It reserves an available-width paragraph box and positions every
final visual row independently within that box. The width comes from this frame's
layout, including the parent's padding; it does not read previous-frame bounds.

Center uses half the remaining row width; right uses all of it. An overflowing row
receives a negative center/right offset, as in the pinned source. Empty and trailing
lines retain their normal height without producing a text command. These are
physical directions: RTL text does not turn LEFT into logical start. Original
paragraph bytes, bidi context, selected byte ranges and intersected color spans
are unchanged. The resulting draw command bounds carry the final aligned origin,
so existing renderer, clip and copied-snapshot consumers receive that position.
No geometry session is retained or newly acquired for alignment itself.

`cui_label_options.alignment` forwards the same choice. Zero initialization keeps
the existing left/default path, including the legacy paragraph box sizing. Center
and right also expand the optional semantic wrapper to available width, so adding
a semantic ID does not change the text's alignment. Masked labels continue to
submit only bullet bytes, without highlight spans or original semantic names.

The old paragraph API, ordinary `cui_text`, contextual line APIs and legacy Label
wrapper keep their existing behavior. Applications can still align whole boxes
through `cui_box`; text alignment is distinct because rows may have unequal widths.
Scrolling/clipping owners retain Core's existing sizing conventions; applications
that permit oversized scroll content should supply its intended inner width.

The source requirement is frozen GPUI Kit commit
`87d10ae5e1299d1be18670a37570c5402f952e34`, `crates/component/src/label.rs:192-215`:
Label forwards its inherited Styled refinement to its containing div. The frozen
website documents `.text_center()` and `.text_right()` at
`website/component/label.md:36-49`; the actual story exercises both at
`crates/story/src/stories/label_story.rs:193-195`. Its pinned gpui-pre 0.3.8
StyledText renderer forwards `text_style.text_align` to wrapped line painting;
`src/text_system/line.rs:954-976,1000-1005` defines the signed row-width offset.
This API covers those three alignment choices, not arbitrary GPUI StyleRefinement,
justification, logical start/end, or platform pixel parity.

`test_text_alignment_source` checks all three choices at changing current-frame
widths, wrapped and unwrapped unequal rows, hard breaks, blank/trailing lines,
padding, original RTL paragraphs and intersected spans, copied command lifetimes,
invalid alignment/UTF-8 rejection, capacity failure and next-frame recovery,
legacy origin preservation, secondary/highlight colors, and masked semantic Label
alignment. It is portable C11 and runs headlessly with strict compiler warnings
and ASAN/UBSAN. Integrators register this new test and regenerate distribution.
