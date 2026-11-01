/* SPDX-License-Identifier: MIT */
#ifndef CUI_DIALOG_COMPOSITION_PRIVATE_H
#define CUI_DIALOG_COMPOSITION_PRIVATE_H
#include "cui/cui_dialog_composition.h"
typedef struct {
    cui_box panel;
    cui_dialog_slot_style_v1 slots[CUI_DIALOG_SLOT_COUNT];
    cui_scrollbar_style scrollbar;
    cui_color thumb, track;
} cui_dialog_resolved;
struct cui_dialog_frame {
    cui_context *ui;
    cui_dialog_layout_v1 *layout;
    cui_surface_options options;
    const cui_dialog_resolved *resolved;
    float width;
    unsigned wrappers;
    bool active, input_ready, body_input_ready;
};
typedef void (*cui_dialog_close_emitter)(cui_context *, const cui_dialog_frame *, const void *);
cui_dialog_submit_result
cui_dialog_compose_resolved(cui_context *, cui_dialog_layout_v1 *, cui_surface_options,
                            const cui_dialog_sizing_v1 *, const cui_dialog_composition_v1 *,
                            const cui_dialog_resolved *, cui_dialog_close_emitter, const void *);
bool cui_dialog_resolve(const cui_theme *, const cui_dialog_refinement_v1 *, cui_dialog_resolved *);
bool cui_dialog_action_reduce(cui_context *, const cui_dialog_frame *, cui_dialog_action_kind);
#endif
