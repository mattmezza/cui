/* SPDX-License-Identifier: MIT */
#ifdef CUI_SKELETON_PRESENTATION_ALL
#define CUI_IMPLEMENTATION_ALL
#include "cui_all.h"
#else
#define CUI_IMPLEMENTATION
#include "cui.h"
#define CUI_BASE_IMPLEMENTATION
#include "cui_base.h"
#define CUI_THEME_IMPLEMENTATION
#include "cui_theme.h"
#define CUI_COMPONENTS_IMPLEMENTATION
#include "cui_components.h"
#define CUI_SKELETON_PRESENTATION_IMPLEMENTATION
#include "cui_skeleton_presentation.h"
#endif
#include <assert.h>
int main(void) {
    cui_context *ui = cui_create((cui_config){0});
    assert(ui);
    cui_skeleton_palette palette = {{21, 34, 55, 255}};
    cui_loading_motion motion = {0};
    cui_loading_motion_result result;
    cui_skeleton_presentation p = cui_skeleton_presentation_default();
    p.reduced_motion = true;
    assert(cui_begin_frame(ui, 320, 100, 0));
    assert(cui_skeleton_present(ui, 10, &palette, &motion, p, &result));
    cui_draw_list draw = cui_end_frame(ui);
    assert(draw.count && result.opacity == 1 && result.next_wake_seconds == 0);
    cui_rect bounds = cui_bounds(ui, 10);
    assert(bounds.w == 320 && bounds.h == 16);
    cui_destroy(ui);
    return 0;
}
