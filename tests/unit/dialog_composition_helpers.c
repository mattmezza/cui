/* SPDX-License-Identifier: MIT */
#include "dialog_composition_helpers.h"
static bool decision(void *user, cui_surface_result result) {
    dialog_fixture *f = user;
    ++f->decisions;
    f->decision = result;
    return !f->veto;
}
static void closed(void *user, cui_surface_result result) {
    dialog_fixture *f = user;
    ++f->closes;
    f->closed_reason = result;
    assert(!f->surface.overlay.open);
    f->focus_closed = cui_focused(f->ui);
}
static void clicked(void *user) {
    ++((dialog_fixture *)user)->click_callbacks;
}
static void slot(cui_context *ui, const cui_dialog_frame *token, void *user) {
    dialog_slot_data *d = user;
    dialog_fixture *f = d->f;
    ++f->calls[d->kind];
    ++f->total_calls[d->kind];
    cui_dialog_slot_style_v1 resolved;
    assert(cui_dialog_slot_style_v1_get(token, (cui_dialog_slot_kind)d->kind, &resolved));
    if (f->slot_text[d->kind]) {
        assert(cui_text_paragraph_colored(ui, cui_string(f->slot_text[d->kind]), resolved.font_size,
                                          resolved.foreground, resolved.font_size * 1.25f,
                                          (cui_text_style){0}, true, NULL, 0));
        return;
    }
    if (f->text_title && d->kind == CUI_DIALOG_TITLE) {
        assert(cui_text_paragraph_colored(
            ui,
            cui_string(f->title_text
                           ? f->title_text
                           : "Current intrinsic title wraps across narrow windows without "
                             "replaying application callbacks or waiting for an earlier frame."),
            resolved.font_size, resolved.foreground, resolved.font_size * 1.25f,
            (cui_text_style){.weight = CUI_TEXT_WEIGHT_SEMIBOLD}, true, NULL, 0));
        return;
    }
    if (f->action_footer && d->kind == CUI_DIALOG_FOOTER) {
        cui_begin_box(ui, 102, (cui_box){.width = -1, .height = 40, .fixed_height = true});
        if (f->styled) {
            if (cui_dialog_action_v1(
                    ui, token, &f->theme, 101, cui_string("Accept"), CUI_DIALOG_CONFIRM,
                    (cui_button_options){
                        .width = 100, .height = 24, .on_click = clicked, .action_user = f}))
                ++f->actions;
        } else {
            cui_begin_box(ui, 101, (cui_box){.width = 100, .height = 24, .fixed_height = true});
            if (cui_base_dialog_action_v1(ui, token, 101, cui_string("Accept"), CUI_DIALOG_CONFIRM,
                                          false)
                    .activated)
                ++f->actions;
            cui_end_box(ui);
        }
        cui_end_box(ui);
        return;
    }
    cui_begin_box(ui, 200 + d->kind,
                  (cui_box){.width = -1,
                            .height = f->metric[d->kind],
                            .fixed_height = true,
                            .background = {(uint8_t)(30 + d->kind * 30), 0, 0, 255}});
    cui_end_box(ui);
    if (f->poison && d->kind == CUI_DIALOG_BODY)
        f->surface.body_scroll = NAN;
}
void dialog_fixture_init(dialog_fixture *f, bool styled, cui_font *font) {
    memset(f, 0, sizeof *f);
    f->styled = styled;
    f->font = font;
    f->width = 400;
    f->height = 300;
    cui_config config = {.width = 400, .height = 300};
    if (font) {
        config.measure = cui_font_measure;
        config.measure_styled = cui_font_measure_styled;
        config.measure_user = font;
        config.font_metrics = cui_font_get_metrics;
        config.text_geometry = cui_font_text_geometry;
        config.text_geometry_line = cui_font_text_geometry_line;
        config.text_capabilities = cui_font_text_capabilities();
    }
    f->ui = cui_create(config);
    assert(f->ui);
    f->theme = cui_theme_light();
    cui_surface_init(&f->surface, 50, CUI_SURFACE_DIALOG);
    assert(cui_dialog_layout_init_v1(&f->layout, &f->surface));
    f->options = (cui_surface_options){.viewport = {0, 0, 400, 300},
                                       .margin = 16,
                                       .hide_close = true,
                                       .decide = decision,
                                       .decision_user = f,
                                       .on_close = closed,
                                       .close_user = f};
    assert(cui_dialog_sizing_default_v1(f->options, &f->sizing));
    f->sizing.width = 240;
    f->sizing.mode = CUI_SCROLL_BODY_EXPLICIT;
    f->sizing.requested_height = 200;
    f->composition = (cui_dialog_composition_v1){.struct_size = sizeof f->composition,
                                                 .version = 1,
                                                 .name = {"Owner", 5},
                                                 .omit = (1u << CUI_DIALOG_TITLE) |
                                                         (1u << CUI_DIALOG_DESCRIPTION)};
    for (unsigned i = 0; i < CUI_DIALOG_SLOT_COUNT; ++i) {
        f->slot[i] = (dialog_slot_data){f, i};
        f->composition.slots[i] = (cui_dialog_slot){slot, &f->slot[i]};
    }
    f->refinement = (cui_dialog_refinement_v1){.struct_size = sizeof f->refinement, .version = 1};
    f->composition.refinement = &f->refinement;
    assert(cui_dialog_open_v1(f->ui, &f->layout, 1));
}
void dialog_fixture_destroy(dialog_fixture *f) {
    cui_destroy(f->ui);
}
const cui_semantic *dialog_semantic(dialog_fixture *f, cui_id id) {
    size_t count;
    const cui_semantic *s = cui_semantics(f->ui, &count);
    for (size_t i = 0; i < count; ++i)
        if (s[i].id == id)
            return &s[i];
    return NULL;
}
cui_draw_list dialog_fixture_frame(dialog_fixture *f) {
    f->options.viewport = (cui_rect){0, 0, f->width, f->height};
    memset(f->calls, 0, sizeof f->calls);
    assert(cui_begin_frame(f->ui, f->width, f->height, .016f));
    cui_begin_box(f->ui, 3,
                  (cui_box){.width = f->width,
                            .height = f->height,
                            .inert = f->inert,
                            .layout_only = f->layout_only});
    cui_begin_box(f->ui, 1, (cui_box){.width = 30, .height = 20});
    cui_interact(f->ui, 1, CUI_ROLE_BUTTON, cui_string("Trigger"), false, false, 0);
    cui_end_box(f->ui);
    if (!f->omit_submit) {
        cui_dialog_submit_result result =
            f->styled ? cui_dialog_compose_v1(f->ui, &f->theme, &f->layout, f->options, &f->sizing,
                                              &f->composition)
                      : cui_base_dialog_compose_v1(f->ui, &f->layout, f->options, &f->sizing,
                                                   &f->composition);
        if (!f->poison) {
            if (!result.valid)
                fprintf(stderr,
                        "compose invalid: styled=%d sequence=%llu error=%d open=%d actions=%u "
                        "decisions=%u scroll=%g\n",
                        f->styled, (unsigned long long)cui_frame_sequence(f->ui),
                        cui_last_error(f->ui), f->surface.overlay.open, f->actions, f->decisions,
                        f->surface.body_scroll);
            if (!result.valid) {
                cui_diagnostic diagnostic;
                if (cui_first_diagnostic(f->ui, &diagnostic))
                    fprintf(stderr, "diagnostic operation=%s message=%s id=%llu detail=%d\n",
                            diagnostic.operation, diagnostic.message,
                            (unsigned long long)diagnostic.id, diagnostic.detail);
            }
            assert(result.valid);
        }
    }
    cui_end_box(f->ui);
    cui_draw_list draw = cui_end_frame(f->ui);
    if (!f->poison) {
        if (cui_last_error(f->ui) != CUI_OK)
            fprintf(stderr, "component error%d\n", cui_last_error(f->ui));
        assert(cui_last_error(f->ui) == CUI_OK);
    }
    return draw;
}
void dialog_fixture_pointer(dialog_fixture *f, cui_id id) {
    cui_rect b = cui_bounds(f->ui, id);
    assert(b.w > 0 && b.h > 0);
    assert(cui_queue_event(
        f->ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = b.x + 2, .y = b.y + 2}));
    assert(cui_queue_event(f->ui,
                           (cui_event){.type = CUI_EVENT_POINTER_UP, .x = b.x + 2, .y = b.y + 2}));
}
