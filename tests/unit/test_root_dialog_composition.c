/* SPDX-License-Identifier: MIT */
#include "cui/cui_dialog_composition_styled.h"
#include "cui/cui_root.h"
#include <assert.h>
#include <math.h>

enum {
    APP_TRIGGER = 100,
    PARENT_DIALOG = 200,
    PARENT_HEADER,
    PARENT_BODY,
    REVIEW,
    ALERT_DIALOG = 300,
    CONFIRM,
    LEGACY_DIALOG = 400,
    LEGACY_BUTTON,
    ROOT_ID = 900
};

typedef struct {
    cui_root root;
    cui_root_surface stack[2];
    cui_theme theme;
    cui_surface_state parent, alert;
    cui_dialog_layout_v1 parent_layout, alert_layout;
    cui_root_composed_dialog_v1 parent_entry, alert_entry;
    cui_dialog_composition_v1 parent_content, alert_content;
    unsigned application_frames, headers, bodies, footers, alerts, sizing_calls;
    unsigned decisions, closes;
    bool veto;
    cui_surface_result last_reason;
} root_fixture;

static void root_app(cui_context *ui, void *user) {
    root_fixture *f = user;
    ++f->application_frames;
    if (cui_button(ui, APP_TRIGGER, &f->theme, cui_string("Edit settings"), false))
        assert(cui_root_dialog_open_composed_v1(&f->root, ui, &f->parent_entry, APP_TRIGGER));
}

static void parent_header(cui_context *ui, const cui_dialog_frame *frame, void *user) {
    (void)frame;
    root_fixture *f = user;
    ++f->headers;
    cui_begin_box(ui, PARENT_HEADER, (cui_box){.width = -1, .height = 18});
    cui_end_box(ui);
}

static void parent_body(cui_context *ui, const cui_dialog_frame *frame, void *user) {
    (void)frame;
    root_fixture *f = user;
    ++f->bodies;
    cui_begin_box(ui, PARENT_BODY, (cui_box){.width = -1, .height = 640, .fixed_height = true});
    cui_end_box(ui);
}

static void parent_footer(cui_context *ui, const cui_dialog_frame *frame, void *user) {
    root_fixture *f = user;
    (void)frame;
    ++f->footers;
    if (cui_button(ui, REVIEW, &f->theme, cui_string("Review changes"), false))
        assert(cui_root_dialog_open_composed_v1(&f->root, ui, &f->alert_entry, REVIEW));
}

static void alert_footer(cui_context *ui, const cui_dialog_frame *frame, void *user) {
    root_fixture *f = user;
    ++f->alerts;
    (void)cui_dialog_action_v1(ui, frame, &f->theme, CONFIRM, cui_string("Apply changes"),
                               CUI_DIALOG_CONFIRM,
                               (cui_button_options){.variant = CUI_BUTTON_PRIMARY});
}

static void legacy_content(cui_context *ui, void *user) {
    root_fixture *f = user;
    (void)cui_button(ui, LEGACY_BUTTON, &f->theme, cui_string("Legacy control"), false);
}

static void refine_size(cui_rect viewport, cui_dialog_sizing_v1 *sizing, void *user) {
    root_fixture *f = user;
    assert(viewport.w > 0 && sizing->width == 420 && sizing->placement == CUI_DIALOG_SOURCE_TOP);
    ++f->sizing_calls;
    sizing->width = 360;
    sizing->placement = CUI_DIALOG_CENTERED;
}

static bool decide(void *user, cui_surface_result reason) {
    root_fixture *f = user;
    assert(reason != CUI_SURFACE_NONE);
    ++f->decisions;
    return !f->veto;
}

static void closed(void *user, cui_surface_result reason) {
    root_fixture *f = user;
    ++f->closes;
    f->last_reason = reason;
}

static void init(root_fixture *f) {
    f->theme = cui_theme_light();
    assert(cui_root_init(&f->root, NULL, 0, NULL, NULL));
    assert(cui_root_bind_facilities(&f->root, f->stack, 2, NULL));
    cui_surface_init(&f->parent, PARENT_DIALOG, CUI_SURFACE_DIALOG);
    cui_surface_init(&f->alert, ALERT_DIALOG, CUI_SURFACE_ALERT);
    assert(cui_dialog_layout_init_v1(&f->parent_layout, &f->parent));
    assert(cui_dialog_layout_init_v1(&f->alert_layout, &f->alert));
    f->parent_content = (cui_dialog_composition_v1){
        .struct_size = sizeof f->parent_content,
        .version = CUI_DIALOG_COMPOSITION_VERSION_1,
        .present = (1u << CUI_DIALOG_HEADER) | (1u << CUI_DIALOG_BODY) | (1u << CUI_DIALOG_FOOTER),
        .name = cui_string("Settings"),
        .description = cui_string("Edit workspace preferences"),
        .slots = {[CUI_DIALOG_HEADER] = {parent_header, f},
                  [CUI_DIALOG_BODY] = {parent_body, f},
                  [CUI_DIALOG_FOOTER] = {parent_footer, f}}};
    f->alert_content =
        (cui_dialog_composition_v1){.struct_size = sizeof f->alert_content,
                                    .version = CUI_DIALOG_COMPOSITION_VERSION_1,
                                    .present = 1u << CUI_DIALOG_FOOTER,
                                    .name = cui_string("Confirm changes"),
                                    .slots = {[CUI_DIALOG_FOOTER] = {alert_footer, f}}};
    f->parent_entry = (cui_root_composed_dialog_v1){.layout = &f->parent_layout,
                                                    .options = {.margin = 12},
                                                    .composition = &f->parent_content,
                                                    .refine_sizing = refine_size,
                                                    .sizing_user = f};
    f->alert_entry = (cui_root_composed_dialog_v1){.layout = &f->alert_layout,
                                                   .options = {.margin = 12,
                                                               .decide = decide,
                                                               .decision_user = f,
                                                               .on_close = closed,
                                                               .close_user = f},
                                                   .composition = &f->alert_content};
}

static void render(cui_context *ui, root_fixture *f, float width, bool expected) {
    assert(cui_begin_frame(ui, width, 360, .016f));
    assert(cui_root_render(ui, ROOT_ID, &f->root, &f->theme, (cui_rect){0, 0, width, 360}, NULL,
                           root_app, f, 0) == expected);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == (expected ? CUI_OK : CUI_ERROR_INVALID_ACTION));
}

static void click(cui_context *ui, cui_id id) {
    cui_rect hit = cui_bounds(ui, id);
    assert(hit.w > 0 && hit.h > 0);
    float x = hit.x + hit.w * .5f, y = hit.y + hit.h * .5f;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = x, .y = y}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = x, .y = y}));
}

static bool has_role(cui_context *ui, cui_id id, cui_role role) {
    size_t count;
    const cui_semantic *items = cui_semantics(ui, &count);
    for (size_t i = 0; i < count; ++i)
        if (items[i].id == id && items[i].role == role)
            return true;
    return false;
}

int main(void) {
    root_fixture f = {0};
    init(&f);
    cui_context *ui = cui_create((cui_config){.width = 600, .height = 360});
    assert(ui);
    render(ui, &f, 600, true);
    assert(f.application_frames == 1 && f.headers == 0);
    assert(!cui_root_dialog_close_top(&f.root, ui, CUI_SURFACE_CLOSED));

    click(ui, APP_TRIGGER);
    render(ui, &f, 600, true);
    assert(f.parent.overlay.open && f.root.dialog_count == 1 && f.headers == 1 && f.bodies == 1 &&
           f.footers == 1 && f.sizing_calls == 1);
    assert(has_role(ui, PARENT_DIALOG, CUI_ROLE_DIALOG));
    assert(f.parent_layout.completed[0].w == 360);
    assert(f.parent_layout.completed[5].h < f.parent_layout.completed[6].h);
    assert(f.parent_layout.completed[7].y >=
           f.parent_layout.completed[5].y + f.parent_layout.completed[5].h);
    assert(!cui_root_dialog_open_composed_v1(&f.root, ui, &f.parent_entry, APP_TRIGGER));

    /* A window resize is reflected on the first new frame. */
    render(ui, &f, 290, true);
    cui_rect panel = f.parent_layout.completed[0];
    assert(panel.w == 266 && panel.x == 12);
    assert(f.headers == 2 && f.sizing_calls == 2);
    assert(fabsf(cui_bounds(ui, REVIEW).y - f.parent_layout.completed[7].y) < 10);

    /* Both renderers share capacity, order and deferred top-close admission. */
    cui_surface_state legacy;
    cui_surface_init(&legacy, LEGACY_DIALOG, CUI_SURFACE_DIALOG);
    assert(cui_root_dialog_open(
        &f.root, ui, (cui_root_surface){.surface = &legacy, .content = legacy_content, .user = &f},
        REVIEW));
    assert(!cui_root_dialog_open_composed_v1(&f.root, ui, &f.alert_entry, REVIEW));
    assert(!f.alert.overlay.open);
    render(ui, &f, 290, true);
    assert(has_role(ui, LEGACY_DIALOG, CUI_ROLE_DIALOG) && cui_bounds(ui, LEGACY_BUTTON).w > 0);
    assert(cui_root_dialog_close_top(&f.root, ui, CUI_SURFACE_CLOSED));
    render(ui, &f, 290, true);
    assert(!legacy.overlay.open && f.parent.overlay.open);

    click(ui, REVIEW);
    render(ui, &f, 290, true);
    assert(f.alert.overlay.open && !has_role(ui, ALERT_DIALOG, CUI_ROLE_ALERTDIALOG) && !f.alerts);
    render(ui, &f, 290, true);
    assert(has_role(ui, ALERT_DIALOG, CUI_ROLE_ALERTDIALOG) && f.alerts == 1);

    f.veto = true;
    assert(cui_root_dialog_close_top(&f.root, ui, CUI_SURFACE_CANCELLED));
    render(ui, &f, 290, true);
    assert(f.alert.overlay.open && f.decisions == 1 && !f.closes);
    f.veto = false;
    cui_focus(ui, CONFIRM);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_ENTER}));
    render(ui, &f, 290, true);
    assert(!f.alert.overlay.open && f.parent.overlay.open && f.root.dialog_count == 1);
    assert(!f.stack[1].surface && f.closes == 1 && f.last_reason == CUI_SURFACE_ACCEPTED);
    assert(cui_focused(ui) == REVIEW);

    f.parent_content.version = 2;
    render(ui, &f, 290, false);
    f.parent_content.version = CUI_DIALOG_COMPOSITION_VERSION_1;
    cui_clear_error(ui);
    render(ui, &f, 290, true);
    assert(f.parent.overlay.open && f.root.dialog_count == 1);

    assert(cui_root_dialog_close_top(&f.root, ui, CUI_SURFACE_CLOSED));
    render(ui, &f, 290, true);
    assert(!f.parent.overlay.open && !f.root.dialog_count && !f.stack[0].surface);
    assert(cui_focused(ui) == APP_TRIGGER);
    cui_root_dispose(&f.root);
    cui_destroy(ui);
    return 0;
}
