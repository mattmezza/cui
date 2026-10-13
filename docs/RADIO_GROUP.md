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
ID after calling the base API. The reference's richer child composition, tooltip
builder, spring mark animation and unrestricted style refinement are not provided
by this initial group API. Full Radio-family source parity is not claimed.

The headless test asserts same-frame selection paint, semantic set relationships,
callback rejection/acceptance, selected no-op, keyboard wrap/disabled skipping,
queued navigation, one tab stop, touch drag/cancel suppression, focus cleanup,
bounded validation and all 14 design sizes at two densities. The browser suite
uses actual mouse, keyboard and CDP touch events on the gallery's visible options.
Chromium touch emulation is not evidence from a physical mobile device.
