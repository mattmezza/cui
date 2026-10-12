# Message and MessageScroller

Contracts were audited against the frozen gpui-component 0.7.1 Message and
MessageScroller sources and website documentation in the local reference checkout.
These components leave application message data, formatting and actions with the
caller.

`cui_message_options_default()` uses start alignment, width 480, avatar-row gap 8,
vertical gaps 10 and metadata gap 4. Avatar content reserves at least 32 pixels of
width and aligns to the bottom of the header/body stack. End alignment reverses
that row. Footer content is outside that row and indents 40 pixels on the avatar
side. Header/footer use muted 12-pixel text at the default theme font size and
12-pixel horizontal insets. A typed ghost Bubble anywhere in the body suppresses
both default metadata insets; explicit ON/OFF wins. Arbitrary callbacks do not
participate in typed Bubble inference. A message is passive by default; opt into
`CUI_ROLE_LISTITEM` with an application name and stable ID when appropriate.

Slots accept borrowed text or a rendering callback with available width, color,
font size and alignment. Body slots may wrap an existing typed Bubble or render
arbitrary children. Callbacks may create real interactive controls. Borrowed text,
body arrays and callback users must remain valid through `cui_end_frame`.
`cui_begin_message_group` opens a vertical container; pass gap 8 for the reference
default. Caller supplies width and closes it with `cui_end_message_group`.

Create a scroller with an explicit capacity, item count and estimated raw row
height. Allocation is bounded at creation; capacity is 1–1,000,000 items and no
operation grows it. Heights and spacing are finite and at most 1,000,000 pixels;
estimates must be positive. `memory_bytes` reports retained allocation. State
retains layout keys and heights, never message records. Destroy it explicitly.
The application updates its collection and performs matching reset/splice/append/
prepend operations between frames. Splice ranges are half-open. Invalid ranges,
capacity overflow and invalid geometry leave the model unchanged.

Initial/reset position follows the tail. Appends follow only while that mode is
active. Prepending and splicing preserve the surviving item anchor, including
remeasurement of the previous last row's missing trailing gap. Scrolling to an
item disables following even when the content fits. Scrolling to the end resumes
following. Global remeasurement restores proportional position; selective
remeasurement keeps the item anchor and cached height until its replacement is
measured. Styled width changes request global remeasurement. Retained row bounds
supply measurements on the next frame; explicitly invalidate selective ranges
when application content changes.

Scroller defaults are 480×320, horizontal insets 12, top/bottom padding 8, row gap
32 except after the final row, and 400 pixels of overdraw on each side. Rendering
calls only this bounded visible range. Row callbacks receive the inner width and
stable internal row ID. Do not mutate collection/state until frame completion.
The owner has a focusable LOG role. Vertical dominant wheel events (vertical on
ties), arrows, PageUp/PageDown, Home/End and the optional scrollbar update position.
At scroll edges the normal core parent chain applies. Hiding the scrollbar does
not disable these interactions.

The jump button is enabled by default, named “Jump to latest”, and retains its
built-in action when custom content/size is supplied. It fades with 200ms cubic
ease-out by default, disables during exit, and exposes `next_wake_seconds` for
host scheduling. Reduced motion and zero jump duration settle immediately. An
optional 48-pixel bottom fade has its own 200ms transition. The renderer samples
that fade with 32 bounded strips. Overlays need one initial bounds-tracking frame.

The base composition/model and drawing-free scroller do not link theme
implementation. The base UI needs core, virtual-list base, scroll base and math.
Styled code additionally needs theme, content/Bubble and styled scrollbar code.
Core must include LISTITEM/LOG roles, dominant-axis scroll routing, retained bounds
and opt-in semantic container focus.

Known differences remain: built-in Image, TextView and Attachment dependencies are
absent and require application callbacks; font weight and true rounded image
clipping are unavailable in the current primitives; avatar background uses the
available surface token. Existing Bubble spacing is reused. Arbitrary source
style refinements are represented by slots and geometry rather than a complete
GPUI style builder. Jump tooltips and a configurable button variant are not yet
provided. The accessible jump name deliberately improves on the source's empty
icon-only default. No full rendering or native dependency parity is claimed.
