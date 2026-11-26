/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include "cui/cui_media.h"

#include <math.h>

/* Keep one zero-initialized state per file between frames. */
typedef struct {
    cui_attachment_state card;
    cui_attachment_child children[2];
    const cui_theme *theme;
    cui_design_size size;
    bool inspected, disabled;
} attachment_slots_example;

static void draw_file_fallback(cui_context *ui, cui_id id, void *user, float width, float height) {
    const attachment_slots_example *content = user;
    cui_icon_named(ui, id, content->theme, cui_string("file-text"),
                   (cui_icon_options){.size = CUI_ICON_CUSTOM,
                                      .custom_size = fminf(64, fminf(width, height) * .6f)});
}

static void draw_inspected_overlay(cui_context *ui, cui_id id, void *user, float width,
                                   float height) {
    const attachment_slots_example *content = user;
    if (!content->inspected)
        return;
    float edge = fminf(20, fminf(width, height) * .45f);
    cui_begin_absolute_box(ui, id,
                           (cui_box){.background = content->theme->primary,
                                     .radius = edge / 2,
                                     .align_x = CUI_ALIGN_CENTER,
                                     .align_y = CUI_ALIGN_CENTER},
                           (cui_rect){width - edge - 2, height - edge - 2, edge, edge});
    cui_icon_named(ui, cui_id_from(ui, "inspect-check"), content->theme, cui_string("check"),
                   (cui_icon_options){.size = CUI_ICON_CUSTOM,
                                      .custom_size = edge * .75f,
                                      .custom_color = true,
                                      .color = content->theme->on_primary});
    cui_end_box(ui);
}

static void draw_file_metadata(cui_context *ui, cui_id id, void *user, float width, float height) {
    (void)height;
    const attachment_slots_example *content = user;
    cui_str text = cui_string(content->inspected ? "Inspected" : "Shared");
    cui_interact(ui, id, CUI_ROLE_LABEL, text, false, false, 0);
    cui_set_tab_stop(ui, id, false);
    cui_begin_box(ui, id, (cui_box){.width = width, .clip = true});
    cui_text(ui, text, content->theme->font_size * .8f, content->theme->muted);
    cui_end_box(ui);
}

static void draw_inspect_action(cui_context *ui, cui_id id, void *user, float width, float height) {
    (void)height;
    attachment_slots_example *content = user;
    if (cui_button_ex(ui, id, content->theme, cui_string("Inspect"),
                      (cui_button_options){.width = width,
                                           .size = CUI_CONTROL_XSMALL,
                                           .design_size = content->size,
                                           .disabled = content->disabled})) {
        content->inspected = !content->inspected;
        /* The metadata and overlay earlier in this submission read the previous state. */
        cui_invalidate(ui);
    }
}

/* Call during a frame with the space available to this card. */
static inline void draw_attachment_with_slots(cui_context *ui, cui_id id, const cui_theme *theme,
                                              attachment_slots_example *state, float available,
                                              bool vertical, cui_design_size size, bool disabled) {
    cui_design_metrics design;
    bool measured = cui_design_measure(size, theme->font_size / 16, &design);
    float font = measured ? design.font_size : theme->font_size;
    float width = fminf(available, fmaxf(320, 18 * font));
    if (width < 64)
        return;
    state->theme = theme;
    state->size = size;
    state->disabled = disabled;
    state->children[0] = (cui_attachment_child){.kind = CUI_ATTACHMENT_TITLE,
                                                .text = cui_string("Release notes.pdf")};
    state->children[1] = (cui_attachment_child){
        .kind = CUI_ATTACHMENT_CUSTOM, .render = draw_file_metadata, .user = state};
    cui_attachment_options file = cui_attachment_options_default();
    file.name = cui_string("Release notes attachment");
    file.model.design_size = size;
    file.model.width = width;
    file.model.vertical = vertical || width < 17 * font;
    file.children = state->children;
    file.child_count = sizeof state->children / sizeof *state->children;
    file.media_content = draw_file_fallback;
    file.media_overlay = draw_inspected_overlay;
    file.actions = draw_inspect_action;
    file.media_user = file.overlay_user = file.actions_user = state;
    float action_width = cui_measure_text(ui, cui_string("Inspect"), font).width +
                         (measured ? 2 * design.padding_x : 16);
    file.actions_width = fminf(width - 32, fmaxf(64, action_width));
    cui_attachment_with_metadata(ui, id, theme, &state->card, file,
                                 (cui_attachment_metadata_style){0});
}
