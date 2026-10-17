/* SPDX-License-Identifier: MIT */
#define CUI_IMPLEMENTATION
#include "cui.h"
#define CUI_TEXT_OVERFLOW_IMPLEMENTATION
#include "cui_text_overflow.h"
#include <assert.h>
int main(void) {
    cui_context *ui = cui_create((cui_config){.width = 160, .height = 40});
    assert(ui);
    assert(cui_begin_frame(ui, 160, 40, 0));
    assert(cui_text_ellipsis(
        ui, 1, cui_string("A subject too wide for this list"),
        (cui_text_ellipsis_options){.width = 80, .font_size = 14, .color = {0, 0, 0, 255}}));
    cui_draw_list list = cui_end_frame(ui);
    assert(list.count && cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
    return 0;
}
