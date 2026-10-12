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
entities, escaped punctuation, flat lists with source starting numbers and consecutive numbering, rules, pipe tables and block images. Soft paragraph breaks join with spaces; two-space hard breaks retain line breaks.
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
an explicit host `open_url` callback handles primary activation. URLs remain
borrowed slices of the source buffer and must be copied for deferred use.

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
requires Core, Theme and Scroll (base and components), plus portable platform
clipboard declarations. Styled text uses real weight/italic/monospace measurement
and drawing; register matching native faces for the desired typography. The regular
face remains the renderer fallback. No Content rich-text parser existed to reuse.

Remaining frozen-source gaps are tracked explicitly: complete CommonMark/HTML5
and nested lists, inline/linked images, MDX/plugins/YAML front matter, image
alignment/error UI, global selection spanning multiple views, touch selection menus
and handles, grapheme/BiDi shaping, inherited parent foreground palettes, adaptive
horizontal table scrolling/actions styling,
custom word/CJK stagger and easing, automatic enclosing-list reveal, background
syntax styles, cached/asynchronous parsing and layout. Current tables divide the
available width equally rather than measuring intrinsic column widths. These
limitations must not be presented as full source parity.

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
