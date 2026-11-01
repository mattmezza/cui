/* SPDX-License-Identifier: MIT */
#ifndef DIALOG_COMPOSITION_HELPERS_H
#define DIALOG_COMPOSITION_HELPERS_H
#include "cui/cui_dialog_composition_styled.h"
#include "cui/cui_software.h"
#include "cui/cui_text.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef struct dialog_fixture dialog_fixture;
typedef struct {
    dialog_fixture *f;
    unsigned kind;
} dialog_slot_data;
struct dialog_fixture {
    cui_context *ui;
    cui_surface_state surface;
    cui_dialog_layout_v1 layout;
    cui_dialog_composition_v1 composition;
    cui_dialog_sizing_v1 sizing;
    cui_dialog_refinement_v1 refinement;
    cui_surface_options options;
    cui_theme theme;
    cui_font *font;
    const char *title_text;
    const char *slot_text[CUI_DIALOG_SLOT_COUNT];
    dialog_slot_data slot[CUI_DIALOG_SLOT_COUNT];
    float metric[CUI_DIALOG_SLOT_COUNT];
    unsigned calls[CUI_DIALOG_SLOT_COUNT], total_calls[CUI_DIALOG_SLOT_COUNT];
    unsigned decisions, closes, actions, click_callbacks;
    cui_surface_result decision, closed_reason;
    cui_id focus_closed;
    float width, height;
    bool styled, veto, action_footer, text_title, omit_submit, inert, layout_only, poison;
};
void dialog_fixture_init(dialog_fixture *, bool styled, cui_font *);
void dialog_fixture_destroy(dialog_fixture *);
cui_draw_list dialog_fixture_frame(dialog_fixture *);
const cui_semantic *dialog_semantic(dialog_fixture *, cui_id);
void dialog_fixture_pointer(dialog_fixture *, cui_id);
void dialog_geometry_tests(const char *font_path);
void dialog_input_tests(void);
static inline void dialog_equal(float a, float b, const char *name) {
    if (fabsf(a - b) > .06f) {
        fprintf(stderr, "%s actual%g expected%g\n", name, a, b);
        assert(false);
    }
}
#endif
