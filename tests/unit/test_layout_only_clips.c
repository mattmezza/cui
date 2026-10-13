/* SPDX-License-Identifier: MIT */
#include "cui/cui.h"
#include <assert.h>
#include <stdio.h>
static void nested(cui_context *ui, bool layout_only, bool inert) {
    cui_begin_box(
        ui, 10,
        (cui_box){
            .width = 100, .height = 80, .clip = true, .layout_only = layout_only, .inert = inert});
    cui_begin_box(ui, 0, (cui_box){.width = 80, .height = 60, .clip = true});
    cui_begin_box(
        ui, 12,
        (cui_box){.width = 60, .height = 40, .clip = true, .background = {11, 22, 33, 255}});
    cui_interact(ui, 12, CUI_ROLE_BUTTON, cui_string("Nested"), false, false, 0);
    cui_end_box(ui);
    cui_end_box(ui);
    cui_end_box(ui);
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 64, .max_commands = 64});
    assert(ui);
    assert(cui_begin_frame(ui, 200, 200, 0));
    nested(ui, true, false);
    size_t semantic_count = 0;
    cui_draw_list draw = cui_end_frame(ui);
    cui_semantics(ui, &semantic_count);
    assert(cui_last_error(ui) == CUI_OK && draw.count == 0 && semantic_count == 0);
    assert(cui_bounds(ui, 12).w == 60 && cui_bounds(ui, 12).h == 40);
    assert(cui_begin_frame(ui, 200, 200, 0));
    nested(ui, false, true);
    draw = cui_end_frame(ui);
    cui_semantics(ui, &semantic_count);
    assert(cui_last_error(ui) == CUI_OK && draw.count == 7 && semantic_count == 0);
    size_t depth = 0, rectangles = 0;
    for (size_t i = 0; i < draw.count; ++i) {
        if (draw.commands[i].type == CUI_DRAW_CLIP_PUSH)
            ++depth;
        else if (draw.commands[i].type == CUI_DRAW_CLIP_POP) {
            assert(depth);
            --depth;
        } else {
            assert(draw.commands[i].type == CUI_DRAW_RECT && depth == 3);
            ++rectangles;
        }
    }
    assert(depth == 0 && rectangles == 1);
    assert(cui_begin_frame(ui, 200, 200, 0));
    cui_begin_box(ui, 20, (cui_box){.width = 150, .height = 150, .clip = true});
    nested(ui, true, false);
    cui_begin_box(ui, 21, (cui_box){.width = 20, .height = 20, .background = {90, 80, 70, 255}});
    cui_end_box(ui);
    cui_end_box(ui);
    draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK && draw.count == 3);
    assert(draw.commands[0].type == CUI_DRAW_CLIP_PUSH && draw.commands[1].type == CUI_DRAW_RECT &&
           draw.commands[1].color.r == 90 && draw.commands[2].type == CUI_DRAW_CLIP_POP);
    cui_destroy(ui);
    puts("layout-only nested clips suppressed; inert and sibling paint clips retained");
    return 0;
}
