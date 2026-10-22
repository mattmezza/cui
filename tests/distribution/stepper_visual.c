/* SPDX-License-Identifier: MIT */
#ifdef CUI_STEPPER_ALL
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
#define CUI_STEPPER_VISUAL_IMPLEMENTATION
#include "cui_stepper_visual.h"
#endif
#include <assert.h>
int main(void) {
    cui_context *ui = cui_create((cui_config){0});
    assert(ui);
    cui_theme theme = cui_theme_light();
    cui_step steps[] = {{.title = CUI_STR("Start")}, {.title = CUI_STR("Done")}};
    assert(cui_begin_frame(ui, 400, 200, .016f));
    assert(cui_stepper_visual_navigation(ui, 10, &theme, steps, 2, 1,
                                         (cui_stepper_visual_options){0},
                                         (cui_stepper_navigation_handler){0})
               .valid);
    assert(cui_end_frame(ui).count);
    cui_destroy(ui);
    return 0;
}
