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

## Remaining same-frame caret limitation

Width containment is **not** complete responsive caret/scroll correctness. The
opt-in `--probe-caret` requirement fails after this correction:

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
geometry share that stale projection. This probe is intentionally opt-in and
failing, not a test that enshrines the defect as success.

Any follow-up must be reviewed separately before Core/API work. Existing
prepaint supplies current bounds but forbids layout/input mutation; updating
scroll state alone does not reposition already-laid-out text/caret or its tracked
IME geometry. In particular, `cui_input_paint` receives a stack-local temporary
state inside Styled InputGroup: a deferred callback must not retain that state
pointer. An architecture proposal must resolve that lifetime and geometry
ownership, not silently add an end-frame borrowing requirement to existing APIs.

Reproduction is in `tests/fixtures/input_grow_width/README.md`. Source metadata,
consumer-guidance regeneration and canonical registration are integrator work
under separate review. The guide should replace the old content-sized-viewport
warning with the narrower current-frame grow-caret/scroll limitation above. No
full Input parity, browser acceptance or general text-engine completion claimed.

Verified on this slice: strict-warning Release and ASan/UBSan builds each pass
`input_grow_width`, `input`, and `input_group_events` (3/3) through the private
Xvfb wrapper. Formatting and whitespace checks pass.
