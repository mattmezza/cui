/* Frozen b4cd6a8f Badge functions; original excerpt SHA256
 * 716fb343fb673c5102f9474b0df3eb864fe6a96c9366126f1884f821130cd1d8 */
#include "../../src/base/cui_design_metrics.h"
#include "cui/cui_components.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
void frozen_cui_badge(cui_context *ui, cui_id id, const cui_theme *t, cui_str label) {
    cui_begin_box(ui, id, (cui_box){.padding = 4, .radius = t->radius, .background = t->primary});
    cui_text(ui, label, t->font_size * .8f, t->on_primary);
    cui_end_box(ui);
}
cui_id frozen_cui_badge_indicator_id(cui_id id) {
    return id ? id ^ UINT64_C(0x2780a7c4f39251db) : 0;
}
cui_badge_options frozen_cui_badge_defaults(void) {
    return (cui_badge_options){.maximum = 99, .size = CUI_CONTROL_MEDIUM};
}
bool frozen_cui_badge_ex(cui_context *ui, cui_id id, const cui_theme *t, cui_badge_options o,
                         cui_button_content_fn content, void *user) {
    if (!ui || !id || !t || !cui_design_valid(o.design_size) || o.variant < CUI_BADGE_COUNT ||
        o.variant > CUI_BADGE_ICON || o.size < CUI_CONTROL_MEDIUM || o.size > CUI_CONTROL_XSMALL ||
        (o.name.length && !o.name.data) || (o.icon.length && !o.icon.data))
        return false;
    cui_design_metrics dm = {0};
    if (o.design_size && !cui_design_resolve(o.design_size, t->font_size / 16, &dm))
        return false;
    float badge_font = o.design_size ? dm.font_size * .75f : 10;
    char number[3 * sizeof(size_t) + 3];
    if (o.count > o.maximum)
        snprintf(number, sizeof(number), "%zu+", o.maximum);
    else
        snprintf(number, sizeof(number), "%zu", o.count);
    bool visible = o.variant != CUI_BADGE_COUNT || o.count > 0;
    cui_begin_box(ui, id, (cui_box){0});
    if (content)
        content(ui, user);
    if (visible) {
        float extent = o.size == CUI_CONTROL_LARGE ? 24 : o.size == CUI_CONTROL_MEDIUM ? 16 : 10;
        if (o.design_size)
            extent = dm.indicator_size;
        float x = o.variant != CUI_BADGE_COUNT ? 0
                                               : (o.size == CUI_CONTROL_LARGE    ? 1
                                                  : o.size == CUI_CONTROL_MEDIUM ? 3
                                                                                 : 4) *
                                                     strlen(number);
        float y = o.variant != CUI_BADGE_COUNT   ? 0
                  : o.size == CUI_CONTROL_LARGE  ? 2
                  : o.size == CUI_CONTROL_MEDIUM ? -3
                                                 : -4;
        cui_id overlay = id ^ UINT64_C(0x35d9258f62be9a0c);
        cui_id indicator = frozen_cui_badge_indicator_id(id);
        cui_begin_absolute_box(
            ui, overlay,
            (cui_box){.align_x = CUI_ALIGN_END,
                      .align_y = o.variant == CUI_BADGE_ICON ? CUI_ALIGN_END : CUI_ALIGN_START},
            (cui_rect){x, y, -1, -1});
        cui_box mark = {.background = o.custom_color ? o.color : t->danger,
                        .radius = 1024,
                        .align_x = CUI_ALIGN_CENTER,
                        .align_y = CUI_ALIGN_CENTER};
        if (o.variant == CUI_BADGE_COUNT) {
            mark.height = o.design_size ? dm.line_height * .75f + dm.padding_y : 14;
            mark.width =
                fmaxf(mark.height, cui_measure_text(ui, cui_string(number), badge_font).width +
                                       (o.design_size ? dm.padding_x : 4));
        } else {
            mark.width = mark.height = o.variant == CUI_BADGE_DOT
                                           ? (o.design_size ? dm.indicator_size * .375f : 6)
                                           : extent;
            if (o.variant == CUI_BADGE_ICON) {
                mark.border = t->background;
                mark.border_width = 1;
            }
        }
        cui_begin_box(ui, indicator, mark);
        cui_str name = o.name.length                  ? o.name
                       : o.variant == CUI_BADGE_COUNT ? cui_string(number)
                       : o.variant == CUI_BADGE_DOT   ? cui_string("Notification")
                                                      : cui_string("Status");
        cui_interact(ui, indicator, CUI_ROLE_STATUS, name, false, false, (float)o.count);
        cui_set_semantic_details(ui, indicator,
                                 (cui_semantic_details){.parent = id, .no_focus = true});
        cui_set_tab_stop(ui, indicator, false);
        if (o.variant == CUI_BADGE_COUNT)
            cui_text_with_line_height(ui, cui_string(number), badge_font,
                                      (cui_color){255, 255, 255, 255}, badge_font);
        else if (o.variant == CUI_BADGE_ICON)
            cui_icon_named(ui, indicator ^ UINT64_C(0x2c38f42991a75e63), t, o.icon,
                           (cui_icon_options){.custom_size = extent - 2,
                                              .size = CUI_ICON_CUSTOM,
                                              .color = {255, 255, 255, 255},
                                              .custom_color = true});
        cui_end_box(ui);
        cui_end_box(ui);
    }
    cui_end_box(ui);
    return true;
}
