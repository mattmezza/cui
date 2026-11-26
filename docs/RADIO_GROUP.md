# RadioGroup

`cui_radio_group` supplies a bounded collection of up to 64 radio options. Each
item has a stable, unique, nonzero `cui_id`, a label and an optional disabled flag.
The application owns one selected ID (`0` means no selection) and persistent
`cui_radio_group_state`. Existing `cui_radio` and `cui_radio_ex` remain compatible.

```c
#include <cui/cui_radio_group.h>

typedef struct {
    cui_id selected;
    cui_radio_group_state radio;
} settings;

static void accept_delivery(void *user, cui_id requested) {
    settings *model = user;
    model->selected = requested;
}

void delivery_settings(cui_context *ui, const cui_theme *theme, settings *model) {
    cui_radio_group_item items[] = {
        {cui_id_from(ui, "standard"), cui_string("Standard"), false},
        {cui_id_from(ui, "priority"), cui_string("Priority"), false},
        {cui_id_from(ui, "express"), cui_string("Express unavailable"), true}};
    cui_radio_group(ui, cui_id_from(ui, "delivery"), theme, items, 3,
        &model->selected, &model->radio,
        (cui_radio_group_options){.name = cui_string("Delivery options"),
            .vertical = true, .on_change = accept_delivery, .change_user = model});
}
```

With a callback, each validated request invokes it once; the group does not write
the selected model. Accept by updating the selected ID synchronously, or reject by
leaving it unchanged. Without a callback, the group updates the selected ID.
Activating the already selected option produces no callback. Requests are resolved
before any option is painted, so accepted and rejected state produce coherent
checked snapshots and selected marks in the same frame. Multiple queued navigation
events advance in order. A callback must not change the item array or destroy its
model during the call. Labels/items are borrowed for the synchronous frame build;
retain ordinary Core text lifetimes through rendering as required by the host.

Arrow keys move focus and request selection, skipping disabled items and wrapping.
Home and End choose the first and last enabled option. Enter/Space activate the
focused option. One enabled item is in the Tab sequence: the focused item while
focus is within the group, otherwise the selected enabled item or first enabled
item. `skip_tab` removes all group options from that sequence while pointer focus
and explicit keyboard focus remain available. Disabled groups have no tab stop and
reject activation. A removed or newly disabled focused item moves focus to an
enabled fallback, without inventing a selection callback. An empty/all-disabled
group clears its own focus. External focus is retained.

Selection IDs are stable across reorder. A removed selection stays caller-owned
and paints no selected option; a disabled selected option remains checked but
cannot activate. The application decides whether to clear or replace those model
values. This preserves controlled callback semantics during collection changes.
Invalid inputs (duplicate/zero IDs, more than 64 items, invalid sizes or gaps) return
`valid=false` before registering the collection. No heap allocation is performed.

The collection emits RADIOGROUP semantics with orientation/name/required/invalid;
children emit RADIO with parent, checked, disabled, position and set size. These
are portable snapshots, not a native OS accessibility bridge. Shared cursor and
interaction queries can be applied to stable item IDs after submission.

The component supports four legacy control sizes and all 14 design sizes,
including theme density. It uses the existing selection mark renderer and the
same indicator/font/theme colors as the standalone radio. The separate base API
registers/reduces behavior without styled layout; a custom renderer must submit
boxes with the same IDs. Do not register another interactive radio with the same
ID after calling the base API. Standalone Radio accepts decorative composition
through `cui_radio_options.content`. Integrated tooltips and the full styling
audit remain open.

## Add descriptions and custom content

`cui_radio_group_present_with_content` accepts one optional descriptor per
option. Use `description` for supporting text. The item label remains the
accessible name, and the supporting text is also the semantic description.
Use `max_width` to cap the option's text column in logical pixels. Leaving it
zero lets the parent layout determine the available space.

```c
const cui_radio_group_content content[] = {
    {.description = cui_string("Arrives in four business days"), .max_width = 205},
    {.description = cui_string("Arrives Wednesday with tracked delivery"), .max_width = 205}
};

cui_radio_group_present_with_content(
    ui, group_id, theme, items, 2, &selection, &navigation,
    (cui_radio_group_options){.vertical = true, .name = cui_string("Delivery options")},
    (cui_radio_group_presentation){.motion = &motion}, content, 2,
    (cui_selection_handler){0});
```

Supply `content` and exactly one descriptor per item, or pass `NULL, 0`. A
zeroed descriptor preserves ordinary label layout for that option. The
presenter copies the small descriptor array before synchronous selection
requests. Description bytes and callback userdata still follow ordinary
frame lifetimes. An optional `content` callback runs once per submitted row
after the selection request and observer have completed. Its subtree paints
and participates in layout while the row remains the only focus and
activation target. Submit decorative content such as styled labels or art
and put selection instructions in the accessible name or description.

Retained applications can attach copied description text to each direct
RadioGroup Item with the existing detail setter. The runnable delivery forms
use that path. [Retained descriptions](BRIDGE_COMPONENTS.md#describe-retained-radio-options)
The C bridge also supports opt-in children from a bounded set of decorative
nodes. [Retained decorative content](BRIDGE_COMPONENTS.md#compose-retained-radio-option-content)

## Animate the marks

For animated RadioGroup presentation, keep a zero-initialized
`cui_radio_group_motion` beside the selected ID and navigation state. Give each
group its own motion state. Pass it to `cui_radio_group_present` each frame.

```c
/* Keep these fields in the application model. Initialize them once. */
typedef struct {
    cui_id selected;
    cui_radio_group_state navigation;
    cui_radio_group_motion motion;
} delivery_choice;

double draw_delivery_choice(cui_context *ui, const cui_theme *theme,
                            delivery_choice *choice) {
    cui_radio_group_item items[] = {
        {cui_id_from(ui, "standard"), cui_string("Standard"), false},
        {cui_id_from(ui, "priority"), cui_string("Priority"), false}};
    cui_radio_group_event_result result = cui_radio_group_present(
        ui, cui_id_from(ui, "delivery"), theme, items, 2,
        &choice->selected, &choice->navigation,
        (cui_radio_group_options){.vertical = true, .name = cui_string("Delivery")},
        (cui_radio_group_presentation){.motion = &choice->motion},
        (cui_selection_handler){0});
    return result.legacy.valid ? choice->motion.next_wake_seconds : 0;
}
```

Use the returned deadline on the frame clock. Zero means the animation needs
no frame. The first submission snaps to the model. Later accepted selections
crossfade the outgoing and incoming marks and their circular fills. Selection callbacks,
keyboard focus and checked semantics reflect the new choice in that frame.
Reversing selection redirects the running spring. Motion follows stable item
IDs on reorder, and removed options release their slots. The theme's
`spring_control` and the context's reduced-motion preference govern the fade.

Use `presentation.parts` and `presentation.part_count` to combine animation
with [named indicator styling](NAMED_SELECTION_INDICATORS.md). Existing
`on_change` and typed selection observers preserve their synchronous order.
With a null motion pointer, the style has immediate marks.

### Standalone Radio

Use `cui_radio_present` when a standalone Radio needs an animated mark. Retain
one zero-initialized `cui_radio_motion` per option. The existing shared integer
and `bool` change result follow `cui_radio_ex`. For sibling choices, use
RadioGroup when you need coordinated paint, checked semantics and arrow navigation.

```c
/* Keep selected and priority_motion in your application model. */
bool changed = cui_radio_present(ui, cui_id_from(ui, "priority"), theme,
    cui_string("Priority"), &selected, 1, (cui_radio_options){0},
    (cui_radio_presentation){.motion = &priority_motion});
```

Schedule `priority_motion.next_wake_seconds` as you would for the group. The
context reduced-motion setting applies to both. A retained C app can opt in
with `cui_bridge_radio_set_motion_v1`. See [the bridge guide](BRIDGE_COMPONENTS.md#animate-retained-radio-selection-from-c).

The headless test asserts same-frame selection paint, semantic set relationships,
callback rejection/acceptance, selected no-op, keyboard wrap/disabled skipping,
queued navigation, one tab stop, touch drag/cancel suppression, focus cleanup,
bounded validation and all 14 design sizes at two densities. Motion tests check
stable mark geometry and state across reorder, removal and disabled options.
Chromium checks fading, reversal, reduced motion and that frames stop when the
fade settles. It also exercises mouse, keyboard and CDP touch on the visible options.
Chromium touch emulation is not evidence from a physical mobile device.
