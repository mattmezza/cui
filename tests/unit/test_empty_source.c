/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include "cui/cui_content.h"
#include <assert.h>
#include <math.h>
static unsigned order[8], count;
static void child(cui_context *ui, const cui_theme *t, cui_color color, void *user) {
    unsigned n = *(unsigned *)user;
    (void)t;
    (void)color;
    order[count++] = n;
    cui_begin_box(ui, 100 + n, (cui_box){.width = 24, .height = 16});
    cui_end_box(ui);
}
static unsigned activations;
static bool disabled;
static void action(cui_context *ui, const cui_theme *t, cui_color color, void *user) {
    (void)color;
    (void)user;
    if (cui_button(ui, 500, t, cui_string("Create"), disabled))
        ++activations;
}
static void action_frame(cui_context *ui, const cui_theme *t) {
    cui_empty_slot content = {.children = action};
    assert(cui_begin_frame(ui, 300, 200, .016f));
    assert(cui_empty_ex(ui, 1, t, (cui_empty_options){.content = &content}));
    cui_end_frame(ui);
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 256});
    assert(ui);
    cui_theme t = cui_theme_light();
    unsigned numbers[] = {0, 1, 2, 3, 4};
    cui_empty_slot slots[4] = {0};
    for (unsigned i = 0; i < 4; ++i) {
        slots[i].children = child;
        slots[i].user = &numbers[i];
    }
    cui_empty_header h = {.media = &slots[0], .title = &slots[1], .description = &slots[2]};
    cui_empty_options o = {
        .header = &h, .content = &slots[3], .children = child, .user = &numbers[4]};
    cui_empty_style root = cui_empty_default_style(&t, CUI_EMPTY_ROOT, CUI_EMPTY_MEDIA_DEFAULT, 16);
    assert(root.box.background.a == 0 && root.box.border_width == 0);
    assert(root.box.padding == 24 && root.box.gap == 16);
    cui_empty_style icon = cui_empty_default_style(&t, CUI_EMPTY_MEDIA, CUI_EMPTY_MEDIA_ICON, 20);
    assert(icon.box.width == 40 && icon.box.height == 40);
    cui_empty_style desc =
        cui_empty_default_style(&t, CUI_EMPTY_DESCRIPTION, CUI_EMPTY_MEDIA_DEFAULT, 16);
    assert(desc.font_size == 14 && desc.line_height == 22.75f);
    assert(cui_begin_frame(ui, 800, 600, .016f));
    assert(cui_empty_ex(ui, 1, &t, o));
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK && count == 5);
    for (unsigned i = 0; i < 5; ++i)
        assert(order[i] == i);
    cui_id header_id = 1 ^ (UINT64_C(0x874e65fa19c80721) + UINT64_C(0x9e3779b97f4a7c15));
    cui_rect header_bounds = cui_bounds(ui, header_id);
    assert(header_bounds.w == 384);

    for (unsigned i = 1; i < 5; ++i) {
        cui_rect a = cui_bounds(ui, 99 + i), b = cui_bounds(ui, 100 + i);
        assert(a.y + a.h <= b.y && a.w > 0 && b.w > 0);
        assert(fabsf(a.x + a.w / 2 - b.x - b.w / 2) < 1);
    }
    cui_empty_style leading_root = root;
    cui_empty_style leading_header =
        cui_empty_default_style(&t, CUI_EMPTY_HEADER, CUI_EMPTY_MEDIA_DEFAULT, 16);
    cui_empty_style leading_content =
        cui_empty_default_style(&t, CUI_EMPTY_CONTENT, CUI_EMPTY_MEDIA_DEFAULT, 16);
    cui_empty_style leading_description =
        cui_empty_default_style(&t, CUI_EMPTY_DESCRIPTION, CUI_EMPTY_MEDIA_DEFAULT, 16);
    leading_description.box.align_x = CUI_ALIGN_START;
    slots[2].style = &leading_description;
    leading_root.box.align_x = CUI_ALIGN_START;
    leading_header.box.align_x = CUI_ALIGN_START;
    leading_content.box.align_x = CUI_ALIGN_START;
    o.style = &leading_root;
    h.style = &leading_header;
    slots[3].style = &leading_content;
    count = 0;
    assert(cui_begin_frame(ui, 800, 600, .016f));
    assert(cui_empty_ex(ui, 1, &t, o));
    cui_end_frame(ui);
    assert(count == 5);
    for (unsigned i = 1; i < 5; ++i)
        assert(fabsf(cui_bounds(ui, 100).x - cui_bounds(ui, 100 + i).x) < 1);
    o.style = NULL;
    h.style = NULL;
    slots[3].style = NULL;
    slots[2].style = NULL;
    count = 0;
    h.media = NULL;
    h.description = NULL;
    o.content = NULL;
    assert(cui_begin_frame(ui, 180, 600, .016f));
    assert(cui_empty_ex(ui, 1, &t, o));
    cui_end_frame(ui);
    assert(count == 2 && order[0] == 1 && order[1] == 4);
    root.box.padding = NAN;
    o.style = &root;
    count = 0;
    assert(cui_begin_frame(ui, 180, 600, .016f));
    assert(!cui_empty_ex(ui, 1, &t, o));
    size_t n = 0;
    cui_semantics(ui, &n);
    assert(n == 0 && count == 0);
    cui_end_frame(ui);
    o.style = NULL;
    h.title = &slots[2];
    slots[2].text = (cui_str){NULL, 1};
    assert(cui_begin_frame(ui, 180, 600, .016f));
    assert(!cui_empty_ex(ui, 1, &t, o));
    cui_end_frame(ui);
    slots[2] = (cui_empty_slot){
        .text = cui_string("Long Unicode description: 日本語 café — words that wrap across a "
                           "narrow available width with more supporting content.")};
    h.title = NULL;
    h.description = &slots[2];
    o.children = NULL;
    assert(cui_begin_frame(ui, 180, 600, .016f));
    assert(cui_empty_ex(ui, 1, &t, o));
    cui_draw_list wrapped = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    bool saw_text = false;
    for (size_t i = 0; i < wrapped.count; ++i)
        if (wrapped.commands[i].type == CUI_DRAW_TEXT) {
            saw_text = true;
            assert(wrapped.commands[i].bounds.w <= 132 && wrapped.commands[i].bounds.h > 22);
        }
    assert(saw_text);
    action_frame(ui, &t);
    cui_focus(ui, 500);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    action_frame(ui, &t);
    assert(activations == 1);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_ENTER}));
    action_frame(ui, &t);
    cui_rect button = cui_bounds(ui, 500);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = button.x + 2, .y = button.y + 2}));
    action_frame(ui, &t);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = button.x + 2, .y = button.y + 2}));
    action_frame(ui, &t);
    assert(activations == 2);
    disabled = true;
    action_frame(ui, &t);
    cui_focus(ui, 500);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    action_frame(ui, &t);
    assert(activations == 2);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = button.x + 2, .y = button.y + 2}));
    action_frame(ui, &t);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = button.x + 2, .y = button.y + 2}));
    action_frame(ui, &t);
    assert(activations == 2);
    cui_destroy(ui);
    return 0;
}
