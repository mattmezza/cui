/* SPDX-License-Identifier: MIT */
#include "cui/cui_status.h"
bool cui_begin_tag(cui_context *ui, cui_id id, const cui_theme *t, cui_tag_options o,
                   cui_tag_style *out) {
    cui_tag_style s;
    if (!ui || !cui_tag_resolve(t, o, cui_hovered(ui, id), &s))
        return false;
    /* Separate horizontal/vertical padding using inner composition, since the
     * core box exposes uniform padding. */
    cui_begin_box(ui, id,
                  (cui_box){.row = true,
                            .padding = s.padding_y,
                            .radius = s.radius,
                            .background = s.background,
                            .border = s.border,
                            .border_width = 1,
                            .align_y = CUI_ALIGN_CENTER});
    cui_begin_box(ui, id ^ UINT64_C(0xe937dbc482156a30),
                  (cui_box){.width = s.padding_x - s.padding_y, .height = 1});
    cui_end_box(ui);
    cui_begin_box(ui, id ^ UINT64_C(0xa037dbc482156a30),
                  (cui_box){.row = true, .align_y = CUI_ALIGN_CENTER});
    if (out)
        *out = s;
    return true;
}
void cui_end_tag(cui_context *ui, cui_id id, const cui_tag_style *s) {
    if (ui && s) {
        cui_end_box(ui);
        cui_begin_box(ui, id ^ UINT64_C(0xb037dbc482156a30),
                      (cui_box){.width = s->padding_x - s->padding_y, .height = 1});
        cui_end_box(ui);
        cui_end_box(ui);
    }
}
bool cui_tag(cui_context *ui, cui_id id, const cui_theme *t, cui_str text, cui_tag_options o) {
    cui_tag_style s;
    if (!cui_begin_tag(ui, id, t, o, &s))
        return false;
    cui_interact(ui, id, CUI_ROLE_LABEL, text, false, false, 0);
    cui_set_tab_stop(ui, id, false);
    cui_text(ui, text, s.font_size, s.foreground);
    cui_end_tag(ui, id, &s);
    return true;
}
