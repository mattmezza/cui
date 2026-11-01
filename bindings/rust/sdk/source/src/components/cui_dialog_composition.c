/* SPDX-License-Identifier: MIT */
#include "../base/cui_dialog_composition_private.h"
#include "cui/cui_dialog_composition_styled.h"
#include <math.h>
bool cui_dialog_action_v1(cui_context *ui, const cui_dialog_frame *f, const cui_theme *t, cui_id id,
                          cui_str name, cui_dialog_action_kind kind, cui_button_options o) {
    if (!ui || !f || !f->active || f->ui != ui || !t || !id || (unsigned)kind > CUI_DIALOG_CLOSE ||
        !cui_frame_is_submitting(ui)) {
        if (ui)
            cui_report_diagnostic(ui, &(cui_diagnostic){.error = CUI_ERROR_INVALID_ACTION});
        return false;
    }
    o.disabled |= !f->layout->surface->overlay.open;
    bool activated = cui_button_ex(ui, id, t, name, o);
    if (activated)
        (void)cui_dialog_action_reduce(ui, f, kind);
    return activated;
}
static void cui_dialog_styled_close(cui_context *ui, const cui_dialog_frame *f, const void *user) {
    const cui_theme *t = user;
    (void)cui_dialog_action_v1(ui, f, t, cui_surface_part_id(f->layout->surface->overlay.id, 1),
                               cui_string("Close"), CUI_DIALOG_CLOSE,
                               (cui_button_options){.variant = CUI_BUTTON_GHOST,
                                                    .size = CUI_CONTROL_SMALL,
                                                    .width = 24,
                                                    .height = 24,
                                                    .icon = {"close", 5},
                                                    .icon_only = true});
}
cui_dialog_submit_result cui_dialog_compose_v1(cui_context *ui, const cui_theme *t,
                                               cui_dialog_layout_v1 *l, cui_surface_options o,
                                               const cui_dialog_sizing_v1 *z,
                                               const cui_dialog_composition_v1 *c) {
    if (!t || !isfinite(t->font_size) || t->font_size <= 0 || !cui_dialog_composition_valid_v1(c)) {
        if (ui)
            cui_report_diagnostic(ui, &(cui_diagnostic){.error = CUI_ERROR_INVALID_ACTION});
        return (cui_dialog_submit_result){0};
    }
    cui_dialog_resolved r;
    if (!cui_dialog_resolve(t, c->refinement, &r))
        return (cui_dialog_submit_result){0};
    return cui_dialog_compose_resolved(ui, l, o, z, c, &r, cui_dialog_styled_close, t);
}
