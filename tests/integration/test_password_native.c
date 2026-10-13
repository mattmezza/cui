/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include "cui/cui_text.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
typedef struct {
    cui_font *font;
    bool secure, fail;
    unsigned calls, live, released;
} host;
typedef struct {
    cui_text_geometry native;
    host *owner;
} session;
static cui_text_layout_status carets(const void *p, size_t b, cui_text_caret *out, size_t cap,
                                     size_t *n) {
    const session *s = p;
    return s->native.ops->carets(s->native.session, b, out, cap, n);
}
static cui_text_layout_status hit(const void *p, float x, float y, cui_text_affinity a,
                                  cui_text_caret *out) {
    const session *s = p;
    return s->native.ops->hit_test(s->native.session, x, y, a, out);
}
static cui_text_layout_status selection(const void *p, size_t a, size_t b,
                                        cui_text_selection_rect *out, size_t cap, size_t *n) {
    const session *s = p;
    return s->native.ops->selection(s->native.session, a, b, out, cap, n);
}
static void release(void *p) {
    session *s = p;
    s->owner->live--;
    s->owner->released++;
    cui_release_text_geometry(&s->native);
    free(s);
}
static const cui_text_geometry_ops ops = {carets, hit, selection, release};
static cui_text_layout_status geometry(void *p, cui_str text, float size, cui_text_style style,
                                       cui_text_geometry *out) {
    host *h = p;
    h->calls++;
    if (h->secure) {
        assert(text.length % 3 == 0);
        for (size_t i = 0; i < text.length; i += 3)
            assert(!memcmp(text.data + i, "•", 3));
        if (h->fail)
            return CUI_TEXT_LAYOUT_CAPACITY;
    }
    session *s = malloc(sizeof *s);
    assert(s);
    s->owner = h;
    cui_text_layout_status result = cui_font_text_geometry(h->font, text, size, style, &s->native);
    if (result != CUI_TEXT_LAYOUT_OK) {
        free(s);
        return result;
    }
    *out = s->native;
    out->session = s;
    out->ops = &ops;
    h->live++;
    return result;
}
static cui_size measure(void *p, cui_str text, float size) {
    return cui_font_measure(((host *)p)->font, text, size);
}
static cui_draw_list list;
static cui_input_result frame(cui_context *ui, cui_edit *e, cui_input_state *s,
                              cui_input_options o) {
    assert(cui_begin_frame(ui, 400, 120, .016f));
    cui_theme t = cui_theme_light();
    cui_begin_box(ui, 1, (cui_box){.width = -1, .height = -1, .padding = 16});
    cui_input_result r = cui_input_ex(ui, 10, &t, cui_string("Password"), e, s, o);
    cui_end_box(ui);
    list = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return r;
}
typedef struct {
    uint64_t hash;
    unsigned pixels;
} raster;
static void glyph(void *p, int x, int y, int w, int h, int pitch, const unsigned char *bytes) {
    raster *r = p;
    r->hash = r->hash * 1099511628211ULL + (unsigned)x;
    r->hash = r->hash * 1099511628211ULL + (unsigned)y;
    for (int row = 0; row < h; row++)
        for (int col = 0; col < w; col++) {
            unsigned value = bytes[row * pitch + col];
            r->hash = r->hash * 1099511628211ULL + value;
            r->pixels += value != 0;
        }
}
int main(int argc, char **argv) {
    const char *font_path = argc > 1 ? argv[1] : getenv("CUI_TEST_FONT");
    if (!font_path || !font_path[0])
        return 77;
    cui_font *font = cui_font_open(font_path);
    if (!font)
        return 77;
    host h = {.font = font};
    cui_context *ui =
        cui_create((cui_config){.measure = measure, .measure_user = &h, .text_geometry = geometry});
    char buffer[128] = "aéx";
    cui_edit *e = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                    .capacity = sizeof buffer,
                                                    .history_bytes = 4096,
                                                    .max_history_steps = 16});
    assert(ui && e);
    cui_input_state state = {0};
    cui_input_options o = {0};
    assert(frame(ui, e, &state, o).error == CUI_EDIT_OK && h.live);
    unsigned oldreleased = h.released;
    h.secure = true;
    o.masked = true;
    o.content_type = CUI_TEXT_INPUT_PURPOSE_PASSWORD;
    assert(frame(ui, e, &state, o).error == CUI_EDIT_OK && h.released > oldreleased);
    cui_focus(ui, 10);
    assert(cui_edit_select(e, 1, 3) == CUI_EDIT_OK);
    h.fail = true;
    unsigned start_calls = h.calls;
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_COMPOSITION_START, .composition_target = 10}));
    assert(frame(ui, e, &state, o).error == CUI_EDIT_PRESENTATION &&
           !state.behavior.composition->active);
    assert(!strcmp(buffer, "aéx") && cui_edit_get_state(e).anchor == 1 &&
           cui_edit_get_state(e).cursor == 3 && h.calls == start_calls + 1);
    h.fail = false;
    start_calls = h.calls;
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_COMPOSITION_START, .composition_target = 10}));
    assert(frame(ui, e, &state, o).error == CUI_EDIT_OK && state.behavior.composition->active &&
           h.calls == start_calls + 1);
    bool start_painted = false;
    for (size_t i = 0; i < list.count; i++)
        if (list.commands[i].type == CUI_DRAW_TEXT && list.commands[i].text.length) {
            const cui_draw_command *c = &list.commands[i];
            assert(c->text.length == 6 && !memcmp(c->text.data, "••", 6));
            raster actual = {0};
            assert(cui_font_draw(font, c->text, c->font_size, 0, 0, glyph, &actual) &&
                   actual.pixels);
            start_painted = true;
        }
    assert(start_painted && !strcmp(buffer, "aéx"));
    start_calls = h.calls;
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_COMPOSITION_CANCEL, .composition_target = 10}));
    assert(frame(ui, e, &state, o).error == CUI_EDIT_OK && !state.behavior.composition->active &&
           h.calls == start_calls);
    assert(cui_edit_select(e, 3, 3) == CUI_EDIT_OK);
    frame(ui, e, &state, o);
    bool found = false;
    for (size_t i = 0; i < list.count; i++)
        if (list.commands[i].type == CUI_DRAW_TEXT && list.commands[i].text.length) {
            const cui_draw_command *c = &list.commands[i];
            assert(c->text.length == 9 && !memcmp(c->text.data, "•••", 9));
            raster actual = {0}, expected = {0};
            assert(cui_font_draw(font, c->text, c->font_size, 0, 0, glyph, &actual));
            assert(cui_font_draw(font, cui_string("•••"), c->font_size, 0, 0, glyph, &expected));
            assert(actual.pixels && actual.hash == expected.hash);
            found = true;
        }
    assert(found);
    cui_rect caret;
    assert(cui_text_input_rect(ui, &caret) && caret.h > 0 && caret.x > 16);
    unsigned calls = h.calls;
    frame(ui, e, &state, o);
    assert(h.calls == calls);
    cui_text_metrics_changed(ui);
    calls = h.calls;
    assert(frame(ui, e, &state, o).error == CUI_EDIT_OK && h.calls == calls + 1);
    h.fail = true;
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_TEXT, .text_data = "extra", .text_length = 5}));
    cui_edit_state before = cui_edit_get_state(e);
    assert(frame(ui, e, &state, o).error == CUI_EDIT_PRESENTATION);
    cui_edit_state after = cui_edit_get_state(e);
    assert(!strcmp(buffer, "aéx") && before.revision == after.revision &&
           before.anchor == after.anchor && before.cursor == after.cursor);
    h.fail = false;
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_TEXT, .text_data = "x", .text_length = 1}));
    assert(frame(ui, e, &state, o).changed);
    if (state.behavior.composition)
        assert(cui_edit_composition_event(e, state.behavior.composition,
                                          (cui_event){.type = CUI_EVENT_COMPOSITION_CANCEL},
                                          false) == CUI_EDIT_OK);
    cui_edit_destroy(e);
    strcpy(buffer, "aéx");
    state = (cui_input_state){0};
    e = cui_edit_create((cui_edit_config){.buffer = buffer,
                                          .capacity = sizeof buffer,
                                          .history_bytes = 4096,
                                          .max_history_steps = 16});
    assert(e);
    unsigned rebound_calls = h.calls, rebound_release = h.released;
    assert(frame(ui, e, &state, o).error == CUI_EDIT_OK && h.calls == rebound_calls + 1 &&
           h.released > rebound_release);
    cui_destroy(ui);
    assert(!h.live);
    cui_edit_destroy(e);
    cui_font_close(font);
    return 0;
}
