# Input grow-width containment: bounded correction

Base `41429b9d72037834af31352cde6113475f317b59`. No Editor changes.

The pre-change headless regression confirmed the documented bug: a 255-character
value measured 2550px; in a fixed 400px parent the Input frame was 400px but its
viewport was 2550px instead of 378px. This is actual geometry overflow, not an
unsupported assumption about fitting an unwrapped value.

Core `cui_core_axis` maps negative width to Clay GROW, with zero minimum. In the
pinned Clay main-axis sizing pass (`third_party/clay/clay.h`, "If the parent clips
content ... don't compress children"), a clipped row skips compression. Input's
outer clipped row therefore retained its content-sized growing viewport.

The component-only correction inserts one anonymous percentage-width row for
negative-width single-line Input. That row takes the resolved frame width
independently of text; its unclipped flex layout constrains the existing viewport
between the existing exact float insets. The frame and viewport both retain their
clips, and the text wrapper remains unwrapped. There is no previous-width cap,
extra reduction, Core change or public API. Fixed/default-width and multiline
composition are unchanged. The extra row costs one layout element per growing
single-line field.

Frozen upstream `87d10ae5e1299d1be18670a37570c5402f952e34`:
`crates/component/src/input/input.rs::render_editor` uses a full-size flex wrapper;
`crates/base/src/input/base/element.rs::request_layout` requests relative width
1.0. Its prepaint computes scroll size against current bounds. Do not infer
CUI's current-frame scroll behavior from that source behavior.

The new passing fixture covers first-frame long text and 400→220→500→180 resizing,
column and flex-row/dashboard composition, all three alignments, fractional
padding, border/no-border/no-appearance/invalid variants, preserved outer and
viewport clip streams, fixed/default widths, and settled scrolled pointer hit /
Shift+Home selection. Existing Styled InputGroup with long text retains prefix
40px/suffix56px at widths400/220/500; actual input widths are304/124/404 and its
tracked viewport stays within that input. Existing Input and InputGroup event
suites remain passing, including their acceptance/history/reduction coverage.
No acceptance, IME composition, history or callback implementation changed.

## Baseline before same-frame placement

Width containment left a one-frame delay in responsive caret placement. The
original opt-in `--probe-caret` reproduced it:

| Frame width | Current viewport | End caret X | Viewport right | Stored scroll | Needed scroll |
| --- | --- | --- | --- | --- | --- |
| 400 | 378 | 388 | 389 | 2173 | 2173 |
| 220 | 198 | 388 | 209 | 2173 | 2353 |
| 500 | 478 | 208 | 489 | 2353 | 2073 |
| 500 again | 478 | 488 | 489 | 2073 | 2073 |

`cui_input_paint` reads `cui_bounds(viewport)` before layout to clamp scroll; Core
explicitly returns previous-frame bounds. Merely fixing viewport sizing cannot
make that calculation current. Shrink can hide the end caret for a frame; grow
can leave excess scroll until the next frame. Selection paint and host IME caret
geometry share that stale projection. The later same-frame placement regression promotes these cases to the ordinary suite.

Any follow-up must be reviewed separately before Core/API work. Existing
prepaint supplies current bounds but forbids layout/input mutation; updating
scroll state alone does not reposition already-laid-out text/caret or its tracked
IME geometry. In particular, `cui_input_paint` receives a stack-local temporary
state inside Styled InputGroup: a deferred callback must not retain that state
pointer. An architecture proposal must resolve that lifetime and geometry
ownership, not silently add an end-frame borrowing requirement to existing APIs.

The original fixture verified containment. The updated fixture in
`tests/fixtures/input_grow_width/README.md` also verifies same-frame placement.

Verified on this slice: strict-warning Release and ASan/UBSan builds each pass
`input_grow_width`, `input`, and `input_group_events` (3/3) through the private
Xvfb wrapper. Formatting and whitespace checks pass.

## Same-frame projection proposal

The 400→220→500 probe was rerun on `6a8dfa8a` through private Xvfb.
The first shrinking frame published an IME anchor with X=388 while the viewport
ended at X=209. The first growing frame put the caret at X=208 while the right
edge was X=489. This affects what's painted during a drag even when the host
presents every resize frame.

Use a private, declarative horizontal viewport record. Styled Input supplies
its measured display extent, caret position, alignment, incoming scroll and
optional placeholder geometry. The registration copies scalars and uses only
Core-owned storage and current-layout element references. It borrows no
`cui_input_state`, edit model, string, callback or userdata. The pinned Clay
integration resolves registered placements immediately after horizontal sizing,
before positioning, clipping, offscreen culling or floating-root placement.
Text, selection, the caret and the host IME anchor then share the same layout.
An overflowing value starts at the logical left edge and scrolls. Center and
end alignment apply while the value fits. A placeholder aligns by its own
measured width. Repeating a layout calculation uses the same copied seed.

The completed-frame projection records the actual scroll and the synchronously
returned scroll for the viewport ID. Reduction uses validated previous-frame
geometry when the supplied scroll still matches that return value. An explicit
change to the supplied scroll takes precedence. An omitted, invalid or failed
frame cannot publish a new projection or resurrect an older one. The next
submission reconciles its retained scroll from the completed frame.
`cui_input_state.scroll_x` continues to be written synchronously. An additive
read-only C query exposes the actual growing-viewport scroll when a consumer
needs it immediately after layout. Short-lived paint-only states remain valid.

The implementation keeps the existing component submission and event order,
the underlying public structure layouts and the active font geometry. Use the
existing context resource budget for per-viewport storage. Test the capacity
path and guard publication after layout/render-command failure. The layout hook
processes only registrations, and it does not add another sizing or measurement
pass. Native transition behavior that overrides bounds after sizing needs its
own evidence.

Prepaint runs too late to affect Clay culling or positioned children. Adjusting
draw commands afterwards would also miss tracked bounds and floating clips. A
second sizing pass would repeat text/layout work during live resize. Delayed
writeback into state supplied to existing functions would extend caller
lifetimes. This proposal keeps the adjustment in the ordinary layout pass and
keeps event dispatch unchanged.

1. Add the layout integration and direct C regression tests for live width,
   paint and IME agreement, alignment, optional placeholder and event timing.
2. Exercise the shared styled path with InputGroup and other temporary-state
   callers, next-frame hit testing, capacity and stale publication. Promote
   the reproducer to an ordinary required regression.
3. Validate the real WASM gallery and regenerate the standalone distributions,
   SDK sources, usage metadata and consumer skill.

## Implemented scope

The ordinary grow-width fixture now checks the previously failing sequence, all
three alignments, paint and tracked IME agreement, short and overflowing
placeholders, next-frame pointer hits, InputGroup and resource exhaustion. Real
Chromium exercises the first frame at five widths during both committed input
and active preedit. `cui_input_scroll_x` reads the actual completed growing
viewport. The synchronous state field remains usable by legacy callers. The
placement record retains no caller state after submission. The Clay revision is unchanged and
its tiny local hook is recorded in the pin manifest.
