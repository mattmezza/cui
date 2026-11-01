/* SPDX-License-Identifier: MIT */
/* Real controls reproduce the source FLEX1 allocation seen in the gallery.
 * FIT is an explicit application policy, not a replacement Core/default rule. */
#include "dialog_composition_helpers.h"
typedef struct {
    dialog_fixture *fixture;
    bool updates, long_body;
} short_model;
static const char *status_text = "Changes apply to this workspace.";
static void real_header(cui_context *ui, const cui_dialog_frame *frame, void *user) {
    (void)frame;
    short_model *m = user;
    ++m->fixture->calls[CUI_DIALOG_HEADER];
    cui_label(ui, &m->fixture->theme, cui_string("Workspace"));
}
static void real_status(cui_context *ui, const cui_dialog_frame *frame, void *user) {
    (void)frame;
    short_model *m = user;
    ++m->fixture->calls[CUI_DIALOG_CONTENT];
    cui_label(ui, &m->fixture->theme, cui_string(status_text));
}
static void real_body(cui_context *ui, const cui_dialog_frame *frame, void *user) {
    (void)frame;
    short_model *m = user;
    cui_theme *t = &m->fixture->theme;
    ++m->fixture->calls[CUI_DIALOG_BODY];
    cui_checkbox(ui, 310, t, cui_string("Product updates"), &m->updates, false);
    (void)cui_button(ui, 311, t, cui_string("Advanced preferences"), false);
    (void)cui_text_paragraph_colored(
        ui,
        cui_string(
            "Body content wraps at the current width and scrolls above the independent Footer."),
        t->font_size, t->text, t->font_size * 1.25f, (cui_text_style){0}, true, NULL, 0);
    if (m->long_body)
        for (unsigned i = 0; i < 8; ++i)
            (void)cui_text_paragraph_colored(
                ui,
                cui_string("Release notes explain new workspace features, security alerts and team "
                           "updates. "
                           "Delivery preferences can be changed at any time."),
                t->font_size, t->text, t->font_size * 1.25f, (cui_text_style){0}, true, NULL, 0);
}
static void real_footer(cui_context *ui, const cui_dialog_frame *frame, void *user) {
    short_model *m = user;
    ++m->fixture->calls[CUI_DIALOG_FOOTER];
    (void)cui_dialog_action_v1(ui, frame, &m->fixture->theme, 312, cui_string("Save changes"),
                               CUI_DIALOG_CONFIRM, (cui_button_options){0});
}
static float range(dialog_fixture *f) {
    const cui_semantic *s = dialog_semantic(f, cui_surface_part_id(50, 6));
    assert(s && s->details.scrollable);
    dialog_equal(s->details.scroll_max_y,
                 fmaxf(0, f->layout.completed[6].h - f->layout.completed[5].h),
                 "current completed scroll range");
    return s->details.scroll_max_y;
}
static void prepare(dialog_fixture *f, short_model *m, cui_font *font, bool fit) {
    dialog_fixture_init(f, true, font);
    *m = (short_model){.fixture = f, .updates = true};
    f->width = 860;
    f->height = 480;
    f->options.viewport = (cui_rect){0, 0, f->width, f->height};
    assert(cui_dialog_sizing_default_v1(f->options, &f->sizing));
    if (fit)
        f->sizing.content_sizing.mode = CUI_SCROLL_REGION_FIT;
    f->composition.name = cui_string("Notification preferences");
    f->composition.description = cui_string("Choose how CUI should keep you informed.");
    f->composition.omit = 0;
    f->composition.present = (1u << CUI_DIALOG_HEADER) | (1u << CUI_DIALOG_CONTENT) |
                             (1u << CUI_DIALOG_BODY) | (1u << CUI_DIALOG_FOOTER);
    f->composition.slots[CUI_DIALOG_HEADER] = (cui_dialog_slot){real_header, m};
    f->composition.slots[CUI_DIALOG_CONTENT] = (cui_dialog_slot){real_status, m};
    f->composition.slots[CUI_DIALOG_BODY] = (cui_dialog_slot){real_body, m};
    f->composition.slots[CUI_DIALOG_FOOTER] = (cui_dialog_slot){real_footer, m};
}
/* Independent ASCII word-fit oracle using the actual font provider. The status
 * is allowed to wrap; FIT must preserve that current intrinsic line height. */
static float status_height(dialog_fixture *f) {
    size_t length = strlen(status_text), line = 0;
    unsigned lines = 1;
    float width = f->layout.completed[4].w - 32; /* explicit default16px side insets */
    assert(width > 0);
    for (size_t begin = 0; begin < length;) {
        size_t end = begin;
        while (end < length && status_text[end] != ' ')
            ++end;
        cui_str candidate = {status_text + line, end - line};
        if (begin > line &&
            cui_font_measure(f->font, candidate, f->theme.font_size).width > width) {
            ++lines;
            line = begin;
        }
        begin = end + (end < length);
    }
    return (float)lines *
           cui_font_measure(f->font, cui_string(status_text), f->theme.font_size).height;
}
static void check_fit(dialog_fixture *f) {
    float intrinsic = status_height(f);
    dialog_equal(f->layout.completed[4].h, intrinsic, "FIT status intrinsic height");
    assert(range(f) == 0);
    assert(f->layout.completed[0].h < f->height - 32);
    const cui_semantic *action = dialog_semantic(f, 312);
    assert(action && action->bounds.y >= f->layout.completed[7].y);
    assert(action->bounds.y + action->bounds.h <=
           f->layout.completed[0].y + f->layout.completed[0].h);
    for (unsigned i = 0; i < CUI_DIALOG_SLOT_COUNT; ++i)
        assert(f->calls[i] == ((f->composition.present >> i) & 1u));
}
static void idle_sequence(cui_font *font) {
    dialog_fixture f;
    short_model model;
    prepare(&f, &model, font, true);
    model.long_body = true;
    f.theme.scrollbar = (cui_scrollbar_style){
        .visibility = CUI_SCROLLBAR_AUTO_HIDE, .idle_seconds = .032f, .fade_seconds = .064f};
    dialog_fixture_frame(&f);
    assert(cui_focused(f.ui) == 310); /* actual focused child; this is not a Body focus scope */
    for (unsigned i = 0; i < 12; ++i)
        dialog_fixture_frame(&f);
    fprintf(stderr, "idle witness: dirty=%d focus=%llu scroll=%g maximum=%g\n",
            cui_needs_redraw(f.ui), (unsigned long long)cui_focused(f.ui), f.surface.body_scroll,
            range(&f));
    assert(!cui_needs_redraw(f.ui));
    cui_rect b = f.layout.completed[5];
    assert(cui_queue_event(
        f.ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = b.x + 20, .y = b.y + 20}));
    assert(cui_queue_event(
        f.ui, (cui_event){.type = CUI_EVENT_SCROLL, .x = b.x + 20, .y = b.y + 20, .dy = -2}));
    dialog_fixture_frame(&f);
    float scroll = f.surface.body_scroll;
    assert(scroll > 0);
    for (unsigned i = 0; i < 12; ++i)
        dialog_fixture_frame(&f);
    assert(!cui_needs_redraw(f.ui) && f.surface.body_scroll == scroll);
    assert(cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 1, .y = 1}));
    for (unsigned i = 0; i < 12; ++i)
        dialog_fixture_frame(&f);
    assert(!cui_needs_redraw(f.ui) && f.surface.body_scroll == scroll);
    /* Current geometry still changes and requests its ordinary settling frame. */
    f.sizing.width = 280;
    dialog_fixture_frame(&f);
    assert(cui_needs_redraw(f.ui));
    for (unsigned i = 0; i < 12; ++i)
        dialog_fixture_frame(&f);
    assert(!cui_needs_redraw(f.ui));
    f.composition.present &= ~((1u << CUI_DIALOG_BODY) | (1u << CUI_DIALOG_CONTENT) |
                               (1u << CUI_DIALOG_HEADER) | (1u << CUI_DIALOG_FOOTER));
    f.composition.omit |= (1u << CUI_DIALOG_TITLE) | (1u << CUI_DIALOG_DESCRIPTION);
    dialog_fixture_frame(&f);
    for (unsigned i = 1; i <= 7; ++i)
        assert(f.layout.completed[i].w == 0 && f.layout.completed[i].h == 0);
    for (unsigned i = 0; i < 12; ++i)
        dialog_fixture_frame(&f);
    assert(!cui_needs_redraw(f.ui));
    cui_overlay_close(f.ui, &f.surface.overlay);
    assert(cui_dialog_open_v1(f.ui, &f.layout, 0));
    for (unsigned i = 0; i < 10; ++i)
        assert(f.layout.completed[i].w == 0 && f.layout.completed[i].h == 0);
    dialog_fixture_destroy(&f);
}
int main(int argc, char **argv) {
    assert(argc == 3 || (argc == 4 && strcmp(argv[3], "--idle") == 0));
    cui_font *font = cui_font_open(argv[1]);
    assert(font && cui_font_register_style(
                       font, (cui_text_style){.weight = CUI_TEXT_WEIGHT_SEMIBOLD}, argv[2]));
    if (argc == 4) {
        idle_sequence(font);
        cui_font_close(font);
        return 0;
    }
    dialog_fixture f;
    short_model model;
    prepare(&f, &model, font, false);
    dialog_fixture_frame(&f); /* First-open default source FLEX1 control. */
    fprintf(stderr, "source FLEX1 control: panel=%g Content=%g Body=%g natural=%g maximum=%g\n",
            f.layout.completed[0].h, f.layout.completed[4].h, f.layout.completed[5].h,
            f.layout.completed[6].h, range(&f));
    assert(range(&f) > 0); /* Original browser counterexample stays witnessed. */
    dialog_fixture_destroy(&f);
    prepare(&f, &model, font, true);
    dialog_fixture_frame(&f); /* First-presented FIT policy, no warm-up. */
    check_fit(&f);
    float short_panel = f.layout.completed[0].h;
    f.sizing.width = 280;
    dialog_fixture_frame(&f);
    check_fit(&f);
    f.sizing.width = 420;
    dialog_fixture_frame(&f);
    check_fit(&f);
    dialog_equal(f.layout.completed[0].h, short_panel, "same-frame width recovery");
    model.long_body = true;
    dialog_fixture_frame(&f);
    assert(range(&f) > 0);
    assert(f.layout.completed[0].h == f.sizing.max_height);
    float footer_y = f.layout.completed[7].y;
    cui_rect b = f.layout.completed[5];
    assert(cui_queue_event(
        f.ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = b.x + 20, .y = b.y + 20}));
    assert(cui_queue_event(
        f.ui, (cui_event){.type = CUI_EVENT_SCROLL, .x = b.x + 20, .y = b.y + 20, .dy = -2}));
    dialog_fixture_frame(&f);
    assert(f.surface.body_scroll > 0);
    dialog_equal(f.layout.completed[7].y, footer_y, "long Body scroll preserves Footer");
    model.long_body = false;
    dialog_fixture_frame(&f);
    check_fit(&f);
    assert(f.surface.body_scroll == 0); /* Current frame clamp, not next frame. */
    dialog_equal(f.layout.completed[0].h, short_panel, "same-frame body recovery");
    dialog_fixture_destroy(&f);
    cui_font_close(font);
    return 0;
}
