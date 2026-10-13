# TextView

This module implements a bounded document model and selectable styled presentation
against the frozen GPUI 0.7.1 TextView contract. It is a substantive first slice;
the remaining source features below are not implemented.

The application supplies `cui_text_view_workspace`: separate source and rendered
UTF-8 buffers, byte mappings, spans, blocks, glyph geometry, and optional highlight
storage. Keep this storage and state alive while the view is submitted. The parser
allocates no memory. Source and rendered buffers include a terminating zero byte;
capacities therefore include that byte. Documents are limited to 65,536 bytes and
8,192 nodes. Glyph capacity must cover rendered byte length. Insufficient parser
capacity and malformed UTF-8 leave the previous document intact.

Initialize with `cui_text_view_init`, update with `cui_text_view_set_text`, and render
with `cui_text_view(ui, id, theme, &state, cui_text_view_default())`. The defaults
select text, fit content without a scrollbar, use theme text/link/selection/code
colors, and leave streaming fades disabled. Set a positive width for deterministic
first-frame wrapping. Grow dimensions use retained bounds and a first-frame
estimate (320 by 240); bounds changes schedule a follow-up measurement frame.

Markdown handles headings, emphasis, strikethrough, inline/fenced code, links,
entities, escaped punctuation, bounded nested lists and passive task checkboxes with source starting numbers and consecutive numbering, rules, pipe tables and block images. Soft paragraph breaks join with spaces; two-space hard breaks retain line breaks.
HTML handles the corresponding safe presentation tags and strips script/style
bodies; it never executes markup or loads resources itself. Each rendered scalar
maps to its original source bytes, including entity atoms, while block separators
have no source mapping. Markdown range highlights accept rendered byte ranges;
source range conversion excludes syntax. HTML source mapping/highlights return
unsupported, matching the frozen source restriction. Selection endpoints must be
UTF-8 boundaries. `cui_text_view_select` reports whether selection changed.

Selection supports separate-frame pointer dragging, Shift plus arrow/Home/End,
select all, Escape clearing and copy through the optional portable clipboard
interface. It is read-only. Links remain independently focusable; primary and
secondary activation forward the original event to `on_link`. A selection drag
suppresses link activation. An installed callback consumes activation; otherwise
an explicit host `open_url` callback handles primary activation. URLs are decoded temporary slices valid during the callback and must be copied
for deferred use.

Image loaders return borrowed straight RGBA bitmaps; pixels must survive renderer
completion. Image callbacks run during submission and may report not ready until
a later frame. The application schedules invalidation when asynchronous work
completes. Actual bitmap drawing uses Core clipping and the renderer adapters.
Code and table action callbacks receive a unique action container ID and compose
real controls. Code actions overlay the top right with 8px insets and a fitting
opaque background, after document text paints. They do not enlarge code blocks.
Table actions occupy a full-width footer separated by 4px. Callback heights are
explicit; callbacks must not end the owning container. Interactive action controls
keep their own pointer gestures and do not start document selection. The optional code highlighter supplies foreground colors per code byte.

Scrollable views compose the real Scroll component, including scrollbars and
consumed-event handling. Reveal requests retain only the latest range, expire
after one second, follow unchanged source edits, and reveal the nearest line in
the view's own scroll area. Fit views report window line bounds through an explicit
host callback. Max-lines fit mode clips oversized content and drops ordinary lines
that straddle the cap. Streaming append fades last 350 ms; replacement is immediate
and reduced motion disables animation. Ordered highlights paint in request order,
then selection above them. Unchanged source prefixes/suffixes retain highlights
through edits. A replacement inside a table drops highlights from the edited row
and subsequent rows because their cell positions may change; earlier unchanged
rows and tables shifted as an unchanged suffix retain highlights. Nested HTML
emphasis preserves outer styles after an inner close.

`src/base/cui_text_view.c` links with the C math library only. Styled presentation
requires Core, Theme and Scroll (base and components), the components checkmark drawing helper, plus portable platform
clipboard declarations. Styled text uses real weight/italic/monospace measurement
and drawing; register matching native faces for the desired typography. The regular
face remains the renderer fallback. No Content rich-text parser existed to reuse.

Remaining frozen-source gaps are tracked explicitly: complete CommonMark/HTML5
and complete nested-list/quote structure, MDX/plugins/YAML front matter, image
error UI, complete shared-window selection identity, touch selection menus
and handles, grapheme/BiDi shaping, inherited parent foreground palettes, fully configurable table/action refinements,
automatic enclosing-list reveal, background
syntax styles, cached/asynchronous parsing and layout. Default wrap tables use
capped text-length column proportions (5 to 150 characters) and 64px minimum
columns. Narrow wrap tables clip at the document edge; opt-in adaptive horizontal
table scrolling is available through `table_mode`. These limitations must not be presented as full source parity.

Headless tests cover transactional bounded parsing and randomized syntax, Unicode
mapping/selection, edit tracking, entities, safe HTML, tables, clipping, fade state, composed action geometry and clicks,
fence lengths and code content,
real styled draw commands, separate-frame selection and focus, primary/secondary
link callbacks, drag suppression, bitmap drawing and invalid geometry.

Source-format copy uses `cui_text_view_copy_selection` with a separate caller-owned
output buffer. It reconstructs partial inline emphasis, links, code, headings,
quotes, lists, aligned table rows and fenced code. Table selections preserve all
table rows, emit blank unselected cells and retain the delimiter alignment row.
Select-all returns the original Markdown verbatim;
HTML and plain documents use rendered text. Capacity queries report required bytes
without writing output, and insufficient capacity preserves existing output.
For Ctrl+C, provide `options.copy_buffer/copy_capacity` and set `copy_format` to
Source. Scrollable views copy plain text, matching the frozen source's effective
format. Insufficient output capacity leaves the clipboard unchanged.

Default table cells use 8px horizontal and 4px vertical padding, normal inherited
text weight, clipped content, and one border at each shared edge. Header backgrounds
use the code background, body cells use the theme surface. TextView links follow
the frozen text style's primary color mapping rather than the general link token.

The pure layout API additionally exposes `table_mode` (`WRAP` by default,
`ADAPTIVE`, or `NOWRAP`). Adaptive columns measure each trimmed cell line using
its styled spans, retain a 48px minimum, and shrink to content-dependent floors
between 160px and 480px (never above the measured content). Nowrap columns keep
single-line content widths. Both account for 16px horizontal padding, shared
1px column borders, and the 2px outer frame. Extra viewport space grows columns
in proportion to measured widths. The styled component uses the same mode and composes actual axis-locked horizontal
Scroll owners with hidden bars. Borders stay fixed while cell content moves;
links and selection use translated coordinates. Each first table cell owns its
scroll state, retained across append/edit parsing when its block ordinal and
source start remain stable. Reveal requests bring the target cell text into the
horizontal viewport. Changing a table's source start or block ordinal resets its
scroll state. Window-shared selection, touch and horizontal selection edge
autoscroll remain separate limitations.

Task list markers (`[ ]`, `[x]`, `[X]`) are source metadata rather than selectable
text. Partial Source copy reconstructs them after the list marker. Presentation
uses a 14px foreground border box, a foreground fill when checked, and the shared
10px checkmark drawing helper; the indicator has no interactive semantics.
The checkmark chooses black or white contrast against the supplied foreground,
which preserves the source defaults while also supporting custom palettes.
Continuations after nested child blocks and nested non-list blocks remain parser limitations.

`cui_text_view_decode_resource` decodes the bounded parser entity subset and
Markdown ASCII punctuation escapes into a separate caller buffer. HTML attribute
backslashes and Plain format bytes remain literal. Link, opener and image callbacks
receive decoded, zero-terminated URI bytes borrowed only for the callback;
asynchronous loaders must copy them. No URL fetching or URI policy is implicit.
Source copy retains original source spelling. Complete HTML5 named entity and URI
normalization behavior remains part of the full parser gap. Resource callbacks
use at most 64KiB of temporary stack storage, reused across images; entity lookup
scans no more than 17 bytes per candidate.

List prefix spacing follows measured text, including the trailing space from the
source prefix helper. Ordinary markers reserve their full measured prefix width;
they do not impose a synthetic minimum or extra gap. Task indicators retain their
separate 14px box plus 6px gap. Prefix bytes live in the caller block workspace so
draw-command text remains valid through renderer completion.

Horizontal table routing consumes horizontal wheel input at an edge or without
overflow. Native precise gestures retain their initial axis until an end phase,
a 28ms quiet interval, or a strong axis change. The frozen source disables this
persistent gesture locking on WASM; ordinary vertical chaining is preserved.
Host adapters must provide precise gesture phase metadata explicitly.

Markdown list item nesting is tracked with parent block indices, source starts,
and independent per-level ordered counters. Nested ordered markers use uppercase
letters at depth one and lowercase at deeper levels; zero-start lists retain
numeric prefixes. Bullet shapes vary across five depths. Each rendered nesting
level adds 16px, while partial Source copy uses parent marker widths to indent
children and includes empty ancestor markers for a selection confined to children.
The parser supports at most 32 list levels and rejects deeper input transactionally.
Nested-child continuations reconnect to their enclosing list item. The first
paragraph after a nested list repeats the parent marker or passive task indicator;
later paragraphs use the continuation indent. Source copy groups an item's selected
paragraphs before its nested lists, while plain copy and selection retain paint
order. The bounded serialization order uses at most 8192 16-bit block indices and
may take quadratic time in that fixed node bound. Fenced code following an existing list paragraph retains parent membership and
removes the opening indentation from content while keeping original byte mappings.
It renders in the list continuation column with working code action callbacks.
Fences written directly after a list marker are real code blocks with independent
list-item metadata: the marker or passive task indicator occupies a separate
column, outside the code background. Such code blocks own nested-list ancestry and
keep exact source mappings for literal backslashes and subsequent lines. Loose-list
spacing and multi-paragraph quote containers are not yet a complete list AST. First-child headings and single-paragraph quotes use real block
styles with a separate normal-size list marker column. Following indented headings
and quotes remain children of the enclosing item. List ordering is independent of
heading level, preserving bullet versus decimal prefixes and subsequent counters.

Ordinary list paragraphs join lazy continuation lines with spaces and preserve
two-space hard breaks. A following indented paragraph stays attached to its
list item, paints at a 16px continuation indent, and does not repeat a task
indicator or marker. Partial Source copy prefixes the first selected item
paragraph and indents subsequent paragraphs and hard-break lines under its marker.
Continuations after nested child lists follow the separate row and copy rules above.

Setext headings recognize one or more equals signs or hyphens after a paragraph,
including joined paragraph lines. Leading indentation is limited to three spaces.
Without an opt-in frontmatter parser, a leading horizontal rule followed by a
Setext heading retains Markdown meaning; YAML frontmatter plugins remain absent.
Prepared glyph lines use core's explicit no-wrap text leaf so Clay cannot wrap a
measured line a second time because of fractional font measurements.

Consecutive explicitly prefixed quote lines form one paragraph, with soft lines
joined and two-space hard breaks preserved. This works for root quotes, first
list-item quotes and indented list continuation quotes. Partial Source copy
repeats the quote prefix after a hard break and keeps any surrounding list indent.
Blank quote paragraphs, lazy unprefixed quote continuation, and nested quote
containers remain outside this bounded paragraph representation.

Markdown images in list items retain parent ancestry, ordered counters and Source
copy grouping. Standalone Markdown images now use the same one-byte paragraph
object flow as mixed images, including default inline sizing, loaded/unloaded slots
and list continuation marker rules. Their ranges cover complete image syntax,
rather than the former alt-text ranges. Bare root HTML images also use a separate paragraph with an atomic image slot.

Exact source inline image flow requires styled font ascent, descent and baseline
metrics. In frozen `inline_flow.rs`, each line starts with the body's baseline
and descent. An image baseline is clamped to its height from
`height / 2 + body_baseline - line_height / 2`; line ascent and descent expand
with that baseline, and the image paints at `line_y + line_ascent - baseline`.
The optional core font metrics provider now supplies actual body ascent and descent.
The body baseline is `(line_height - ascent - descent) / 2 + ascent`. Ready standalone
images use this formula and retain a separate logical row extent for list prefixes
and following blocks. Pure layout accepts validated optional body metrics. Without
a valid provider, images retain top alignment; no size-derived baseline is synthesized.
Mixed Markdown images now occupy atomic one-byte span slots. Their host bitmaps
reserve horizontal space among text runs and wrap at object boundaries. Default
inline height is 75% of the logical line height, with loaded aspect ratio or a
square unloaded slot, matching the frozen source. Selection maps each slot to
the entire image source syntax, and Source copy preserves that syntax. Title tooltips,
standalone-image dimensions and custom object renderers remain unimplemented. Bare root HTML image blocks still retain their earlier alt-text ranges.

Tables following existing list content retain parent membership and paint in the
16px continuation column. Column sizing, horizontal viewport clipping and footer
action widths use the remaining column width. Source copy emits the table among
the parent item's own blocks, indents every table row beneath its marker, and keeps
nested child lists after that content. Tables within a list omit inter-block
paragraph space; the list still separates from following root content. A final
root table also omits trailing paragraph space, matching the document's last-block
rule. First-child tables introduced directly by a list marker preserve list-item metadata
on the first cell. Other cells and nested children retain that item as their parent.
The separate marker column stays outside the horizontal mask and does not move
when the table scrolls; table-body snapshots preserve its scroll offset when
appending rows.

Inline image slots use a single space byte in the rendered selection coordinate
space; Plain copy omits that slot, while Source copy writes the original image
syntax. No unloaded bitmap is fabricated. Removing the host loader restores a
compact square slot without discarding surrounding text or retaining stale pixels.

Layout may provide a font-metrics callback keyed by each block's actual font size
and style flags. The styled component uses the core provider, so a semibold heading
image uses heading metrics rather than regular body metrics. The callback supersedes
fixed body metrics and may decline measurement; unavailable or invalid callback
metrics use top alignment. Host bitmap validity follows the shared core contract,
and invalid loader output becomes an unloaded slot instead of a malformed draw.

Each mixed image line expands its logical ascent and descent from the actual body
baseline and image baseline. Wrapped following lines begin after that complete
logical extent, including fonts whose ascent or descent exceeds the requested line
height. Text-only lines retain their requested logical height.

Bounded balanced brackets and parentheses recognize linked Markdown images and
parenthesized destinations. Quoted titles and angle-delimited destinations no
longer become part of the host image URI. A linked image uses its alt source as
its link label, activates the host link callback through ordinary capture, and
Source copy preserves its enclosing link. The frozen renderer itself ignores the image title (`image_element` takes `_title`),
so no image-title tooltip is required. This delimiter parser is not a complete
CommonMark grammar.

Images inside HTML paragraph, heading and table-cell containers now share the
atomic inline flow with neighboring styled text. Linked HTML images also use the
ordinary link callback. Missing `src` images inside these containers are skipped.
Bare root HTML images use the same sizing rules in their own paragraph.
HTML source-range/copy support remains unfinished. HTML container tracking is bounded
and does not replace a repairing HTML5 tree parser.

Inline HTML image dimensions support finite nonnegative numbers, `px`, and `%`,
with width/height attributes taking precedence over matching CSS declarations.
Percentages use the logical line height as their base, matching frozen
`inline_flow.rs`, rather than viewport width. A single loaded dimension preserves
intrinsic aspect ratio; an unloaded width-only image uses the line height, and an
unloaded height-only image uses a square. Both dimensions are independent when
specified. Invalid declarations fall back to valid CSS or default dimensions.
Numeric tokens are bounded to 64 bytes, exponents to ±38, and dimensions to
10,000,000 logical pixels. Zero-sized images retain their selection slot and paint
no bitmap. Caller-provided NaN/negative/nonfinite dimensions are rejected before
layout mutation. Tall image rows retain their logical origin separately from shifted
text bounds, so aligned table cells move all runs together.

Both Markdown and HTML standalone images now use one-byte image slots and source
default sizing rather than alt-text ranges or unscaled intrinsic pixels. Bare root
HTML images retain separate paragraph boundaries and accept the same explicit
dimensions as paragraph images. Missing root `src` images are skipped. HTML tag
boundaries respect quoted `>` characters and HTML attribute whitespace.

HTML `<img>` tags embedded in Markdown use the same atomic run model, decoded
URI, explicit dimensions, baseline and enclosing Markdown link behavior. This
includes the frozen linked HTML image after a soft newline regression. Missing
`src` tags are skipped. Other embedded HTML tags and multiline HTML grammar remain
outside this targeted inline-image handling.

Plain copy and Ctrl+C omit image slots, matching source paragraph text output.
Copying only an image in Plain mode produces no text and leaves the clipboard
unchanged. The raw `selected_text` accessor remains a borrowed coordinate slice
of `workspace.rendered`, including object slots, valid until the next text mutation;
use `copy_document_selection(PLAIN)` for styled presentation or clipboard text. Default Plain
clipboard output uses bounded temporary storage when no caller buffer is supplied.
Ordinary Source-format Ctrl+C continues to require caller output storage.

The frozen implementation selects shared text runs around images and includes
images in Source copy when selection spans the adjacent run boundaries. CUI still
retains atomic image slots in its low-level coordinate API. Styled pointer selection
projects text glyphs instead of independently selecting those slots; full shared-window
selection identity remains tracked.

Markdown Source copy and selected source ranges include consecutive images at a
paragraph boundary when selection begins at the first text byte after leading
images or ends at the last text byte before trailing images. Selecting only a
middle portion of that text does not include the boundary images. This follows
the frozen paragraph's pending-image rule while leaving Plain output and raw
selection coordinates unchanged.

Optional shared selection uses an application-owned `cui_text_view_selection`
and a fixed `cui_text_view_selection_member` array. Initialize the manager, then
register each live state with its unique view ID, nonzero scope, stable logical
`document_order`, table mode and enabled flag. Registration order does not control
copy order. Duplicate state ownership and replacement of a live ID with a different
state are rejected. Remove a member **before** destroying its state or workspace;
removal clears its selection and removes the retained pointer. The array and
manager must also outlive all uses. Re-registering the same ID/state updates its
options; disabling the owner clears the active shared selection.

For each frame, open the application's nonmodal focus scope matching the member
scope, call `cui_text_view_selection_update` once before painting any members,
then submit those views with `options.shared_selection` set to the manager and
close the application scope. This preserves one event reduction and lets every
member paint the same projection. Initial hit testing uses the previous clipped
semantic ownership; member geometry becomes available after its first submitted
frame. Do not update a scope before its modal/ancestor guards have been submitted.
Registration does not create UI or make a disabled member selectable.

A cross-view drag retains window anchor/cursor points and projects their row band
through each member's retained viewport, vertical scroll and horizontal table
scroll. Glyph midpoints follow the frozen window selection band rule. Image slots
are not independently selected by this shared projection; text on both sides can
still select their Source syntax. Other scopes are not projected. Ctrl+C joins
nonempty selected participants with a newline in document order; Source copy uses
each document's existing reconstruction. Caller copy storage is recommended for
large multi-view selections; capacity failure leaves the clipboard unchanged.
Standalone views retain their existing behavior when the manager is absent.

Remaining shared-selection gaps include touch handles, comprehensive multi-click coverage across every block kind,
content-key resolution across dynamic renderers, cross-view automatic scrolling,
and individual image-separated text-run ranges within a paragraph. Projection
retains a range for each block/table cell and clips to its viewport, rather than
maintaining the source window manager's full inline-run state.
Applications must refresh registration enabled flags and call the update before
painting; there is no implicit window registry or stale-participant eviction.


Styled Ctrl+C and shared copy use `cui_text_view_copy_document_selection`, which
serializes selected blocks. Partial Plain Paragraph, Heading and Code selections
include the source block's trailing newline. Table cells join with spaces, rows
with newlines, and the table adds a final newline. Shared selection joins these
participant strings with another newline in document order, preserving the frozen
output (two selected one-line paragraphs copy as `first\n\nsecond\n`). Source
mode retains Markdown reconstruction without those Plain terminators.
`copy_selection(PLAIN)` remains a low-level coordinate slice copy with image slots
omitted; it does not serialize document structure. `selected_text` is a borrowed
bounding coordinate view and may include unselected gaps in a shared projection.
Use document selection copy for source-equivalent styled text output.

Shared projection stores separate ranges for wrapped table cells. Selection of
one visual row therefore does not pull later wrapped text from a different cell
into its copy or highlight. Within one paragraph, separate image-delimited text
runs still share one block range; exact per-inline-run state remains tracked.
Re-registering an existing member under a different scope or table mode clears
its old projection; if it owns the active drag, the manager clears that drag.
Changing text discards glyph geometry and projected ranges. Such a member cannot
start a shared gesture until it has submitted fresh geometry; retained viewport
coordinates alone are insufficient.


Ordinary pointer drags now use the same window-point projection as shared selection.
Dragging within an image selects no text and copies no Source syntax; a tap on a
linked image still activates its link, while a drag cancels that release activation.
Reverse text-image-text drags retain their direction and include image syntax in
Source copy when adjacent text boundaries are selected. Programmatic
`cui_text_view_select` remains the explicit low-level coordinate API and can select
an image slot by itself; that does not describe the styled pointer behavior.

HTML Source fallback to Plain and unavailable HTML source offsets match the frozen
implementation's `effective_format` contract and html5ever's absent source spans;
they are not missing HTML reconstruction features promised by that source.

Mouse multi-click selection now uses the pinned source character classes and 128-scalar word boundary limit. Double-click selects a word without splitting UTF-8 or crossing image runs; triple-click selects an ordinary paragraph or the visual row in an image-flow paragraph. Higher counts use logical-line boundaries. Plain multi-click copy preserves the chosen text override while Source copy retains Markdown markup. Shared selection clears other members, retains focus ownership and preserves the selection after release. Headless tests cover bounded long words, UTF-8 offsets, whitespace/punctuation, image boundaries, paragraph/row copy, text mutation and shared view ownership. Touch gesture selection and complete coverage of every block/run kind remain unfinished.

The website's Markdown example starts with a visible fenced C code block, then shows emphasis, a link, lists, a quote and a table. Select **Scrollable** to constrain the document viewport, or switch **TextView format** to HTML or plain text. Fenced code is parsed and rendered in monospace with the theme's code background; it is separate from the website's copyable C usage excerpt.


## Stream motion policy

The frozen `crates/base/src/text/stream_fade.rs` at commit
`87d10ae5e1299d1be18670a37570c5402f952e34` defines words with their trailing
whitespace as reveal units, or individual Chinese/Japanese/Korean scalars.
CUI now exposes this timing through `cui_text_view_motion`,
`cui_text_view_set_motion`, and styled `options.custom_motion/options.motion`.
The default remains 350 ms, no stagger, ease-out cubic. Linear and ease-in-out
cubic are also supported; arbitrary user-defined easing functions are not.

Configure state **before** applying an appended document update. Initial document
loading normally precedes enabling streaming so existing text appears immediately:

This complete headless C11 program needs Core and the TextView components, with
no browser or native font adapter. Its assertions verify stagger timing and
reduced motion; a real host adds event delivery and draw-list painting:

```c
#include "cui/cui_text_view.h"
/* Keep this executable verification example active in release builds too. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <math.h>

int main(void) {
    static char source[1024], rendered[1024];
    static cui_text_view_mapping mapping[1024];
    static cui_text_view_span spans[256];
    static cui_text_view_block blocks[128];
    static cui_text_view_glyph glyphs[1024];
    cui_text_view_workspace workspace = {
        .source = source, .source_capacity = sizeof source,
        .rendered = rendered, .rendered_capacity = sizeof rendered,
        .mapping = mapping, .mapping_capacity = 1024,
        .spans = spans, .span_capacity = 256,
        .blocks = blocks, .block_capacity = 128,
        .glyphs = glyphs, .glyph_capacity = 1024
    };
    cui_text_view_state state;
    assert(cui_text_view_init(&state, workspace, CUI_TEXT_VIEW_MARKDOWN));
    assert(cui_text_view_set_text(&state, cui_string("Before")) == CUI_TEXT_VIEW_OK);
    cui_context *ui = cui_create((cui_config){0});
    assert(ui);
    cui_text_view_motion motion = {
        .duration = 1, .stagger = .2f, .easing = CUI_TEXT_VIEW_FADE_LINEAR
    };
    assert(cui_text_view_set_motion(&state, motion));
    state.stream_fade = true;
    /* A real host sets the context preference from its platform settings. */
    state.reduced_motion = cui_reduced_motion(ui);
    assert(cui_text_view_set_text(&state, cui_string("Before one two")) == CUI_TEXT_VIEW_OK);
    assert(cui_text_view_stream_opacity(&state, 7) == 0);
    assert(cui_text_view_stream_opacity(&state, 11) == 0);
    cui_text_view_options options = cui_text_view_default();
    options.width = 300;
    options.stream_fade = true;
    options.custom_motion = true;
    options.motion = motion;
    options.reduced_motion = state.reduced_motion;
    options.dt = .3f;
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 400, 240, .3f));
    assert(cui_text_view(ui, 77, &theme, &state, options));
    cui_draw_list draws = cui_end_frame(ui);
    assert(draws.count && cui_last_error(ui) == CUI_OK);
    /* The component paints first, then advances its clock for the next frame. */
    assert(fabsf(cui_text_view_stream_opacity(&state, 7) - .3f) < .00001f);
    assert(fabsf(cui_text_view_stream_opacity(&state, 11) - .1f) < .00001f);
    cui_set_reduced_motion(ui, true);
    state.reduced_motion = cui_reduced_motion(ui); /* Before the next text update. */
    options.reduced_motion = state.reduced_motion; /* Also forward every frame. */
    assert(cui_text_view_set_text(&state, cui_string("Before one two three")) == CUI_TEXT_VIEW_OK);
    assert(cui_text_view_stream_opacity(&state, 7) == 1);
    assert(cui_text_view_stream_opacity(&state, 15) == 1);
    assert(!cui_text_view_advance(&state, 0));
    assert(cui_begin_frame(ui, 400, 240, .3f));
    assert(cui_text_view(ui, 77, &theme, &state, options));
    cui_end_frame(ui);
    cui_destroy(ui);
    return 0;
}
```

Save the complete program as `textview_motion.c`, build the library, then run:

```sh
cc -std=c11 -Wall -Wextra -Wpedantic -Werror -Iinclude textview_motion.c build/libcui.a -lm -o textview_motion
./textview_motion
```

Each changed contiguous run within a paragraph/code/table-cell leaf starts its
own word timing. Unicode whitespace follows the preceding word; leading
whitespace joins the first word. CJK ranges match the pinned source:
Hiragana/Katakana, CJK Extension A/unified/compatibility ideographs, Hangul
syllables, and ideograph extensions U+20000–U+2FA1F. Every byte of a UTF-8 scalar
receives the same birth time. Staggering spans inline formatting within a leaf;
it restarts at block and table-cell boundaries. If the last unit would start
later than the fade duration, the entire changed run fades as one chunk. CUI
uses its existing rendered-byte/format comparison, rather than introducing a
full source leaf-identity tracker; more complex reparses can have different
chunk boundaries from the frozen implementation.

`cui_text_view_stream_opacity` samples the current alpha multiplier without
painting. `cui_text_view_advance` continues to request frames until the last
delayed unit reaches full opacity. A disabled fade, zero duration, replacement,
or reduced motion paints immediately. Replacing text clears unfinished fades
in unchanged prefixes too. Invalid negative/nonfinite durations or stagger and
unknown easing values fail without changing the state. A zero duration clears
pending fades; other policy changes update durations/easing of active fades but
do not reschedule already recorded stagger birth times. Set a policy before
`set_text` for deterministic new-chunk timing. TextView's state clock is advanced
by `options.dt` at the end of component submission, after painting. Supply the
elapsed interval once per frame; do not also call `cui_text_view_advance` for that
same interval. The pure model API may instead advance explicitly when used
without the styled component. `cui_begin_frame`'s dt alone does not advance this
model clock.

TextView does not implicitly copy `cui_reduced_motion(ui)` into its state/options.
Forward the host preference to `state.reduced_motion` before a text update and to
`options.reduced_motion` on each submission. This prevents new fade birth times
while reduced motion is active and immediately suppresses existing fades. A
zero-duration motion clears pending birth times; merely suppressing a fade does
not clear them, so unfinished text may resume fading if the preference is turned
off before its stored deadline. The styled custom policy is
validated before image, link, or composition callbacks run.

Headless tests exercise overlapping word timing, delayed animation completion,
Latin/CJK/Unicode-whitespace scalar boundaries, oversized chunks, easing values,
zero/invalid policies, replacement and reduced motion, Markdown completion,
independent table-cell timings, transactional failed parses, and actual text
command alpha. Complete parser/plugins, touch selection, shaping and other gaps
listed above remain outside this change.
