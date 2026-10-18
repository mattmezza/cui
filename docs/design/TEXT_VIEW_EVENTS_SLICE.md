# TextView events: existing typed callback decision

Design/source audit at `6251b9d6`; no production changes or parity promotion.
Compare [shared concepts](COMPONENT_EVENTS_CONTRACT.md) and the TextView row in
[the 78-family audit](COMPONENT_EVENTS_COVERAGE.md). Consistent concepts do not
require identical callback signatures.

## Decision

Retain the existing typed `on_link` and `on_reveal` interfaces as justified
activation/service exceptions. Adding a shared envelope alone provides no missing
user operation and would risk mislabelling secondary clicks, reveal routing or
selection as accepted activation/commit. Do not replace callbacks, introduce a
rejectable transaction, or silently change their timing. Selection observation,
if requested, is a separate value domain requiring its own reviewed design.

## Actual ownership and results

[`cui_text_view.h`](../../include/cui/cui_text_view.h) exposes caller-owned document
workspace, parsed spans/blocks/glyphs, selection offsets and gesture state, scroll,
reveal and animation state. Source/rendered buffers have independent capacities;
byte/node/list-depth limits are explicit. Parsing has typed errors and documents
atomic capacity/UTF-8 failure. `cui_text_view`'s bool means valid successful
submission/layout, **not** link dispatch, selection change or successful URL open.
`cui_text_view_selection_update`'s bool concerns shared-selection updates; it is not
a per-event observer count. Programmatic `select` and `reveal` are explicit helpers,
not fabricated input events. `cui_text_view_reveal` validates a rendered range and
sets pending reveal; it does not synchronously call `on_reveal`.

| Existing callback | Exact meaning and timing |
| --- | --- |
| `void on_link(cui_str url, const cui_event *, void *link_user)` | Synchronous link invocation with decoded URI and actual borrowed event. In `cui_text_view_link`, matching unconsumed events are scanned during link painting, after LINK semantics/absolute hit box registration and before remaining glyph paint. It takes precedence over `open_url`; event is consumed **after** it returns. Return void cannot reject/undo navigation policy. |
| `bool open_url(cui_str url, void *link_user)` | Optional host fallback only when no `on_link` and event's pointer-button field is PRIMARY. Return value is ignored; event is consumed after invocation even when false. It is not a confirmed navigation notification or portable browser/OS implementation. |
| `void on_reveal(cui_rect window_line, void *reveal_user)` | Host viewport request when a pending range is located and the view is **not** internally scrollable. Executed after layout/reduction and possible table horizontal adjustment, before opening/painting the main viewport. `reveal_pending` clears **after** callback; callback cannot enqueue a second reveal reliably by rewriting that state. Scrollable mode adjusts internal scroll and clears pending without this callback. No input event or accepted activation is implied. |

Source: [`cui_text_view.c`](../../src/components/cui_text_view.c), private link helper,
paint function and public submission; [`Base`](../../src/base/cui_text_view.c),
selection/copy/reveal helpers. Base parsing/layout helpers have no analogous
`on_link` callback: the link targets belong to Styled composition.

## Multiplicity, evidence and routing

The link helper is not an aggregate zero/one callback: it loops matching events.
Normal activation requires `interaction.activated` plus `cui_event_activation(i)`
matching the actual generated link ID. Secondary down remembers `secondary_link`;
secondary up on that same target dispatches independently of accepted activation.
Thus secondary link invocation is useful application context-menu/open policy,
not proof of a primary accepted activation. Input selection dragging can consume
events and prevent activation; children do not become root clicks.

URI groups are rendered in document/paint order, each scanning its matching frame
events; do not promise one global event-index ordering across different links.
The existing event pointer exposes actual type/modifiers/button but not the loop
index or link ID in its callback signature. Applications must not invent either by
comparing event structs. A future evidence extension would need capture at this
existing seam before callbacks, unique accepted snapshots only where genuine, and
raw indexed pointer evidence for secondary clicks. That is an optional diagnostic
capability, not a prerequisite to use existing callbacks honestly. No blanket
keyboard/a11y parity follows from LINK role alone; exercise actual public routing
before claiming those paths or host fallback support.

Selection is already applied to caller-owned `anchor`/`cursor` (and possibly
multiple registered shared-selection members). It is not rejectable. Clipboard
copy is a service operation. There is no C selection CHANGE callback or SUBMIT/
COMMIT/lifecycle event. A separate selection observer proposal must choose between
per-transition reducer seams and an explicit final pair comparison, include
shared-selection ownership, distinguish programmatic projection/reflow from input,
and retain old mutation/order/paint. Do not add it solely to make signatures uniform.

## Lifetimes, disabled work and callback mutation

Decoded URI lives in a fixed stack buffer inside the link helper and is borrowed
**only during each callback**, as the header states. Copy bytes into bounded
caller storage before return, checking capacity; do not retain `url.data` or assume
NUL-based length. `cui_text_view_decode_resource` provides a separate bounded copy
operation from source ranges. The borrowed `const cui_event *` is a frame view;
copying `cui_event` alone does not own any optional text pointer. Copy only needed
scalars, or explicitly bound/copy text separately. Revealed rect is passed by
value but may become stale after layout/scroll changes. Selected-text slices live
in workspace until text mutation; selection-copy helpers provide checked owned
output and distinguish Plain/Source/image slots.

State/workspaces, items/images, options' users and UI must survive submission and
renderer consumption. No edit guard protects arbitrary document mutation from
`on_link`/`on_reveal`: mutating/reparsing/rebinding/freeing active state, shared
selection members, workspace or context during callbacks is unsupported and may
invalidate pointers still used by painting. Unrelated flags and bounded copies are
safe intended uses; defer document replacement, removal and destruction until
outer return plus renderer completion. Do not unwind foreign exceptions or block
on networking in these synchronous callbacks.

Disabled selection reduction clears `selecting` and returns; viewport is submitted
inert so link target/activation lookup is suppressed there. Preserve cleanup rather
than interpreting absence of callbacks as readonly model ownership. Reveal handling
runs **before** that inert viewport, so `disabled` does not establish silence for
programmatic reveal service callbacks. Parent inert/layout behavior and shared
selection scope routing need actual-scope tests before broader promises; layout
measurement is not an input cause. Image loaders, highlighters, code/table action
renderers and font metrics remain resource/render policy, with their own lifetimes,
not invented component notifications.

## Existing evidence, binding omissions and bounded next work

[`test_text_view.c`](../../tests/unit/test_text_view.c) includes
`test_links_and_images` (primary/secondary pointer callbacks and drag suppression),
`test_decoded_resource_callbacks` (HTML URI entity decoding), inline image links,
selection/copy/shared-selection and table/reveal tests. `test_theme_scroll_and_reveal`
proves internal reveal scrolling. No `on_reveal` callback fixture was found: do not
promote that host branch from internal-scroll coverage. Existing link tests are
headless source/commands tests, not evidence of external URL opening.

[`bridge_render` TextView branch](../../src/bridge/cui_bridge.c) constructs default
options, forwards disabled/size/scrollable, compares selection pair and queues a
generic bridge change. It supplies neither link nor reveal callbacks nor URL host.
C bridge selection/read/copy APIs are distinct from C callback registration; Python,
JS/TS, Go and Rust factories do not imply foreign `on_link`/`on_reveal` delivery.
No new binding support is claimed.

Highest-value follow-up is documentation/testing of **existing** contracts: callback
URI copy survives later frame/reparse; exact primary/secondary multiplicity and
cross-link order; false `open_url` still consumes and on_link overrides it;
non-scrollable on_reveal coordinates/pending-clear timing versus scrollable silence;
disabled/inert/layout scopes; unrelated callback flags and deferred omission.
Use public key/a11y routing tests separately before claiming support. Any behavior
bug discovered needs its own approved production slice. No new API, dependency,
workspace, shared event-kind change or full-family status update is proposed here.
