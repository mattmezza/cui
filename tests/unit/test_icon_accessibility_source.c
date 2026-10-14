/* SPDX-License-Identifier: MIT */
#include "cui/cui_icon.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static const char svg[] = "<svg viewBox=\"0 0 24 24\"><title>Private SVG metadata</title>"
                          "<path d=\"M2 12h20\" stroke=\"currentColor\" stroke-width=\"2\"/></svg>";
static const cui_draw_command *bitmap(cui_draw_list d) {
    for (size_t i = 0; i < d.count; ++i)
        if (d.commands[i].type == CUI_DRAW_IMAGE)
            return &d.commands[i];
    assert(false);
    return NULL;
}
static unsigned long mask_hash(cui_bitmap b) {
    unsigned long hash = 5381;
    for (unsigned y = 0; y < b.height; ++y)
        for (unsigned x = 0; x < b.width * 4; ++x)
            hash = hash * 33 + b.pixels[y * b.stride + x];
    return hash;
}
static cui_draw_list frame(cui_context *ui, cui_theme *t, cui_icon_state *s, cui_icon_options o) {
    assert(cui_begin_frame(ui, 200, 100, .016f));
    assert(cui_icon(ui, 10, t, s, o) == CUI_ICON_OK);
    cui_draw_list d = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return d;
}
static void semantics(cui_context *ui, bool named, cui_str name) {
    size_t count = 0;
    const cui_semantic *a = cui_semantics(ui, &count);
    assert(count == (named ? 1u : 0u));
    if (named) {
        assert(a[0].id == 10 && a[0].role == CUI_ROLE_IMAGE && a[0].tab_stop_disabled);
        assert(a[0].name.length == name.length && !memcmp(a[0].name.data, name.data, name.length));
        assert(a[0].details.no_focus);
        cui_rect r = cui_bounds(ui, 10);
        assert(a[0].bounds.w == r.w && a[0].bounds.h == r.h);
    }
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 128});
    assert(ui);
    cui_theme t = cui_theme_light();
    t.font_size = 22;
    cui_icon_state s = {0};
    char owned[sizeof(svg)];
    memcpy(owned, svg, sizeof(svg));
    assert(cui_icon_set_data(&s, cui_string(owned)));
    memset(owned, '!', sizeof(owned));
    cui_icon_options o = {0};
    cui_draw_list d = frame(ui, &t, &s, o);
    assert(cui_bounds(ui, 10).w == 22 && bitmap(d)->bitmap.tint.r == t.text.r);
    semantics(ui, false, (cui_str){0});
    cui_str name = cui_string("Search records — 検索");
    o.accessible_name = name;
    o.custom_inherited_color = true;
    o.inherited_color = (cui_color){11, 33, 77, 255};
    d = frame(ui, &t, &s, o);
    semantics(ui, true, name);
    assert(bitmap(d)->bitmap.tint.b == 77);
    cui_focus(ui, 10);
    assert(cui_focused(ui) != 10);
    cui_rect hit = cui_bounds(ui, 10);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = hit.x + 1, .y = hit.y + 1}));
    frame(ui, &t, &s, o);
    assert(cui_focused(ui) != 10 && !cui_event_accepted_activation(ui, 0));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = hit.x + 1, .y = hit.y + 1}));
    frame(ui, &t, &s, o);
    assert(cui_focused(ui) != 10 && !cui_event_accepted_activation(ui, 0));
    o.custom_color = true;
    o.color = (cui_color){201, 44, 99, 255};
    o.size = CUI_ICON_LARGE;
    o.rem_size = 20;
    d = frame(ui, &t, &s, o);
    assert(cui_bounds(ui, 10).w == 30 && bitmap(d)->bitmap.tint.r == 201);
    o.decorative = true;
    o.accessible_name = (cui_str){NULL, 99};
    d = frame(ui, &t, &s, o);
    semantics(ui, false, (cui_str){0});
    assert(bitmap(d)->bitmap.tint.r == 201);
    o.size = CUI_ICON_INHERITED;
    o.custom_width = true;
    o.width = 40;
    o.custom_height = true;
    o.height = 12;
    cui_box style = {
        .background = {12, 13, 14, 255}, .border = {1, 2, 3, 255}, .border_width = 1, .radius = 3};
    o.style = &style;
    d = frame(ui, &t, &s, o);
    assert(cui_bounds(ui, 10).w == 40 && cui_bounds(ui, 10).h == 12);
    bool background = false;
    for (size_t i = 0; i < d.count; ++i)
        if (d.commands[i].type == CUI_DRAW_RECT && d.commands[i].color.r == 12)
            background = true;
    assert(background);
    o.size = CUI_ICON_CUSTOM;
    o.custom_size = 18;
    frame(ui, &t, &s, o);
    assert(cui_bounds(ui, 10).w == 18);
    o.style = NULL;
    o.decorative = false;
    o.accessible_name = (cui_str){NULL, 1};
    assert(cui_begin_frame(ui, 200, 100, .016f));
    size_t parses = s.parse_count, rasters = s.raster_count;
    assert(cui_icon(ui, 10, &t, &s, o) == CUI_ICON_INVALID);
    assert(s.parse_count == parses && s.raster_count == rasters);
    size_t count = 0;
    cui_semantics(ui, &count);
    assert(count == 0);
    assert(cui_end_frame(ui).count == 0);
    o.accessible_name = name;
    o.rem_size = NAN;
    assert(cui_begin_frame(ui, 200, 100, .016f));
    assert(cui_icon_named(ui, 10, &t, cui_string("search"), o) == CUI_ICON_INVALID);
    assert(cui_end_frame(ui).count == 0);
    o.rem_size = 16;
    o.size = CUI_ICON_MEDIUM;
    d = frame(ui, &t, &s, o);
    cui_bitmap old = bitmap(d)->bitmap.bitmap;
    unsigned long original_hash = mask_hash(old);
    assert(cui_icon_set_empty(&s));
    assert(mask_hash(old) ==
           original_hash); /* Earlier immutable mask survives source replacement. */
    frame(ui, &t, &s, o);
    semantics(ui, true, name);
    cui_icon_state_collect(&s);
    assert(cui_icon_set_name(&s, cui_string("search"), NULL, NULL));
    frame(ui, &t, &s, o);
    semantics(ui, true, name);
    assert(cui_icon_set_data(&s, cui_string(svg)));
    frame(ui, &t, &s, o);
    semantics(ui, true, name);
    o.accessible_name = name;
    assert(cui_begin_frame(ui, 200, 100, .016f));
    assert(cui_icon_named(ui, 10, &t, cui_string("search"), o) == CUI_ICON_OK);
    cui_end_frame(ui);
    semantics(ui, true, name);
    o.decorative = true;
    assert(cui_begin_frame(ui, 200, 100, .016f));
    assert(cui_icon_named(ui, 10, &t, cui_string("icons/search.svg"), o) == CUI_ICON_OK);
    cui_end_frame(ui);
    semantics(ui, false, (cui_str){0});
    cui_destroy(ui);
    cui_icon_state_destroy(&s);
    return 0;
}
