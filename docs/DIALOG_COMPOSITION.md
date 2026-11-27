# Dialog and AlertDialog composition

Compose Dialog and AlertDialog with six independent slots. Styled C Root can
mount both automatically. The retained C bridge and the Python, JavaScript,
TypeScript, Go and Rust builders expose the same slots. The immediate C API
offers a synchronous decision callback. The retained APIs deliver queued Button
clicks and dialog close results.

Use headers, sources, SDK and generated resources from one matching revision. See
the [beta roadmap](../BETA_VERSION_ROADMAP.md#active-work-and-acceptance-gaps) for
release and platform validation.

## API and ownership

The authoritative declarations are
[`cui_dialog_composition.h`](../include/cui/cui_dialog_composition.h) and
[`cui_dialog_composition_styled.h`](../include/cui/cui_dialog_composition_styled.h).
The latter adds the Styled action helper and includes standard component types.

| Operation | Base | Styled |
| --- | --- | --- |
| Initialize caller storage | `cui_dialog_layout_init_v1` | Same |
| Open/reset a closed owner | `cui_dialog_open_v1` | Same |
| Resolve default sizing | `cui_dialog_sizing_default_v1` | Same |
| Submit complete tree | `cui_base_dialog_compose_v1` | `cui_dialog_compose_v1` |
| Submit owner action | `cui_base_dialog_action_v1` | `cui_dialog_action_v1` |
| Inspect resolved slot style | `cui_dialog_slot_style_v1_get` | Same |
| Balanced child wrapper | `cui_dialog_slot_begin_v1` / `cui_dialog_slot_end_v1` | Same |

Retain one `cui_surface_state` and one `cui_dialog_layout_v1` for each Dialog or
AlertDialog. Initialize the surface with `CUI_SURFACE_DIALOG` or
`CUI_SURFACE_ALERT`, then initialize its layout. Use `cui_dialog_open_v1` for every
closed-to-open transition, including close/reopen between frames. Already-open
calls do not reset scroll. Existing `cui_surface_open` behavior is unchanged;
it does not perform the new layout reset protocol.

Call compose between `cui_begin_frame` and `cui_end_frame`. Compose owns all
structural begin/end calls, focus scope and callbacks. There is no new manual
top-level begin/end route. Submit a layout at most once per ordinary frame; do
not recursively compose the same layout. Distinct nested owners are allowed.

The surface, layout and their borrowed offset/bounds storage must remain alive
through `cui_end_frame`. Do not edit private bookkeeping fields. The layout binds
to its context; reinitialize only while unmounted before rebinding. Descriptor,
slot-user and refinement pointers are read during compose. Emitted strings,
images and other resources retain their ordinary Core frame/draw lifetimes.
A callback token is valid only inside its active callback; never store it.

## Six independent slots

Callbacks run once in enum order, while the owner remains open. Core may measure
text repeatedly; it does not replay application composition.

| Slot | Default when no `present` or `omit` bit | Layout role |
| --- | --- | --- |
| `CUI_DIALOG_HEADER` | Absent | Intrinsic region, independent of Title |
| `CUI_DIALOG_TITLE` | Wrapped `name` when nonempty | Intrinsic label region |
| `CUI_DIALOG_DESCRIPTION` | Wrapped `description` when nonempty | Intrinsic region |
| `CUI_DIALOG_CONTENT` | Absent | FLEX1 with intrinsic minimum |
| `CUI_DIALOG_BODY` | Absent | FLEX1 with zero minimum; scrolling children |
| `CUI_DIALOG_FOOTER` | Absent | Separate intrinsic row outside Inner clip |

Set a slot's `present` bit to invoke its callback. `present` plus a NULL callback
means authored empty, including an empty Content/Body allocation. `omit` suppresses
the slot and string fallback; masks cannot overlap or contain unknown bits. An
omitted Body has no Body callback, scroll semantics or scrollbar. Header never
suppresses Title or Description. To compose Description inside a custom Header,
set the independent Description's `omit` bit and use a balanced manual child
wrapper inside Header. Manual wrappers cannot span callbacks.

`name` and `description` supply explicit accessibility text. Arbitrary visual
callback children do not replace those strings. The composition links the owner to its
Title label only when the Title wrapper is admitted. Caller-created child IDs
must be distinct from structural IDs; use stable IDs from the owning context.

## Sizing, styling and placement

Recompute `cui_dialog_sizing_default_v1` from the current viewport each frame,
then apply explicit overrides. In this route `sizing` alone owns Panel width and
height; `cui_surface_options.width/height` are inactive. Defaults are width420,
AUTO intrinsic height, minimum96, viewport-bounded finite maximum, and
`CUI_DIALOG_SOURCE_TOP`. The source96 minimum wins over a smaller usable maximum;
the default resolver normalizes the effective maximum accordingly. Nonpositive
usable width or height is rejected; do not silently skip failed submission.

`CUI_DIALOG_CENTERED` selects current-size centering. Source-top defaults to
viewport-height/10 and can take explicit preferred-top/layer-offset fields. The
submitted current tree determines placement; previous completed bounds are not
used to paint or allocate current geometry.

Content and Body have separate `cui_scroll_region_sizing_v1` policies: FLEX1,
FIT or FIXED, explicit minimum policy and finite bounds. Preserve their distinct
default minima. Content can exceed its Inner allocation and be clipped while
Footer retains its independent position. An oversized Footer can overflow the
bounded Panel; this source policy does not convert Outer into a scroll owner.
Body alone scrolls. The two FLEX1 peers use equal zero-basis allocation after
minimum freezing. AUTO sizes the Panel from their combined intrinsic demand;
it does not promise each peer receives its own natural height. For example, a
short status Content can grow while a taller Body scrolls. This is the reviewed
source policy, not stale first-frame geometry. For a status region that should
stay intrinsic, explicitly set `sizing.content_sizing.mode = CUI_SCROLL_REGION_FIT`
after resolving defaults, leaving Body FLEX1. The gallery Dialog uses this
application policy: status may wrap at narrow widths, short Body fits, and long
Body still scrolls. Do not change the global FLEX1 default to obtain that choice.

This is a bounded Y constraint route, not a general layout
solver: widths must remain independent of allocated Y. See
[`SCROLL_LAYOUT.md`](SCROLL_LAYOUT.md) for admission and failed-frame behavior.

Use `cui_dialog_refinement_v1` to override masked Panel background, border,
radius, padding, gap and border width, or each slot's foreground, font size,
padding, gap and alignment. Zero reserved fields; initialize `struct_size` and
`version` with `sizeof` and `CUI_DIALOG_COMPOSITION_VERSION_1`. Unknown fields and
invalid active floats fail validation. Base starts with neutral styles; Styled
resolves theme values before explicit refinements. Supply the same theme to
Styled compose and Styled actions.

Core layout record V2 is unrelated to draw ABI2/background negotiation. This
route uses ordinary V1 drawing and real track/thumb nodes; it does not require
another renderer ABI. Body scrolling keeps Footer fixed. Wheel scrolling
remains available with hidden bars.

## Actions, timing and failure

Base action requires a caller-submitted box with the same ID. Styled action
submits the standard button. Return values report the underlying interaction or
button activation; **an activation can be vetoed** and need not mean the owner
closed. Standard Styled button callbacks run before the owner action reducer.
CONFIRM/CANCEL/CLOSE map to ACCEPTED/CANCELLED/CLOSED. The existing synchronous
`decide` callback may veto; accepted close restores focus and calls `on_close`
through existing surface behavior. No queued veto or global Enter mapping is
introduced. `cui_surface_request_close` remains deferred to the next ordinary
update; it is not an immediate between-frame close callback.

Slot/action/decide/on_close callbacks may close the owner through supported
helpers. Stop emitting application children after that close. They must not free
or reinitialize the active layout/surface/context, or invalidate borrowed strings
or offset storage. Remove owned storage only after `cui_end_frame`. Earlier
callback effects are not rolled back when a later callback closes or fails.

The new route uses `surface.body_viewport`, `body_content` and `body_maximum`
only as admitted previous-input caches, refreshed before ordinary input
reduction and cleared when not admitted. Current completed ranges come from the
Body semantic after successful end_frame; current rectangles come from
`layout.completed`. These state cache fields are not a current-paint oracle.

Input uses the previous valid completed snapshot. Composition admits previous Body
geometry only for an adjacent successful submission in the same open lifetime.
When there is no such snapshot, composition calls
`cui_discard_previous_scope_input` before invoking slot callbacks. It consumes
events routed to the previous owner or its nested scopes and cancels their
pending activations separately. It releases their held mouse and key captures
and blocks their in-flight touch contacts until lift. The first composed frame
still paints enabled controls and restores normal focus. Subsequent events use
the newly published layout. Use `cui_dialog_open_v1` for each new lifetime.

This applies to C Base and Styled Dialog and AlertDialog, including caller-authored
controls. Core keeps event targets available for inspection. Application-owned
gesture models remain the application's responsibility. Use the
[Root mounting API](ROOT.md#mount-a-composed-dialog-or-alertdialog) to submit
this composition from the window's Root. The retained APIs are described below.
Native window-service integration remains open.

Inspect both `cui_dialog_submit_result` and the error after `cui_end_frame`.
`valid=false` rejects a descriptor or reports a later submission failure;
`submitted=false` can also mean the owner is closed. An early valid result does
not guarantee the later Core constraint calculation succeeds. An errored opt-in
frame has no valid current geometry/draw promise; do not present its draw list.
Clear/correct errors according to Core recovery, then submit a new frame. Never
retry compose in the same ordinary frame or assume callbacks were rolled back.
`layout.completed[10]` is meaningful only after successful end_frame while the
owner remains open: Panel, Header, Title, Description, Content, BodyViewport,
BodyContent, Footer, Outer, Column. These are borrowed tracking destinations,
not an input cache to be copied back into current paint geometry.

Present-part rectangles remain retained across ordinary submissions so unchanged
tracked bounds do not request another frame. Omitted parts clear their rectangles;
a new open lifetime resets all retained rectangles. A successful stable Dialog
can quiesce after input and bounded animations settle. Auto-hide fading may request
frames while active; focused scrollbar visibility does not itself require an
endless redraw loop. Failed opt-in frames still provide no valid current geometry.

## Complete source example

This small native headless program composes its first open frame, renders to a
software buffer, then tears down after end_frame. The extracted example passed
a strict C11 build and a private-Xvfb headless Inter Regular/Bold run. A host application calls the
frame code repeatedly and queues its real input before begin_frame. Pass actual
regular and semibold font paths; the program does not download fonts or open a
window. The example uses Header, Title and Description independently, Content,
Body and Footer. Static text outlives the frame.

```c
#include "cui/cui_dialog_composition_styled.h"
#include "cui/cui_software.h"
#include "cui/cui_text.h"
#include <stdio.h>

typedef struct {
    cui_surface_state surface;
    cui_dialog_layout_v1 layout;
    cui_theme theme;
    unsigned confirmations;
    bool closed;
} demo;

static bool decide(void *user, cui_surface_result result) {
    demo *d = user;
    if (result == CUI_SURFACE_ACCEPTED)
        ++d->confirmations;
    return true; /* Return false here to veto synchronously. */
}
static void on_close(void *user, cui_surface_result result) {
    (void)result;
    ((demo *)user)->closed = true; /* No storage removal here. */
}
static void header(cui_context *ui, const cui_dialog_frame *frame, void *user) {
    (void)frame;
    demo *d = user;
    cui_label(ui, &d->theme, cui_string("Workspace settings"));
}
static void content(cui_context *ui, const cui_dialog_frame *frame, void *user) {
    (void)frame;
    demo *d = user;
    cui_label(ui, &d->theme, cui_string("Changes apply to this workspace."));
}
static void body(cui_context *ui, const cui_dialog_frame *frame, void *user) {
    (void)user;
    cui_dialog_slot_style_v1 style;
    if (!cui_dialog_slot_style_v1_get(frame, CUI_DIALOG_BODY, &style))
        return;
    (void)cui_text_paragraph_colored(
        ui, cui_string("This body wraps at the current width. Longer content "
                       "scrolls without placing the independent Footer inside it."),
        style.font_size, style.foreground, style.font_size * 1.25f,
        (cui_text_style){0}, true, NULL, 0);
}
static void footer(cui_context *ui, const cui_dialog_frame *frame, void *user) {
    demo *d = user;
    (void)cui_dialog_action_v1(
        ui, frame, &d->theme, cui_id_from(ui, "demo-cancel"), cui_string("Cancel"),
        CUI_DIALOG_CANCEL, (cui_button_options){0});
    if (!d->surface.overlay.open)
        return;
    (void)cui_dialog_action_v1(
        ui, frame, &d->theme, cui_id_from(ui, "demo-confirm"), cui_string("Save"),
        CUI_DIALOG_CONFIRM, (cui_button_options){0});
}
int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s regular-font semibold-font\n", argv[0]);
        return 2;
    }
    int status = 1;
    cui_font *font = cui_font_open(argv[1]);
    if (!font)
        return 1;
    if (!cui_font_register_style(
            font, (cui_text_style){.weight = CUI_TEXT_WEIGHT_SEMIBOLD}, argv[2])) {
        cui_font_close(font);
        return 1;
    }
    cui_context *ui = cui_create((cui_config){
        .width = 640, .height = 480,
        .measure = cui_font_measure, .measure_styled = cui_font_measure_styled,
        .measure_user = font, .font_metrics = cui_font_get_metrics,
        .text_geometry = cui_font_text_geometry,
        .text_geometry_line = cui_font_text_geometry_line,
        .text_capabilities = cui_font_text_capabilities()});
    cui_software *renderer = ui ? cui_software_create(640, 480, font) : NULL;
    if (!ui || !renderer)
        goto cleanup;
    demo d = {.theme = cui_theme_light()};
    cui_surface_init(&d.surface, cui_id_from(ui, "demo-dialog"), CUI_SURFACE_DIALOG);
    if (!cui_dialog_layout_init_v1(&d.layout, &d.surface) ||
        !cui_dialog_open_v1(ui, &d.layout, 0))
        goto cleanup;
    cui_surface_options options = {
        .viewport = {0, 0, 640, 480}, .margin = 16,
        .decide = decide, .decision_user = &d,
        .on_close = on_close, .close_user = &d};
    cui_dialog_sizing_v1 sizing;
    if (!cui_dialog_sizing_default_v1(options, &sizing))
        goto cleanup;
    cui_dialog_composition_v1 composition = {
        .struct_size = sizeof composition,
        .version = CUI_DIALOG_COMPOSITION_VERSION_1,
        .present = (1u << CUI_DIALOG_HEADER) | (1u << CUI_DIALOG_CONTENT) |
                   (1u << CUI_DIALOG_BODY) | (1u << CUI_DIALOG_FOOTER),
        .name = {"Notification preferences", 24},
        .description = {"Choose how to stay informed.", 28},
        .slots = {
            [CUI_DIALOG_HEADER] = {header, &d},
            [CUI_DIALOG_CONTENT] = {content, &d},
            [CUI_DIALOG_BODY] = {body, &d},
            [CUI_DIALOG_FOOTER] = {footer, &d}}};
    if (!cui_begin_frame(ui, 640, 480, 0.016f))
        goto cleanup;
    cui_dialog_submit_result result =
        cui_dialog_compose_v1(ui, &d.theme, &d.layout, options, &sizing, &composition);
    cui_draw_list draw = cui_end_frame(ui); /* d and all borrowed data still live */
    if (result.valid && result.submitted && cui_last_error(ui) == CUI_OK &&
        cui_software_render(renderer, &draw, (cui_color){240, 240, 240, 255}))
        status = 0;
cleanup:
    cui_software_destroy(renderer);
    cui_destroy(ui);
    cui_font_close(font);
    return status;
}
```

For AlertDialog, initialize with `CUI_SURFACE_ALERT`, provide the desired Header
and Footer callbacks, and omit Body when there are no children. The legacy
`cui_alert_dialog` and surface begin/footer/end helpers remain available.

The intended source CMake consumer links `cui`, `cui_text` and `cui_software` from
the same checkout (native text requires the repository's pinned/configured
FreeType/HarfBuzz prerequisites). For example, after saving the program as
`dialog_demo.c`, use a separate consumer CMake project with:

```cmake
cmake_minimum_required(VERSION 3.20)
project(dialog_demo C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CUI_BUILD_NATIVE ON CACHE BOOL "" FORCE)
add_subdirectory("/absolute/path/to/matching/cui" cui)
if(NOT TARGET cui_text)
  message(FATAL_ERROR "Configure CUI native shaped-text prerequisites first")
endif()
add_executable(dialog_demo dialog_demo.c)
target_link_libraries(dialog_demo PRIVATE cui cui_text cui_software)
```

The extracted C example has run with real Inter fonts. The standalone consumer
CMake recipe and generated-header/SDK alternatives require their own validation. Do not combine this draft's header with an older installed
library. The source metadata/guide registration to keep synchronized before release is:
Dialog and AlertDialog entries in `docs/component-usage.json` and their matching
human Usage paragraphs; supplemental header authority and Base/Styled limitations;
consumer skill guidance; canonical SDK, amalgamation and skill generation; and
meaningful configured source/distribution/offline-resource checks. Keep all78
manifest entries. See the exact pending-gate receipt in
[`dialog-six-slot-source-checkpoint.json`](validation/dialog-six-slot-source-checkpoint.json).

## Compose a retained C dialog

Create a `CUI_BRIDGE_DIALOG` or `CUI_BRIDGE_ALERT_DIALOG`, then opt in with
`cui_bridge_dialog_configure_v1` while it is closed. The bridge copies configuration
and owns each slot and descendant until removal. Create slots in any order. Add
ordinary controls with `cui_bridge_node_create`, using the returned slot as the
parent. Dialog's ordinary direct children share the scrollable Body with a named
Body slot. Give the owner a text label and description for accessibility even
when supplying visual Title and Description children.

A custom Footer replaces AlertDialog's default Confirm and Cancel buttons.
Mark direct Footer Buttons with `CUI_BRIDGE_DIALOG_ACTION_CONFIRM_V1`,
`CUI_BRIDGE_DIALOG_ACTION_CANCEL_V1` or `CUI_BRIDGE_DIALOG_ACTION_CLOSE_V1`.
Drain the ordinary Button click and the owner's typed `CUI_BRIDGE_NOTIFY_LIFECYCLE`
from the same queue. Use the owner's `payload.surface_result` for the outcome
and perform each side effect once. For app-controlled validation, use an
ordinary Button and call `cui_bridge_set_open(app, dialog, 0)` after validation.
A programmatic close reports `CUI_BRIDGE_SURFACE_CLOSED` on the next submitted
step. Use the immediate C API when a synchronous decision callback is required.

This headless example creates an AlertDialog with a short Body and an authored
Footer. Save the program as `retained_dialog.c`.

```c
#include <assert.h>
#include <cui/cui_bridge.h>

int main(void) {
    cui_bridge_options options = {.struct_size = sizeof options,
                                  .abi_version = CUI_BRIDGE_ABI_VERSION,
                                  .backend = CUI_BRIDGE_BACKEND_NONE,
                                  .width = 640,
                                  .height = 400};
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root, alert, body, copy, footer, cancel, confirm;
    assert(cui_bridge_node_create(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN,
                                  "workspace", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_ALERT_DIALOG,
                                  "remove-account", &alert) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, alert, "Remove this account?") == CUI_BRIDGE_OK);
    cui_bridge_dialog_config_v1 layout = {.flags = CUI_BRIDGE_DIALOG_CENTERED_V1, .width = 360};
    assert(cui_bridge_dialog_configure_v1(app, alert, sizeof layout, &layout) == CUI_BRIDGE_OK);
    assert(cui_bridge_dialog_slot_create_v1(app, alert, CUI_BRIDGE_DIALOG_BODY_V1,
                                           "body", &body) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, body, CUI_BRIDGE_LABEL, "details", &copy) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, copy, "Local preferences will be removed.") == CUI_BRIDGE_OK);
    assert(cui_bridge_dialog_slot_create_v1(app, alert, CUI_BRIDGE_DIALOG_FOOTER_V1,
                                           "actions", &footer) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, footer, CUI_BRIDGE_BUTTON, "cancel", &cancel) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, cancel, "Keep account") == CUI_BRIDGE_OK);
    assert(cui_bridge_dialog_set_action_v1(app, cancel, CUI_BRIDGE_DIALOG_ACTION_CANCEL_V1) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, footer, CUI_BRIDGE_BUTTON, "confirm", &confirm) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, confirm, "Remove account") == CUI_BRIDGE_OK);
    assert(cui_bridge_dialog_set_action_v1(app, confirm, CUI_BRIDGE_DIALOG_ACTION_CONFIRM_V1) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_open(app, alert, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, 0.016f) == CUI_BRIDGE_OK);
    cui_bridge_rect panel;
    assert(cui_bridge_node_bounds(app, alert, &panel) == CUI_BRIDGE_OK && panel.width == 360);
    cui_bridge_destroy(app);
    return 0;
}
```

Run from the matching source checkout on Linux.

```sh
cmake -S . -B build -G Ninja -DCUI_BUILD_BRIDGE=ON -DCUI_BUILD_NATIVE=OFF
cmake --build build --target cui_bridge test_bridge_dialog_composition
cc -std=c11 -Wall -Wextra -Werror -Iinclude retained_dialog.c -Lbuild \
  -Wl,-rpath,"$PWD/build" -lcui_bridge -o build/retained_dialog
build/retained_dialog
python scripts/test_local.py --build-dir build -- -R '^bridge_dialog_composition$'
```

The test also exercises pointer and keyboard input, the fixed Footer while Body
scrolls, nested Escape and focus restoration, removal and the legacy helpers.
The retained functions are versioned symbols in `cui_bridge.h`.
Probe capability when loading an older bridge.

## Compose a dialog in a language builder

Call the capability probe if your app may load an older bridge. Configure the
modal before adding named slots. Connect typed Buttons directly to Footer and
do the application work from the owner close result. Use an ordinary Footer
Button when the app must validate before closing.

| Language | Layout and slots | Close action | Capability |
| --- | --- | --- | --- |
| Python, JavaScript, TypeScript | `dialog_options(...)`, `dialog_slot(key, role)` | `button(...).dialog_action('confirm')` | `app.supports_dialog_composition()` |
| Go | `.DialogOptions(&cui.DialogOptions{...})`, `cui.DialogSlot(key, role)` | `.DialogAction(cui.DialogActionConfirm)` | `app.SupportsDialogComposition()` |
| Rust | `.dialog_options(Some(options))`, `app.create_dialog_slot(dialog, role, key)` | `.dialog_action(DialogAction::Confirm)` | `app.supports_dialog_composition()` |

The `confirm`, `cancel` and `close` actions produce accepted, cancelled and
closed results. They keep any ordinary Button click handler. One event is
queued for the Button and one for the dialog. Immediate C supplies the
synchronous decision API. These retained options affect dialogs inside the
CUI surface and use its existing owner-thread lifecycle.

Each example opens an account dialog, submits a typed confirmation and checks
that the status UI changed. Build the matching portable bridge and Node addon
from the repository root on Linux.

```sh
cmake -S bindings/node -B build-node -G Ninja -DCUI_BUILD_NATIVE=OFF
cmake --build build-node --target cui_node_bindings
bridge_dir=$(dirname "$(find build-node -name libcui_bridge.so -print -quit)")
export CUI_BRIDGE_LIB_DIR="$PWD/$bridge_dir"
export CUI_BRIDGE_LIBRARY="$CUI_BRIDGE_LIB_DIR/libcui_bridge.so"
export CUI_NODE_ADDON="$PWD/$(find build-node -name cui.node -print -quit)"
PYTHONPATH=bindings/python python bindings/python/examples/dialog_composition.py
node bindings/node/examples/dialog-composition.cjs
node bindings/typescript/examples/dialog_composition.ts
go -C bindings/go run ./examples/dialog_composition
cargo run --manifest-path bindings/rust/Cargo.toml --example dialog_composition
```

The TypeScript command uses the Node runtime with type stripping. The repo's
CI also type-checks the example.

The Linux headless package checks run the examples from clean consumers of the
Python wheel, prebuilt npm tarball (JavaScript and TypeScript), embedded-bridge
and source-only Go modules, and Rust crate. These checks cover the local package
artifacts. Native desktop behavior and public publication are tracked separately.

To run a visible native UI, build the matching
SDL-enabled bridge and addon and pass `--font /path/to/font.ttf` to the Python
or Node examples, `-font /path/to/font.ttf` to Go, or `-- /path/to/font.ttf`
after Cargo's example arguments. See the [native setup guide](NATIVE_APP_STARTER.md)
for build prerequisites.
