/* SPDX-License-Identifier: MIT */
#ifndef SKELETON_RECIPES_EXAMPLE_H
#define SKELETON_RECIPES_EXAMPLE_H

#include "cui/cui_components.h"
#include "cui/cui_skeleton_presentation.h"

typedef enum {
    SKELETON_EXAMPLE_PROFILE,
    SKELETON_EXAMPLE_ARTICLES,
    SKELETON_EXAMPLE_TABLE
} skeleton_example_layout;

typedef struct {
    cui_loading_motion motion[9];
    skeleton_example_layout layout;
} skeleton_example_state;

static inline void skeleton_example_semantics(cui_context *ui, cui_id id, cui_role role,
                                              const char *name) {
    cui_interact(ui, id, role, cui_string(name), false, false, 0);
    cui_set_tab_stop(ui, id, false);
}

static inline void skeleton_example_placeholder(cui_context *ui, const char *key, cui_box box,
                                                const cui_skeleton_palette *palette,
                                                cui_skeleton_presentation style,
                                                cui_loading_motion *motion, double *next_wake) {
    style.root.width = box.width;
    style.root.width_percent = box.width_percent;
    style.root.height = box.height;
    style.root.radius = box.radius;
    cui_loading_motion_result result;
    if (cui_skeleton_present(ui, cui_id_from(ui, key), palette, motion, style, &result) &&
        result.next_wake_seconds > 0 && (!*next_wake || result.next_wake_seconds < *next_wake))
        *next_wake = result.next_wake_seconds;
}

/* Keep one zero-initialized state per recipe instance. Call during a CUI frame.
 * Pass the app's logical rem, an owned palette and presentation color/motion options.
 * Use a stable instance key. Schedule the returned deadline on style.now_seconds' clock. */
static inline double skeleton_example_draw(cui_context *ui, const char *key, const cui_theme *theme,
                                           const cui_skeleton_palette *palette,
                                           skeleton_example_state *state,
                                           skeleton_example_layout layout, bool loading, float rem,
                                           cui_skeleton_presentation style) {
    static const char *const names[] = {"Profile", "Articles", "Workspace files"};
    static const char *const statuses[] = {"Profile loaded", "Articles loaded", "Files loaded"};
    static const char *const titles[] = {"Design notes", "Release recap", "Field guide"};
    static const char *const subtitles[] = {"4 min read", "6 min read", "8 min read"};
    static const char *const numbers[] = {"01", "02", "03"};
    static const char *const files[] = {"Notes", "Images", "Exports"};
    static const char *const states[] = {"Ready", "Queued", "Ready"};
    if (layout < SKELETON_EXAMPLE_PROFILE || layout > SKELETON_EXAMPLE_TABLE ||
        !cui_push_id(ui, key))
        return 0;
    cui_id region = cui_id_from(ui, "loading-region");
    cui_begin_box(ui, region,
                  (cui_box){.width = -1,
                            .max_width = (layout == SKELETON_EXAMPLE_PROFILE ? 30 : 36) * rem,
                            .padding = rem,
                            .gap = .75f * rem,
                            .radius = theme->radius_lg,
                            .border = theme->border,
                            .border_width = 1});
    skeleton_example_semantics(ui, region, CUI_ROLE_REGION, names[layout]);
    cui_set_semantic_details(ui, region, (cui_semantic_details){.busy = loading});
    if (!loading || state->layout != layout)
        *state = (skeleton_example_state){.layout = layout};
    double wake = 0;
    float radius = .25f * rem;
    if (layout == SKELETON_EXAMPLE_TABLE) {
        cui_id table = cui_id_from(ui, "files");
        cui_begin_box(ui, table, (cui_box){.width = 1, .width_percent = true, .gap = .25f * rem});
        if (!loading)
            skeleton_example_semantics(ui, table, CUI_ROLE_TABLE, "Files");
        for (size_t row = 0; row < 4; ++row) {
            bool header = row == 0;
            if (!cui_push_id(ui, header ? "header" : files[row - 1]))
                break;
            cui_id row_id = cui_id_from(ui, "row");
            cui_begin_box(
                ui, row_id,
                (cui_box){.width = 1,
                          .width_percent = true,
                          .height = 2 * rem,
                          .row = true,
                          .align_y = CUI_ALIGN_CENTER,
                          .gap = .5f * rem,
                          .padding = .25f * rem,
                          .radius = radius,
                          .background = header ? theme->muted_background : (cui_color){0}});
            if (!loading)
                skeleton_example_semantics(ui, row_id, CUI_ROLE_ROW, "");
            for (size_t column = 0; column < 2; ++column) {
                bool status = column == 1;
                cui_id cell = cui_id_from(ui, status ? "status" : "file");
                cui_begin_box(ui, cell, (cui_box){.width = status ? 4.5f * rem : -1, .clip = true});
                if (loading && !header) {
                    skeleton_example_placeholder(ui, status ? "status-bar" : "file-bar",
                                                 (cui_box){.width = status ? .65f : .3f + .1f * row,
                                                           .width_percent = true,
                                                           .height = .75f * rem,
                                                           .radius = radius},
                                                 palette, style,
                                                 &state->motion[(row - 1) * 2 + column], &wake);
                } else {
                    const char *text = header ? (status ? "Status" : "File")
                                              : (status ? states[row - 1] : files[row - 1]);
                    if (!loading)
                        skeleton_example_semantics(
                            ui, cell, header ? CUI_ROLE_COLUMNHEADER : CUI_ROLE_CELL, text);
                    cui_text(ui, cui_string(text), rem * .875f,
                             status && !header ? theme->muted : theme->text);
                }
                cui_end_box(ui);
            }
            cui_end_box(ui);
            cui_pop_id(ui);
        }
        cui_end_box(ui);
    } else {
        bool profile = layout == SKELETON_EXAMPLE_PROFILE;
        cui_id list = cui_id_from(ui, "items");
        cui_begin_box(ui, list, (cui_box){.width = 1, .width_percent = true, .gap = .75f * rem});
        if (!loading && !profile)
            skeleton_example_semantics(ui, list, CUI_ROLE_LIST, names[layout]);
        for (size_t row = 0; row < (profile ? 1u : 3u); ++row) {
            if (!cui_push_id(ui, numbers[row]))
                break;
            cui_id item = cui_id_from(ui, "item");
            float extent = 2.5f * rem;
            cui_begin_box(ui, item,
                          (cui_box){.width = 1,
                                    .width_percent = true,
                                    .height = extent,
                                    .row = true,
                                    .gap = .75f * rem,
                                    .align_y = CUI_ALIGN_CENTER});
            if (!loading && !profile)
                skeleton_example_semantics(ui, item, CUI_ROLE_LISTITEM, titles[row]);
            if (loading) {
                skeleton_example_placeholder(ui, "thumbnail",
                                             (cui_box){.width = extent,
                                                       .height = extent,
                                                       .radius = profile ? extent / 2 : radius},
                                             palette, style, &state->motion[row * 3], &wake);
            } else if (profile) {
                cui_avatar(ui, cui_id_from(ui, "photo"), theme, cui_string("Sam Patel"), extent);
            } else {
                cui_begin_box(ui, cui_id_from(ui, "number"),
                              (cui_box){.width = extent,
                                        .height = extent,
                                        .radius = radius,
                                        .background = theme->muted_background,
                                        .align_x = CUI_ALIGN_CENTER,
                                        .align_y = CUI_ALIGN_CENTER});
                cui_text(ui, cui_string(numbers[row]), rem * .8f, theme->muted);
                cui_end_box(ui);
            }
            cui_begin_box(ui, cui_id_from(ui, "lines"),
                          (cui_box){.width = -1,
                                    .gap = (loading && profile ? .5f : .25f) * rem,
                                    .clip = true});
            if (loading) {
                skeleton_example_placeholder(ui, "title",
                                             (cui_box){.width = profile ? .65f : .8f - .1f * row,
                                                       .width_percent = true,
                                                       .height = rem,
                                                       .radius = radius},
                                             palette, style, &state->motion[row * 3 + 1], &wake);
                skeleton_example_placeholder(ui, "subtitle",
                                             (cui_box){.width = .4f,
                                                       .width_percent = true,
                                                       .height = .625f * rem,
                                                       .radius = radius},
                                             palette, style, &state->motion[row * 3 + 2], &wake);
            } else {
                const char *title = profile ? "Sam Patel" : titles[row];
                cui_text_line(ui, cui_string(title), rem, theme->text, 0, (cui_text_style){0});
                cui_text_line(ui, cui_string(profile ? "Product designer" : subtitles[row]),
                              rem * .8125f, theme->muted, 0, (cui_text_style){0});
            }
            cui_end_box(ui);
            cui_end_box(ui);
            cui_pop_id(ui);
        }
        cui_end_box(ui);
    }
    if (loading && layout == SKELETON_EXAMPLE_PROFILE) {
        skeleton_example_placeholder(
            ui, "description",
            (cui_box){.width = 1, .width_percent = true, .height = 1.75f * rem, .radius = radius},
            palette, style, &state->motion[3], &wake);
    } else {
        cui_id status = cui_id_from(ui, "completion");
        cui_begin_box(ui, status, (cui_box){.height = 1.75f * rem, .align_y = CUI_ALIGN_CENTER});
        if (!loading) {
            skeleton_example_semantics(ui, status, CUI_ROLE_STATUS, statuses[layout]);
            cui_text(ui, cui_string(statuses[layout]), rem * .875f, theme->muted);
        }
        cui_end_box(ui);
    }
    cui_end_box(ui);
    cui_pop_id(ui);
    return wake;
}

#endif
