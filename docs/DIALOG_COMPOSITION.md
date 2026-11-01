# Dialog composition V1 — focused C implementation

This guide describes the additive C implementation originating in source
checkpoint `138c3a39`, with the focused native proof and cache correction in
`db888055`. Strict C11 builds, native and WASM32 record checks, first-frame
geometry/current-font software tests, owner actions and the complete example
have been exercised locally. Configured distribution, canonical generated
freshness, gallery/browser and wider compatibility checks are tracked separately;
see the validation receipts before claiming a backend or full parity. This work is
unreleased; Root/host and all five foreign composition facades remain unfinished. Use
headers, sources, SDK and generated resources from one matching revision.

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
callback children do not replace those strings. The draft links the owner to its
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
another renderer ABI. Its draft scrollbar/input behavior still needs runtime
validation. Hidden bars do not imply disabling wheel scrolling.

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

Input uses the previous valid completed snapshot. The draft admits previous Body
geometry only for an adjacent successful submission in the same open lifetime.
It does not add an input-lifetime reset operation: **held pointer, touch or Space
capture can retain existing Core behavior across close/reopen**. No cancellation
marker, dispatcher rewrite or guarantee of suppressing every old held child
press is included. Root/native-host ownership, all five foreign facade builders
and stronger lifecycle closure remain separate unfinished work.

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
and Footer callbacks, and omit Body when there are no children. This route does
not introduce an automatic AlertDialog composition adapter. Existing
`cui_alert_dialog`, `cui_surface_begin`/footer/end and their measured-height or
fixed-height behavior stay unchanged.

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
