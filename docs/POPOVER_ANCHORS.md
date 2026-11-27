# Popover source anchors

Semantics are audited against GPUI reference commit
`87d10ae5e1299d1be18670a37570c5402f952e34`, especially component Popover,
base Popup live anchor positioning and Positioner corner clamping.

`cui_popover_begin` is an additive C API for source-style Popover positioning and
arrows. Existing `cui_surface_begin` retains its legacy side placement, 6px gap,
opposite-side overflow selection and size reduction. Existing public state and
options structs have unchanged ABI. Recompile/link against the new library to
use the added symbols. Retained builders expose a subset through their own
options. See [Retained account menu](#retained-account-menu).

```c
cui_surface_options options = {
    .viewport = {0, 0, 640, 480},
    .anchor = {120, 80, 100, 32}, /* current-frame trigger rectangle */
    .width = 240,
    .height = 160,
    .margin = 4,
    .hide_close = true,
    .popover_appearance = CUI_POPOVER_APPEARANCE_SOURCE,
};
cui_popover_position position = {
    .mode = CUI_POPOVER_POSITION_ANCHOR,
    .anchor = CUI_POPOVER_TOP_LEFT,
    .reference_rem = 16,
    .arrow = true,
};
if (cui_popover_begin(ui, theme, state, cui_string("Details"),
                      (cui_str){0}, options, position)) {
    cui_text(ui, cui_string("Popover content"), theme->font_size, theme->text);
    cui_surface_end(ui);
}
```

Read `include/cui/cui_surface.h` for the complete contract. Initialize state with
`CUI_SURFACE_POPOVER` and open it with `cui_surface_open` as for existing surfaces.
Always balance a successful begin with `cui_surface_end`; a false begin has no
caller-owned boxes to close. Supply updated trigger bounds each submitted frame.

Anchor names identify the popup's own anchor: TopLeft/Center/Right open below the
trigger, BottomLeft/Center/Right open above it. LeftCenter opens to its right;
RightCenter opens to its left. Alignment follows the named trigger edge/corner.
Anchor mode clamps without flipping or shrinking the chosen dimensions. Width/
height <=0 select 420/260; invalid active geometry is rejected before overlay state
updates. `cui_popover_surface_geometry` exposes these defaults and placement as a
pure helper with unchanged output on failure.

`reference_rem` is a logical size from app/host root sizing configuration. Zero
selects 16. Never infer it from a theme font. Default offset is reference_rem/4;
arrow depth is reference_rem*3/8 and is added to surface distance. With
`custom_offset=true`, signed offset including 0 replaces the default gap. Finite
negative gaps may overlap the trigger. Clamping may alter the apparent gap. Source
logical placement retains fractional coordinates; host pixel snapping is separate.

Arrow base inset uses the theme radius, even if the surface radius is refined.
Fill follows the effective refined background. Appearance NONE without explicit
background refinement uses theme popover for the arrow. The 1px outline uses theme
text at alpha 26/255 and is absent for NONE. Arrow triangle and ring seam patch
paint after panel contents at its layer, inherit panel opacity and clip to both
the supplied viewport and host viewport. Decoration does not enlarge the panel
hit/dismiss bounds, create semantics or receive focus. It remains visible with
an inert ancestor and while another modal owns input. A separate inert paint
portal uses local prepaint so visual painting does not require input eligibility.

Geometry and vertices are copied into context-owned frame storage; do not retain
draw-list pointers across the next begin-frame. Arrow payload/paint-command
capacity failure balances internal boxes and
suppresses the final draw list. Ordinary layout-node capacity follows Core
semantics: failure remains balanced but ordinary paint may be partial. Retry on
a later adequately sized frame.

The portable outline uses a bounded 2px miter and triangle meshes. GPUI's external
PathBuilder tessellation/raster AA is not reproduced exactly. Source fill, anchor,
tip and seam equations do not establish exact pixel parity. Software, SDL_GPU and
Sokol support Core mesh commands. SDL3 now supports bounded analytic meshes
through prepared CPU coverage textures; see `design/SDL3_MESH.md` for its limits.
The browser Canvas adapter has exact headless mesh-mask and opacity evidence at
DPR 1 and 2. The gallery additionally tests all eight anchors, signed offsets,
clamping, opacity, inert paint and modal ordering with desktop and touch input.
Physical GPU hardware checks remain separate from Xvfb/software validation.
Popover transitions remain separate work.

## Keyboard focus and the trigger

On trigger activation, explicitly move focus to an enabled, focusable child
inside a successful begin. Choose a stable ID and submit the child in the same
frame. Move focus only on the opening action so later frames respect the user's
navigation. Nonmodal Popovers keep ordinary Tab order. Subject to the decision
policy, Escape cancels the focused surface and restores its enabled trigger.
To let another control keep focus and receive its click when it dismisses a
nonmodal Popover, choose `CUI_POPOVER_OUTSIDE_FOCUS_KEEP_POINTER`.

```c
/* card is initialized with CUI_SURFACE_POPOVER and retained between frames. */
bool opening = cui_button(ui, trigger, theme, cui_string("Open account"), false);
if (opening)
    cui_surface_open(ui, card, trigger);

bool admitted = cui_popover_begin_with_outside_focus(
    ui, theme, card, cui_string("Account"), (cui_str){0}, options, position,
    CUI_POPOVER_OUTSIDE_FOCUS_KEEP_POINTER);
if (admitted) {
    if (opening)
        cui_focus(ui, profile);
    if (cui_button(ui, profile, theme, cui_string("View profile"), false))
        cui_surface_close(ui, card, CUI_SURFACE_ACCEPTED);
    cui_surface_end(ui);
}
cui_popover_link_trigger(ui, card, trigger, admitted);
```

The Base equivalent is `cui_base_surface_begin_with_outside_focus`. The policy
applies to accepted outside primary presses only. The close callback sees the
focus Core chose before submission. Use `cui_popover_begin` or explicitly pass
`CUI_POPOVER_OUTSIDE_FOCUS_RESTORE_TRIGGER` for default trigger restoration.
See [Surfaces](SURFACES.md) for close ordering and the modal behavior.

`cui_popover_link_trigger` is optional. Call it after the trigger and the
optional panel, passing the current begin result. On frames that omit the
panel pass false. An admitted open panel exposes expanded and controls on
its actual trigger. Closing from a child or omitting the panel clears both
details in the same frame. Other semantic details are preserved. The helper
uses current submissions and leaves state, input and callbacks to the existing
surface and child APIs. It works with Base or Styled composition.

The gallery projects the expanded state and the live controlled dialog into
its browser accessibility tree. Native screen-reader acceptance and foreign
builder authoring remain open. Core's current expanded boolean alone does
not identify an explicit collapsed state for all native adapters.

## Right-click trigger

After submitting an enabled trigger in its scope, call
`cui_popover_take_secondary_press` with its stable ID. It uses the same routed
target as other CUI input, including the last layout's clipping and occlusion.
Matching secondary downs are consumed and coalesced into one true for the
frame. A true result reports input only. Choose how to change your model. For
example, open a closed card and request a decision-aware close of an open one
before submitting it:

```c
/* The caller submitted trigger earlier this frame. */
if (cui_popover_take_secondary_press(ui, trigger)) {
    if (card->overlay.open)
        cui_surface_request_close(ui, card, CUI_SURFACE_CLOSED);
    else
        cui_surface_open(ui, card, trigger);
}
```

Keep keyboard and accessibility activation when changing the mouse gesture.
The gallery uses the typed Button click observer to accept those activations
and ordinary touch taps. Its accessibility button submits a CUI accessibility
activation. An open card lets its enabled trigger consume touch down so the
subsequent accepted tap can toggle it. In right-click mode a primary mouse click can still
focus the trigger and dismiss an open card through the usual outside policy.
The card stays anchored to the trigger. Explicitly focus a child on opening
and link the trigger after panel submission as in the previous example.

Host context-menu policy is independent of event consumption. The Canvas
gallery uses its existing canvas context-menu policy. Custom hosts provide
their own pointer mapping, including any secondary pen input. Touch long
press is outside this contract.

## Browser gallery

The Popover story starts with its existing legacy account card. Select **Source
anchors (opt in)** to use anchor placement and a real centered CUI trigger.
In either mode, open the card and click or tap **Show previews** to dismiss it
while making a change in the control outside it.
Choose **Right-click, tap, or keyboard** in **Trigger** to try the secondary
press toggle. The displayed C11 example follows that selection.
All eight names identify the popup's own anchor. Controls map directly to
`mode`, `anchor`, `custom_offset`, signed scalar `offset`, `arrow` and
`reference_rem`. The latter is explicit application/host root sizing: zero means
16 logical pixels; the sample also offers 16, 24 and 32. It is never inferred
from the theme font. Inactive controls are disabled in legacy mode.

Edge clamp moves the actual trigger to the lower corner. The gallery schedules
one settling frame when the newly laid-out trigger differs from the bounds used
for composition, then returns to idle. It retains popup size and anchor direction.
Opacity and inert controls wrap the real stage. Open the card first to observe
paint persisting under an inert ancestor while child semantics/input disappear.
The modal demonstration uses a separate ordinary dialog at explicit layer 200,
above the card's default layer; it does not implement automatic overlay ordering.
Closing that dialog updates its control. Route changes close both surfaces and
reset the demonstration controls. The complete C11 snippet reflects the current
positioning and stage settings.

`tests/web/test_popover_anchor_gallery.py` drives actual mouse/CDP touch input,
checks all eight source equations against real CUI bounds and mesh vertices,
compiles the displayed snippets with strict C11 warnings, and reads actual Canvas
pixels for centered arrow joins, opacity, inert persistence and modal backdrop
ordering. Fractional clamped Canvas rectangle edges can still antialias the join
differently from mesh pixel-center coverage: a dark fill value 10 can read 9 at
an adjacent join pixel. Clamped geometry is checked separately; exact centered
join assertions retain exact values. This known raster difference is not claimed
as GPUI pixel parity. Browser keyboard opening, Escape, and mouse/touch outside
dismissal cover the focus paths above. Native focus and accessibility depend on
the host.

The browser gallery keeps the preview toolbar's scrollbar controls in place
through a captured gesture. A touch press can close an overflowing Popover
before release. Deferring the toolbar reflow preserves the canvas position
so the same tap can activate the control underneath. The mobile regression
renders the closed Popover between touch-down and touch-up.

## Retained account menu

A retained Popover accepts child controls. Anchor it to a live node, and open it
from that control's activation handler. The application chooses whether and
when to open the menu. An empty options record opts into anchored placement.
Python, JavaScript and TypeScript use `popover_options`. Go uses
`Node.PopoverOptions`. Rust uses `App::popover_options`. Older providers keep the
original `anchor`, `open` and close event. Check support before opting in.

```python
from cui_bridge import App, button, column, popover

account = button("account").label("Account")
close = button("close").label("Close account menu")
menu = popover("account-menu").anchor(account).children(close)

with App() as app:
    if app.supports_popover_options():
        menu.popover_options(anchor=0, width=220, height=96, margin=8,
                             hide_close=True, focus_content=True)
    account.on_click(lambda event: menu.open())
    close.on_click(lambda event: menu.open(False))
    app.mount(column().children(account, menu))
    app.step()
```

Python and JS/TS use anchor numbers 0–7 in the order TopLeft, TopCenter,
TopRight, BottomLeft, BottomCenter, BottomRight, LeftCenter and RightCenter.
They name the facing edge of the popup. A top anchor opens below the trigger
when space permits. Go and Rust provide named anchors. Options include width,
height, viewport margin, arrow, close-button visibility, and Escape and
outside-pointer dismissal. A signed custom offset replaces the default gap of
one quarter of the reference rem. An `offset` property enables the custom value
in Python and JS/TS, including zero. Set `CustomOffset` in Go or
`custom_offset` in Rust. Clear options with `None`, `null` or `nil` as
appropriate. The C bridge exposes the same subset through
`cui_bridge_popover_configure_v1` in `cui/cui_bridge.h`.

`focus_content` moves focus to the first eligible Tab stop in the body when the
open panel first appears. `keep_pointer_focus` preserves the new target's focus on
an accepted outside press. Escape and explicit close restore the trigger.
The normal queued close event still reports accepted dismissals. Removing
the node restores the trigger if the body owns focus and drops the panel
without a close event. Retained options use the Popover theme colors and link the trigger while the panel is
admitted. Arrow paint requires a compatible renderer.

Placement uses the trigger's most recently completed layout. Showing or
moving an anchor can require a follow-up frame. A hidden anchor can temporarily
leave the panel logically open with no rendered bounds. The application can
still close it explicitly. Arbitrary part styling and transitions remain open.

### Right-click in retained builders

Enable the secondary trigger on an anchored Popover. The anchor must have
an enabled interactive semantic in the current tree and match the routed
secondary down from the last completed layout. Hidden triggers and triggers
behind a modal are ignored. The allowed press toggles once per step. If two
eligible Popovers share an anchor, the first in retained node-slot order handles
it. Use one automatic Popover per trigger for predictable results.

The bridge checks the complete tree, so the anchor can appear before or after
the Popover. An accepted press schedules a redraw. On opening, the logical
open flag becomes true at the end of that step and the next step paints the
panel. A closing press requests the usual deferred close; the next step
queues one `CLOSE` with the closed result and restores trigger focus. A close
already being processed takes precedence. The automatic trigger does not
queue a Button click. Keep the normal trigger callback for primary input,
keyboard, accessibility activation and tap. Host context-menu policy is separate.
Touch long press is outside this API.

| Consumer | Option | Capability probe | Synthetic input |
| --- | --- | --- | --- |
| C bridge | `CUI_BRIDGE_POPOVER_SECONDARY_TRIGGER_V1` in the copied flags | `cui_bridge_queue_pointer_button_v1` present | `cui_bridge_queue_pointer_button_v1(app, phase, CUI_BRIDGE_POINTER_SECONDARY, x, y)` |
| Python | `secondary_trigger=True` | `app.supports_popover_secondary_trigger()` | `app.queue_pointer(phase, x, y, button="secondary")` |
| JavaScript / TypeScript | `secondary_trigger: true` | `app.supports_popover_secondary_trigger()` | `app.pointer(phase, x, y, 'secondary')` |
| Go | `SecondaryTrigger: true` | `app.SupportsPopoverSecondaryTrigger()` | `app.QueuePointerButton(phase, cui.PointerSecondary, x, y)` |
| Rust | `secondary_trigger: true` | `app.supports_popover_secondary_trigger()` | `app.pointer_button(phase, PointerButton::Secondary, x, y)` |

SDL already maps the right mouse button to secondary input. The private
Linux Xvfb test injects SDL right-button events into an initially hidden
software-rendered window. For synthetic tests using the bridge entry point,
send down and up at the trigger center, then run the input step and one
more step for presentation. The legacy pointer methods still send primary
input. Older providers can still use the account callback; probe before
enabling the secondary option.

The following account menu examples exercise right-click when supported and
primary activation on older providers. Both paths close with Escape. From a
repo checkout with a current bridge, run:

```sh
CUI_BRIDGE_LIBRARY="$PWD/build/libcui_bridge.so" PYTHONPATH=bindings/python python bindings/python/examples/account_menu.py
node bindings/node/examples/account-menu.cjs
node bindings/typescript/examples/account_menu.ts
(cd bindings/go && go run ./examples/account_menu)
cargo run --manifest-path bindings/rust/Cargo.toml --example account_menu
```

Use [Binding builders](BINDING_BUILDERS.md) to build the Node addon and select the
platform's bridge library. The TypeScript command uses a Node version with
built-in type stripping.
