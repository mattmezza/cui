/* SPDX-License-Identifier: MIT */
#define CUI_BRIDGE_BUILD 1
#define CUI_BRIDGE_WITH_SDL 1
#include "../../src/bridge/cui_bridge.c"
#include <assert.h>

/* Simulated host exercises actual step/poll/end/render/finish; no SDL/hardware. */
static unsigned polls, renders;
static cui_bridge_status render_status;
cui_bridge_status cui_bridge_native_create(cui_bridge_app *a, const char *t, const char *f,
                                            cui_config *c, bool h) {
    (void)a; (void)t; (void)f; (void)c; (void)h; return CUI_BRIDGE_OK;
}
void cui_bridge_native_destroy(cui_bridge_app *a) { (void)a; }
cui_bridge_status cui_bridge_native_poll(cui_bridge_app *a) { (void)a; polls++; return CUI_BRIDGE_OK; }
cui_bridge_status cui_bridge_native_render(cui_bridge_app *a, const cui_draw_list *l) {
    (void)a; assert(l && l->count); renders++; return render_status;
}
cui_bridge_status cui_bridge_native_run(cui_bridge_app *a, cui_bridge_run_callback c, void *u) {
    (void)a; (void)c; (void)u; return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
cui_bridge_status cui_bridge_native_set_font_fallbacks(cui_bridge_app *a, const char *const *p, uint32_t n) {
    (void)a; (void)p; (void)n; return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
cui_bridge_status cui_bridge_native_clipboard_set(cui_bridge_app *a, cui_str s) {
    (void)a; (void)s; return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
cui_bridge_status cui_bridge_native_clipboard_get(cui_bridge_app *a, char *p, uint32_t c, uint32_t *n) {
    (void)a; (void)p; (void)c; (void)n; return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
cui_bridge_status cui_bridge_native_set_window_visible(cui_bridge_app *a, uint32_t v) {
    (void)a; (void)v; return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
cui_bridge_status cui_bridge_native_get_window_visible(cui_bridge_app *a, uint32_t *v) {
    (void)a; (void)v; return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
static cui_bridge_node create(cui_bridge_app *a, cui_bridge_node p, cui_bridge_kind k, const char *key) {
    cui_bridge_node n;
    assert(cui_bridge_node_create(a, p, k, key, &n) == CUI_BRIDGE_OK);
    return n;
}
static cui_bridge_time_field_config_v1 config(void) {
    cui_bridge_time_field_config_v1 c = {.struct_size = sizeof c, .version = 1,
        .precision = CUI_BRIDGE_TIME_SECOND_V1, .edit_generation = 1};
    const char *tokens[] = {"0","1","2","3","4","5","6","7","8","9",":"," ","AM","PM"};
    for (unsigned i = 0; i < 14; i++) {
        c.tokens[i].length = (uint32_t)strlen(tokens[i]);
        memcpy(c.tokens[i].bytes, tokens[i], c.tokens[i].length);
    }
    return c;
}
static cui_bridge_time_field_change_v1 change(void) {
    return (cui_bridge_time_field_change_v1){.struct_size = sizeof(cui_bridge_time_field_change_v1), .version = 1};
}
static cui_bridge_time_field_result_v1 result(void) {
    return (cui_bridge_time_field_result_v1){.struct_size = sizeof(cui_bridge_time_field_result_v1), .version = 1};
}
static void step(cui_bridge_app *a) { assert(cui_bridge_step(a, .016f) == CUI_BRIDGE_OK); }
static void key(cui_bridge_app *a) {
    assert(cui_bridge_queue_key(a, CUI_BRIDGE_KEY_DOWN, CUI_BRIDGE_KEY_UP_ARROW, 0, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_key(a, CUI_BRIDGE_KEY_UP, CUI_BRIDGE_KEY_UP_ARROW, 0, 0) == CUI_BRIDGE_OK);
}
int main(void) {
    cui_bridge_options options = {.struct_size = sizeof options, .abi_version = 1,
        .width = 800, .height = 800, .backend = CUI_BRIDGE_BACKEND_SDL};
    cui_bridge_status status;
    cui_bridge_app *a = cui_bridge_create(&options, &status);
    assert(a && status == CUI_BRIDGE_OK);
    cui_bridge_node root = create(a, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root");
    cui_bridge_node row = create(a, root, CUI_BRIDGE_ROW, "row");
    cui_bridge_node field = create(a, row, CUI_BRIDGE_TIME_FIELD, "appointment");
    cui_bridge_node other = create(a, root, CUI_BRIDGE_TIME_FIELD, "other");
    assert(cui_bridge_set_time(a, field, (cui_bridge_time){9,45,30}) == CUI_BRIDGE_OK);
    assert(!a->slider && !a->time_fields);
    step(a); /* Existing frames before opt-in must still track actual sequence. */
    assert(cui_bridge_time_field_versions() == 1);
    assert(cui_bridge_time_field_enable_v1(a, 1) == CUI_BRIDGE_OK);
    assert(!a->slider);
    cui_bridge_time_field_config_v1 c = config();
    assert(cui_bridge_time_field_configure_v1(a, field, &c) == CUI_BRIDGE_OK);
    assert(cui_bridge_time_field_configure_v1(a, other, &c) == CUI_BRIDGE_CAPACITY);
    assert(a->time_fields->configured == 1 && !a->time_fields->nodes[other.slot]);
    cui_bridge_time_field_config_v1 bad = c;
    bad.tokens[1] = bad.tokens[0];
    assert(cui_bridge_time_field_configure_v1(a, field, &bad) == CUI_BRIDGE_INVALID_ARGUMENT);
    bad = c; bad.tokens[0].bytes[0] = 0xff;
    assert(cui_bridge_time_field_configure_v1(a, field, &bad) == CUI_BRIDGE_INVALID_UTF8);
    bad = c; bad.minimum.hour = 1;
    assert(cui_bridge_time_field_configure_v1(a, field, &bad) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_time_field_get_config_v1(a, field, &bad) == CUI_BRIDGE_OK);
    assert(memcmp(&bad, &c, sizeof c) == 0);
    memset(&c, 0xcc, sizeof c); /* The bridge copied every token byte. */
    step(a); step(a);
    cui_bridge_time_field_result_v1 r = result();
    assert(cui_bridge_time_field_get_result_v1(a, field, &r) == CUI_BRIDGE_OK);
    cui_focus(a->ui, cui_time_segment_id(a->nodes[field.slot].id, 0));
    assert(cui_focused(a->ui) == cui_time_segment_id(a->nodes[field.slot].id, 0));
    char paste[] = "12:34:56";
    assert(cui_bridge_queue_text(a, paste) == CUI_BRIDGE_OK);
    memset(paste, 'x', sizeof paste - 1);
    step(a);
    cui_bridge_time now;
    assert(cui_bridge_get_time(a, field, &now) == CUI_BRIDGE_OK);
    assert(now.hour == 12 && now.minute == 34 && now.second == 56);
    key(a);
    double time_before = a->now_seconds;
    unsigned polls_before = polls, renders_before = renders;
    unsigned char state_before[sizeof(cui_time_field_state_v1)];
    memcpy(state_before, &a->time_fields->nodes[field.slot]->state, sizeof state_before);
    assert(cui_bridge_step(a, .016f) == CUI_BRIDGE_CAPACITY);
    assert(a->now_seconds == time_before && polls == polls_before && renders == renders_before);
    assert(memcmp(state_before, &a->time_fields->nodes[field.slot]->state, sizeof state_before) == 0);
    cui_bridge_time_field_change_v1 e = change();
    assert(cui_bridge_time_field_poll_change_v1(a, &e) == CUI_BRIDGE_OK);
    assert(e.previous.hour == 9 && e.value.hour == 12 && e.value.second == 56 && e.frame_status == CUI_BRIDGE_OK);
    step(a); /* Previously blocked original key is handled exactly once. */
    e = change(); assert(cui_bridge_time_field_poll_change_v1(a, &e) == CUI_BRIDGE_OK);
    assert(e.value.hour == 13);
    key(a); render_status = CUI_BRIDGE_CORE_ERROR;
    assert(cui_bridge_step(a, .016f) == CUI_BRIDGE_CORE_ERROR);
    r = result(); unsigned char output_before[sizeof r]; memcpy(output_before, &r, sizeof r);
    assert(cui_bridge_time_field_get_result_v1(a, field, &r) == CUI_BRIDGE_CORE_ERROR);
    assert(memcmp(output_before, &r, sizeof r) == 0);
    e = change(); assert(cui_bridge_time_field_poll_change_v1(a, &e) == CUI_BRIDGE_OK);
    assert(e.value.hour == 14 && e.frame_status == CUI_BRIDGE_CORE_ERROR);
    assert(cui_bridge_get_time(a, field, &now) == CUI_BRIDGE_OK && now.hour == 14);
    render_status = CUI_BRIDGE_OK; step(a);
    assert(cui_bridge_time_field_get_result_v1(a, field, &r) == CUI_BRIDGE_OK);
    key(a); step(a);
    assert(cui_bridge_node_remove(a, field) == CUI_BRIDGE_OK);
    e = change(); assert(cui_bridge_time_field_poll_change_v1(a, &e) == CUI_BRIDGE_EMPTY);
    assert(a->time_fields->configured == 0);
    uint64_t last = a->last_core_frame;
    a->last_core_frame = UINT64_MAX;
    time_before = a->now_seconds;
    assert(cui_bridge_step(a, .016f) == CUI_BRIDGE_CAPACITY && a->now_seconds == time_before);
    a->last_core_frame = last;
    cui_bridge_destroy(a);
    puts("localized TimeField bridge: owned config, nested UI, copied paste, thread, preflight and failed-frame observations");
    return 0;
}
