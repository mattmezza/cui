/* SPDX-License-Identifier: MIT */
#ifndef CUI_RADIO_TOOLTIP_INTERNAL_H
#define CUI_RADIO_TOOLTIP_INTERNAL_H
#include "cui/cui_components.h"
#include <math.h>

static inline bool cui_radio_tooltip_valid(const cui_radio_tooltip *hint, cui_id trigger,
                                           cui_id indicator) {
    return !hint || (hint->id && hint->id != trigger && hint->id != indicator && hint->state &&
                     hint->text.data && hint->text.length);
}

static inline bool cui_radio_tooltip_show(cui_context *ui, const cui_theme *theme, cui_id trigger,
                                          bool disabled, const cui_radio_tooltip *hint) {
    if (!hint)
        return false;
    uint64_t frame = cui_frame_sequence(ui);
    if (!frame || cui_last_error(ui) != CUI_OK)
        return false;
    cui_radio_tooltip_state *state = hint->state;
    if (state->trigger != trigger || state->panel != hint->id || state->last_frame != frame - 1)
        *state = (cui_radio_tooltip_state){.trigger = trigger, .panel = hint->id};
    state->last_frame = frame;
    cui_hover_options options = hint->options;
    cui_rect viewport = options.viewport;
    if (!isfinite(viewport.x) || !isfinite(viewport.y) || !isfinite(viewport.w) ||
        !isfinite(viewport.h) || viewport.w <= 0 || viewport.h <= 0) {
        state->hover = (cui_hover_state){0};
        return false;
    }
    options.trigger = trigger;
    options.dt = cui_frame_delta(ui);
    options.disabled |= disabled;
    return cui_tooltip(ui, hint->id, theme, &state->hover, hint->text, options);
}
#endif
