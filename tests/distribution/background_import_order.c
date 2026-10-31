/* SPDX-License-Identifier: MIT */
#define CUI_IMPLEMENTATION
#if CUI_TEST_BACKGROUND_ORDER != 4
#define CUI_BACKGROUND_IMPLEMENTATION
#endif
#if CUI_TEST_BACKGROUND_ORDER == 0
#include "cui_background.h"
#elif CUI_TEST_BACKGROUND_ORDER == 1
#include "cui_draw_v2.h"
#elif CUI_TEST_BACKGROUND_ORDER == 2
#include "cui.h"
#include "cui_background.h"
#elif CUI_TEST_BACKGROUND_ORDER == 3
#include "cui_all.h"
#else
/* This target has only its copied Core file on the project include path. */
#include "background_core_only.h"
#endif
#if CUI_TEST_BACKGROUND_ORDER != 4
/* Repeated imports must emit neither missing nor duplicated definitions. */
#include "cui.h"
#include "cui_background.h"
#include "cui_draw_v2.h"
#endif
#include <assert.h>
int main(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 32, .max_commands = 64});
    assert(ui && cui_begin_frame(ui, 20, 10, 0));
    cui_begin_box(ui, 123,
                  (cui_box){.width = 4,
                            .height = 4,
                            .fixed_width = true,
                            .fixed_height = true,
                            .background = {255, 0, 0, 255}});
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    assert(list.count && list.commands[0].type == CUI_DRAW_RECT);
    cui_destroy(ui);
#if CUI_TEST_BACKGROUND_ORDER != 4
    cui_background brush;
    cui_color color;
    bool (*volatile make)(cui_background_color, cui_background *) = cui_background_make_solid;
    bool (*volatile sample)(const cui_background *, cui_rect, float, float, cui_color *) =
        cui_background_sample_srgba8;
    assert(make((cui_background_color){1, 0, 0, .5f}, &brush));
    assert(sample(&brush, (cui_rect){0, 0, 4, 4}, 2, 2, &color));
    assert(color.r == 255 && color.g == 0 && color.b == 0 && color.a == 128);
#endif
    return 0;
}
