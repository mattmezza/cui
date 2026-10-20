# Stepper navigation and validation checkpoint

Design only; production requires review. CUI baseline `9d491483`; reference
`87d10ae5e1299d1be18670a37570c5402f952e34` in `/tmp/cui-gpui-reference`.

## Frozen source contract

Authorities are `crates/component/src/stepper/{stepper,item,trigger}.rs`,
`crates/story/src/stories/stepper_story.rs` and `website/component/stepper.md`.
Stepper captures a controlled zero-based selected_index, enumerates its items,
and forwards an enabled item's click to on_click(index). Item disabled and
Stepper disabled suppress that trigger's click listener. Clicking the currently
selected item still invokes the listener. Selection is not changed inside the
component: story/application handlers assign their own selected index and ask
for a redraw. There is no component-owned validator, next/back state machine,
linear progress restriction, async validation or built-in veto protocol.
The website's multi-step form example also directly assigns the clicked index.

The trigger renders checked styling for indices <= selected index and the item
renders passed separators for indices < selected index. Items carry ListItem
role and position-in-set. Sizes, arbitrary child content, icons, centering and
separator styling are separate parity concerns. They are not proved merely by
adding navigation callbacks. No Stepper-specific source tests establishing a
validation engine were found in this frozen module.

## Existing CUI coverage and genuine gap

`include/cui/cui_numeric.h`, `src/base/cui_numeric.c` and
`src/components/cui_numeric.c` already implement arbitrary enabled-step
navigation, global/item disabled state, horizontal/vertical paint, explicit
caller-owned complete/error status and controlled-index clamping. Existing tests
in `test_numeric.c`, `test_controlled_siblings.c` and
`test_numeric_selection_events.c` cover index changes and selected paint.

`cui_stepper_events`/`cui_base_stepper_events` are aggregate applied-change
observers: legacy reduction mutates current first, then one callback reports
entry/final difference with UNKNOWN frame metadata. Normalization can report a
change; same-index clicks and requests returning to the entry index do not.
They cannot serve as source-like preselection validation requests without
changing an established contract. They remain unchanged.

The bounded missing behavior is therefore a controlled per-activation request
surface on which the application can perform its own validation, including
same-index requests and independent delivery for a batch. Calling this a new
built-in validator would invent upstream functionality.

## Proposed additive controlled request API

Use a new dedicated public header and implementation, leaving existing structs,
functions, reduction/clamping, statuses and observer timing untouched. Accept
selected index by value, not a mutable owner pointer. Register enabled item
semantics, capture accepted activations only after successful current submission,
then synchronously call an optional typed on_request callback once per accepted
item activation in input order. Return an independent request count even without
a handler. A request includes captured selected index, requested item index,
count and copied genuine accepted activation metadata. Same-index activation is
a request. Layout and paint use the captured selected index throughout the call.

The callback may validate unrelated application form state and assign its own
selected index for the next submission; this call does not reread that owner,
apply or veto a request, or claim navigation completed. All requests in a batch
refer to the same captured selection, matching a source render's controlled
value. Do not advertise a final-change event or fabricate activation evidence
for normalization. Validation failure simply leaves the caller's selected value
unchanged and may update caller-owned error status for a later frame.

Require structurally valid strings, nonzero ID, count <=100 and an in-range
selected index for nonempty arrays; empty input accepts index zero only. Invalid
arguments return invalid without mutating app state or dispatching. This bounded
new preflight is explicitly stricter than both legacy clamping and upstream's
unchecked high selected index; no exact equivalence is claimed for invalid input.
Copy handler/options at entry; handlers/payload are borrowed for the call. Do not
destroy the active UI, mutate the borrowed step array, recursively submit the
same component or unwind foreign exceptions from callbacks. No retained handler,
async validation queue, focus ownership change or new dispatch architecture.

The application must still submit the component each frame. Protected/frame-zero
work registers/paints as permitted by existing Core but emits no request. Disabled
or omitted/current-rejected items cannot produce accepted request evidence.
Respect Core's exact accepted-activation/consumption contract, not a blanket
claim that consuming any ordinary event revokes an accepted activation. Freeze
each request's evidence at acceptance before invoking the application callback;
later consumption must not rewrite already delivered evidence.

## Shared source ownership needed before implementation

Prefer a tiny private `cui_numeric_stepper_paint` seam extracted from the current
styled Stepper paint loop in `src/components/cui_numeric.c`. Both legacy and new
controlled request functions can reuse the same layout/status/paint code, while
new request registration/reduction lives entirely in new Stepper-specific files.
Legacy calls continue their existing Base reducer then this paint seam. The new
function registers semantics from the captured value and invokes the seam without
legacy mutation. All private names must be component-prefixed for amalgamation.
This needs explicit ownership approval for `src/components/cui_numeric.c`;
`components.c`, Button sources and Base numeric reducer are unnecessary.

Alternative: duplicate the small styled paint loop in the new implementation,
with a parity test. That avoids shared ownership but creates two sources of
layout/status behavior. A pure unstyled-only API would be a smaller first slice,
but would not offer the existing styled Stepper without duplicate registration.
No such workaround is silently selected in this checkpoint.

## Required proof and scope limits

Strict private C11/C++17 fixture and wrapper-based headless tests must cover
actual mouse/touch/keyboard/accessibility activation; same index; several
requests in one batch; validation refusal and later owner acceptance; callbacks
before paint with captured selection stable; no handler count; global/item
disabled; invalid selected/count/string pairs; omission; inert/layout-only;
current semantic rejection; precise cancellation/consumption behavior; copied
event lifetime; unchanged explicit statuses and legacy regression fixtures.
Default new paint should match legacy for identical valid captured selection
without pending input, in horizontal and vertical layouts.

Add a real compiling example that checks form data in its request callback and
updates application selection only on success. Root owns canonical CMake,
generated SDK/distribution, manifest and consumer-skill registration afterward.
Foreign binding exposure, source icons/children/size/separator parity, source
ListItem hierarchy and live AT announcements remain separately documented.
This slice may establish application-controlled navigation/validation requests;
it cannot establish full Stepper or platform parity.
