/* SPDX-License-Identifier: MIT */
/* Private traversal proof: ordinary frame publication, no desktop adapter. */
#include "../../src/core/cui.c"
#include <assert.h>
static void item(cui_context *ui, cui_id id, int index, bool stop, bool disabled) {
    cui_begin_box(ui, id, (cui_box){.width = 30, .height = 30});
    cui_interact(ui, id, CUI_ROLE_BUTTON, cui_string("item"), disabled, false, 0);
    cui_set_tab_index(ui, id, index);
    cui_set_tab_stop(ui, id, stop);
    cui_end_box(ui);
}
static void frame(cui_context *ui, bool modal, bool removed) {
    assert(cui_begin_frame(ui, 400, 400, .016f));
    item(ui, 1, 4, true, false);
    item(ui, 9, -999, true, false);
    cui_set_semantic_details(ui, 9, (cui_semantic_details){.no_focus = true});
    assert(cui_begin_tab_group(ui, -2));
    item(ui, 2, 99, true, false);
    assert(cui_begin_tab_group(ui, -1));
    if (!removed) item(ui, 3, -9, true, false);
    item(ui, 4, -9, true, false);
    cui_end_tab_group(ui);
    item(ui, 5, -1, false, false);
    item(ui, 6, -100, true, true);
    cui_end_tab_group(ui);
    if (modal) {
        assert(cui_begin_focus_scope(ui, 100, true));
        item(ui, 7, -3, true, false);
        item(ui, 8, -4, true, false);
        cui_end_focus_scope(ui);
    }
    cui_end_frame(ui);
}
int main(void) {
    cui_context *ui = cui_create((cui_config){0});
    assert(ui);
    frame(ui, false, false);
    cui_core_tab(ui, false); assert(ui->focus == 3);
    /* Metadata refinements compose with independently registered ordering. */
    assert(ui->previous[1].tab_ordered && ui->previous[1].tab_index == -999);
    uint64_t generation = ui->focus_generation;
    cui_core_tab(ui, false); assert(ui->focus == 4 && ui->focus_generation > generation);
    cui_core_tab(ui, false); assert(ui->focus == 2);
    cui_core_tab(ui, false); assert(ui->focus == 1);
    cui_core_tab(ui, false); assert(ui->focus == 3);
    cui_core_tab(ui, true); assert(ui->focus == 1);
    cui_core_set_focus(ui, 5);
    cui_core_tab(ui, false); assert(ui->focus == 3);
    /* Current-frame setters cannot rewrite the rendered snapshot. */
    assert(cui_begin_frame(ui, 400, 400, .016f));
    item(ui, 1, -999, true, false);
    cui_core_set_focus(ui, 0);
    cui_core_tab(ui, false); assert(ui->focus == 3);
    cui_end_frame(ui);
    frame(ui, true, false);
    cui_core_tab(ui, false); assert(ui->focus == 8);
    cui_core_tab(ui, true); assert(ui->focus == 7);
    frame(ui, false, true);
    cui_core_set_focus(ui, 0);
    cui_core_tab(ui, false); assert(ui->focus == 4);
    assert(cui_begin_frame(ui, 400, 400, .016f));
    for (size_t n = 0; n < 32; ++n) assert(cui_begin_tab_group(ui, (int)n));
    assert(!cui_begin_tab_group(ui, 33) && ui->tab_depth == 32);
    for (size_t n = 0; n < 32; ++n) cui_end_tab_group(ui);
    cui_end_frame(ui);
    cui_destroy(ui);
}
