/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
static const cui_str long_name = {"A long label that needs automatic truncation",
                                  sizeof "A long label that needs automatic truncation" - 1};
static unsigned clicks, slots;
static cui_measure_fn fixture_measure;
static void *fixture_measure_user;
static void clicked(void *user) {
    ++*(unsigned *)user;
}
static void slot(cui_context *ui, void *user) {
    ++slots;
    float n = *(float *)user;
    cui_begin_box(ui, 0, (cui_box){.width = n, .height = n, .background = {222, 20, 80, 255}});
    cui_end_box(ui);
}
static cui_draw_list scene(cui_context *ui, cui_theme *t, float parent, cui_str name,
                           cui_button_options o, bool inert, bool layout) {
    assert(cui_begin_frame(ui, 6000, 3000, 0));
    cui_begin_box(
        ui, 1, (cui_box){.width = parent, .height = 2000, .inert = inert, .layout_only = layout});
    cui_button_ex(ui, 10, t, name, o);
    cui_end_box(ui);
    return cui_end_frame(ui);
}
static void contained(cui_draw_list d, cui_rect box, bool expect_ellipsis) {
    unsigned text = 0, tails = 0;
    for (size_t i = 0; i < d.count; i++)
        if (d.commands[i].type == CUI_DRAW_TEXT) {
            const cui_draw_command *c = &d.commands[i];
            ++text;
            assert(c->bounds.x >= box.x - .01f && c->bounds.y >= box.y - .01f);
            assert(c->bounds.x + c->bounds.w <= box.x + box.w + .01f);
            assert(c->bounds.y + c->bounds.h <= box.y + box.h + .01f);
            if (c->text.length == 3 && !memcmp(c->text.data, "…", 3))
                ++tails;
        }
    assert(text && tails == (unsigned)expect_ellipsis);
}
static void full_name(cui_context *ui, cui_str expected) {
    size_t n;
    const cui_semantic *s = cui_semantics(ui, &n);
    for (size_t i = 0; i < n; i++)
        if (s[i].id == 10) {
            assert(s[i].name.length == expected.length &&
                   !memcmp(s[i].name.data, expected.data, expected.length));
            return;
        }
    assert(false);
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 128,
                                              .max_commands = 512,
                                              .frame_bytes = 65536,
                                              .measure = fixture_measure,
                                              .measure_user = fixture_measure_user});
    assert(ui);
    cui_theme theme = cui_theme_light();
    /* First-frame grow must shrink its label, not widen or wrap the control. */
    cui_draw_list initial =
        scene(ui, &theme, 80, long_name, (cui_button_options){.width = -1}, false, false);
    assert(fabsf(cui_bounds(ui, 10).w - 80) < .01f);
    contained(initial, cui_bounds(ui, 10), true);
    for (unsigned size = 0; size < 18; size++)
        for (unsigned density = 0; density < 2; density++)
            for (unsigned grow = 0; grow < 2; grow++) {
                theme = cui_theme_light();
                if (density)
                    theme.font_size = 14;
                cui_button_options o = {.width = grow ? -1 : 80,
                                        .size = (cui_control_size)(size < 4 ? size : 0),
                                        .design_size = size < 4 ? CUI_DESIGN_INHERIT
                                                                : (cui_design_size)(size - 3)};
                /* Width scales with the selected metrics: leaves useful label space at 9XL. */
                float font = size < 4 ? (size == 3   ? 12
                                         : size == 2 ? 16
                                                     : 14) *
                                            (theme.font_size / 16)
                                      : 0;
                if (size >= 4) {
                    cui_design_metrics metrics;
                    assert(cui_design_measure(o.design_size, theme.font_size / 16, &metrics));
                    font = metrics.font_size;
                }
                float width = font * 8;
                if (!grow)
                    o.width = width;
                cui_draw_list d = scene(ui, &theme, width, long_name, o, false, false);
                assert(cui_last_error(ui) == CUI_OK);
                contained(d, cui_bounds(ui, 10), true);
                full_name(ui, long_name);
                /* Current-frame width change needs no prior geometry warm-up. */
                d = scene(
                    ui, &theme, width * 1.2f, long_name,
                    (cui_button_options){.width = -1, .size = o.size, .design_size = o.design_size},
                    false, false);
                assert(cui_last_error(ui) == CUI_OK);
                contained(d, cui_bounds(ui, 10), true);
            }
    theme = cui_theme_light();
    cui_button_options o = {.width = 80, .on_click = clicked, .action_user = &clicks};
    cui_draw_list d = scene(ui, &theme, 80, long_name, o, false, false);
    contained(d, cui_bounds(ui, 10), true);
    cui_str alt = cui_string("Independent accessible name");
    o.accessibility_label = alt;
    d = scene(ui, &theme, 80, long_name, o, false, false);
    contained(d, cui_bounds(ui, 10), true);
    full_name(ui, alt);
    cui_rect b = cui_bounds(ui, 10);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = b.x + 10, .y = b.y + 10}));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = b.x + 10, .y = b.y + 10}));
    scene(ui, &theme, 80, long_name, o, false, false);
    assert(clicks == 1);
    cui_button_state state = {0};
    o.state = &state;
    o.label_width = 30;
    scene(ui, &theme, 80, long_name, o, false, false);
    assert(strstr(state.label, "…"));
    char saved[256];
    memcpy(saved, state.label, sizeof saved);
    scene(ui, &theme, 80, long_name, o, true, false);
    assert(!memcmp(saved, state.label, sizeof saved));
    d = scene(ui, &theme, 80, long_name, o, false, true);
    assert(!d.count);
    o = (cui_button_options){
        .width = 180, .icon = {"check", 5}, .trailing_icon = {"check", 5}, .dropdown_caret = true};
    d = scene(ui, &theme, 180, long_name, o, false, false);
    contained(d, cui_bounds(ui, 10), true);
    float n = 18;
    o.leading_content = slot;
    o.leading_user = &n;
    o.trailing_content = slot;
    o.trailing_user = &n;
    slots = 0;
    d = scene(ui, &theme, 180, long_name, o, false, false);
    assert(slots == 2);
    contained(d, cui_bounds(ui, 10), true);
    /* Explicit label maximum still composes with artwork and the right-aligned caret. */
    o = (cui_button_options){.width = 180,
                             .label_width = 30,
                             .state = &state,
                             .icon = {"check", 5},
                             .dropdown_caret = true};
    d = scene(ui, &theme, 180, long_name, o, false, false);
    contained(d, cui_bounds(ui, 10), true);
    /* No fitting ellipsis produces no text, and empty labels need no paint. */
    o = (cui_button_options){.width = 20};
    d = scene(ui, &theme, 20, long_name, o, false, false);
    for (size_t i = 0; i < d.count; i++)
        assert(d.commands[i].type != CUI_DRAW_TEXT);
    /* Full fit and unconstrained FIT preserve the original text, not a buffer-limited prefix. */
    char huge[2048];
    memset(huge, 'i', sizeof huge);
    o = (cui_button_options){.width = 5000, .font_size = 1};
    d = scene(ui, &theme, 5000, (cui_str){huge, sizeof huge}, o, false, false);
    bool full = false;
    for (size_t i = 0; i < d.count; i++)
        if (d.commands[i].type == CUI_DRAW_TEXT)
            full |= d.commands[i].text.length == sizeof huge;
    assert(full);
    full_name(ui, (cui_str){huge, sizeof huge});
    o = (cui_button_options){.width = -1, .allow_content_overflow = true};
    d = scene(ui, &theme, 80, long_name, o, false, false);
    bool overflow = false;
    b = cui_bounds(ui, 10);
    for (size_t i = 0; i < d.count; i++)
        if (d.commands[i].type == CUI_DRAW_TEXT)
            overflow |= d.commands[i].bounds.y + d.commands[i].bounds.h > b.y + b.h;
    assert(overflow);
    /* A grapheme-safe prefix never separates a combining mark or ZWJ sequence. */
    cui_str unicode = cui_string(
        "e\xcc\x81 family \xf0\x9f\x91\xa9\xe2\x80\x8d\xf0\x9f\x91\xa9 repeated e\xcc\x81");
    if (fixture_measure)
        unicode = cui_string("e\xcc\x81 repeated combining e\xcc\x81 repeated combining");
    o = (cui_button_options){.width = 60};
    d = scene(ui, &theme, 60, unicode, o, false, false);
    contained(d, cui_bounds(ui, 10), true);
    for (size_t i = 0; i < d.count; i++)
        if (d.commands[i].type == CUI_DRAW_TEXT && d.commands[i].text.length != 3)
            assert(cui_grapheme_boundary(unicode, d.commands[i].text.length));
    /* Original temporary bytes are copied before the caller mutates them. */
    char mutable_name[] = "Original label copied before mutation";
    assert(cui_begin_frame(ui, 300, 200, 0));
    cui_begin_box(ui, 1, (cui_box){.width = 120, .height = 100});
    cui_button_ex(ui, 10, &theme, cui_string(mutable_name), (cui_button_options){.width = -1});
    memset(mutable_name, 'z', sizeof mutable_name - 1);
    cui_end_box(ui);
    d = cui_end_frame(ui);
    bool original = false;
    for (size_t i = 0; i < d.count; i++)
        if (d.commands[i].type == CUI_DRAW_TEXT && d.commands[i].text.length)
            original |= d.commands[i].text.data[0] == 'O';
    assert(original);
    /* Malformed shape is rejected before callbacks or label paint. */
    o = (cui_button_options){.width = 80,
                             .custom_corners = true,
                             .corner_radii = {NAN, 0, 0, 0},
                             .on_click = clicked,
                             .action_user = &clicks};
    unsigned before = clicks;
    d = scene(ui, &theme, 80, long_name, o, false, false);
    assert(clicks == before);
    for (size_t i = 0; i < d.count; i++)
        assert(d.commands[i].type != CUI_DRAW_TEXT);
    /* Internal label ownership stays nonzero even for the salt-valued public ID. */
    assert(cui_begin_frame(ui, 80, 80, 0));
    cui_button_ex(ui, UINT64_C(0x2a7251d89c831be7), &theme, long_name,
                  (cui_button_options){.width = 80});
    d = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    contained(d, cui_bounds(ui, UINT64_C(0x2a7251d89c831be7)), true);
    cui_destroy(ui);
    /* Copied label capacity fails atomically; the next small frame recovers. */
    ui = cui_create((cui_config){.max_elements = 128, .max_commands = 512, .frame_bytes = 512});
    assert(ui);
    d = scene(ui, &theme, 80, (cui_str){huge, sizeof huge}, (cui_button_options){.width = -1},
              false, false);
    assert(cui_last_error(ui) == CUI_ERROR_CAPACITY && !d.count);
    cui_clear_error(ui);
    d = scene(ui, &theme, 80, cui_string("OK"), (cui_button_options){.width = -1}, false, false);
    assert(cui_last_error(ui) == CUI_OK);
    contained(d, cui_bounds(ui, 10), false);
    /* Paint-only avoids semantic copying: this exhausts the owned prepaint tail itself. */
    assert(cui_begin_frame(ui, 80, 80, 0));
    cui_button_paint(ui, 10, &theme, (cui_str){huge, sizeof huge},
                     (cui_button_options){.width = 80});
    d = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_ERROR_CAPACITY && !d.count);
    cui_destroy(ui);
    return 0;
}
