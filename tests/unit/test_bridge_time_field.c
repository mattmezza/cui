/* SPDX-License-Identifier: MIT */
#include <stdlib.h>
/* Scoped test-only allocator: existing context allocation is not intercepted. */
static int fail_next_bridge_allocation;
static void *test_bridge_calloc(size_t count, size_t size) {
    if (fail_next_bridge_allocation) {
        fail_next_bridge_allocation = 0;
        return NULL;
    }
    return calloc(count, size);
}
#define calloc test_bridge_calloc
#define CUI_BRIDGE_BUILD 1
#define CUI_BRIDGE_WITH_SDL 1
#include "../../src/bridge/cui_bridge.c"
#undef calloc
#include <assert.h>

/* Simulated host exercises actual step/poll/end/render/finish; no SDL/hardware. */
static unsigned polls, renders;
static cui_bridge_status render_status;
cui_bridge_status cui_bridge_native_create(cui_bridge_app *a, const char *t, const char *f,
                                           cui_config *c, bool h) {
    (void)a;
    (void)t;
    (void)f;
    (void)c;
    (void)h;
    return CUI_BRIDGE_OK;
}
void cui_bridge_native_destroy(cui_bridge_app *a) {
    (void)a;
}
cui_bridge_status cui_bridge_native_poll(cui_bridge_app *a) {
    (void)a;
    polls++;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_native_render(cui_bridge_app *a, const cui_draw_list *l) {
    (void)a;
    assert(l && l->count);
    renders++;
    return render_status;
}
cui_bridge_status cui_bridge_native_run(cui_bridge_app *a, cui_bridge_run_callback c, void *u) {
    (void)a;
    (void)c;
    (void)u;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
cui_bridge_status cui_bridge_native_set_font_fallbacks(cui_bridge_app *a, const char *const *p,
                                                       uint32_t n) {
    (void)a;
    (void)p;
    (void)n;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
cui_bridge_status cui_bridge_native_clipboard_set(cui_bridge_app *a, cui_str s) {
    (void)a;
    (void)s;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
cui_bridge_status cui_bridge_native_clipboard_get(cui_bridge_app *a, char *p, uint32_t c,
                                                  uint32_t *n) {
    (void)a;
    (void)p;
    (void)c;
    (void)n;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
cui_bridge_status cui_bridge_native_set_window_visible(cui_bridge_app *a, uint32_t v) {
    (void)a;
    (void)v;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
cui_bridge_status cui_bridge_native_get_window_visible(cui_bridge_app *a, uint32_t *v) {
    (void)a;
    (void)v;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
static cui_bridge_node create(cui_bridge_app *a, cui_bridge_node p, cui_bridge_kind k,
                              const char *key) {
    cui_bridge_node n;
    assert(cui_bridge_node_create(a, p, k, key, &n) == CUI_BRIDGE_OK);
    return n;
}
static cui_bridge_time_field_config_v1 config(void) {
    cui_bridge_time_field_config_v1 c = {.struct_size = sizeof c,
                                         .version = 1,
                                         .precision = CUI_BRIDGE_TIME_SECOND_V1,
                                         .edit_generation = 1};
    const char *tokens[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", " ", "AM", "PM"};
    for (unsigned i = 0; i < 14; i++) {
        c.tokens[i].length = (uint32_t)strlen(tokens[i]);
        memcpy(c.tokens[i].bytes, tokens[i], c.tokens[i].length);
    }
    return c;
}
static cui_bridge_time_field_change_v1 change(void) {
    return (cui_bridge_time_field_change_v1){.struct_size = sizeof(cui_bridge_time_field_change_v1),
                                             .version = 1};
}
static cui_bridge_time_field_result_v1 result(void) {
    return (cui_bridge_time_field_result_v1){.struct_size = sizeof(cui_bridge_time_field_result_v1),
                                             .version = 1};
}
static void step(cui_bridge_app *a) {
    assert(cui_bridge_step(a, .016f) == CUI_BRIDGE_OK);
}
static void key(cui_bridge_app *a) {
    assert(cui_bridge_queue_key(a, CUI_BRIDGE_KEY_DOWN, CUI_BRIDGE_KEY_UP_ARROW, 0, 0) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_queue_key(a, CUI_BRIDGE_KEY_UP, CUI_BRIDGE_KEY_UP_ARROW, 0, 0) ==
           CUI_BRIDGE_OK);
}
int main(void) {
    cui_bridge_options options = {.struct_size = sizeof options,
                                  .abi_version = 1,
                                  .width = 800,
                                  .height = 800,
                                  .backend = CUI_BRIDGE_BACKEND_SDL};
    cui_bridge_status status;
    cui_bridge_app *a = cui_bridge_create(&options, &status);
    assert(a && status == CUI_BRIDGE_OK);
    cui_bridge_node root = create(a, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root");
    cui_bridge_node row = create(a, root, CUI_BRIDGE_ROW, "row");
    cui_bridge_node field = create(a, row, CUI_BRIDGE_TIME_FIELD, "appointment");
    cui_bridge_node other = create(a, root, CUI_BRIDGE_TIME_FIELD, "other");
    assert(cui_bridge_set_time(a, field, (cui_bridge_time){9, 45, 30}) == CUI_BRIDGE_OK);
    assert(!a->slider && !a->time_fields);
    step(a); /* Existing frames before opt-in must still track actual sequence. */
    assert(cui_bridge_time_field_versions() == 1);
    fail_next_bridge_allocation = 1;
    assert(cui_bridge_time_field_enable_v1(a, 1) == CUI_BRIDGE_OUT_OF_MEMORY);
    assert(!a->time_fields && !a->slider);
    assert(cui_bridge_time_field_enable_v1(a, 1) == CUI_BRIDGE_OK);
    assert(!a->slider);
    cui_bridge_time_field_config_v1 c = config();
    fail_next_bridge_allocation = 1;
    assert(cui_bridge_time_field_configure_v1(a, field, &c) == CUI_BRIDGE_OUT_OF_MEMORY);
    assert(a->time_fields->configured == 0 && !a->time_fields->nodes[field.slot]);
    assert(cui_bridge_time_field_configure_v1(a, field, &c) == CUI_BRIDGE_OK);
    assert(cui_bridge_time_field_configure_v1(a, other, &c) == CUI_BRIDGE_CAPACITY);
    assert(a->time_fields->configured == 1 && !a->time_fields->nodes[other.slot]);
    cui_bridge_time_field_config_v1 bad = c;
    bad.tokens[1] = bad.tokens[0];
    assert(cui_bridge_time_field_configure_v1(a, field, &bad) == CUI_BRIDGE_INVALID_ARGUMENT);
    bad = c;
    bad.tokens[0].bytes[0] = 0xff;
    assert(cui_bridge_time_field_configure_v1(a, field, &bad) == CUI_BRIDGE_INVALID_UTF8);
    bad = c;
    bad.minimum.hour = 1;
    assert(cui_bridge_time_field_configure_v1(a, field, &bad) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_time_field_get_config_v1(a, field, &bad) == CUI_BRIDGE_OK);
    assert(memcmp(&bad, &c, sizeof c) == 0);
    memset(&c, 0xcc, sizeof c); /* The bridge copied every token byte. */
    step(a);
    step(a);
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
    assert(cui_bridge_set_disabled(a, field, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(a, .016f) == CUI_BRIDGE_CAPACITY);
    assert(cui_bridge_set_disabled(a, field, 0) == CUI_BRIDGE_OK);
    assert(a->now_seconds == time_before && polls == polls_before && renders == renders_before);
    assert(memcmp(state_before, &a->time_fields->nodes[field.slot]->state, sizeof state_before) ==
           0);
    cui_bridge_time_field_change_v1 e = change();
    assert(cui_bridge_time_field_poll_change_v1(a, &e) == CUI_BRIDGE_OK);
    assert(e.previous.hour == 9 && e.value.hour == 12 && e.value.second == 56 &&
           e.frame_status == CUI_BRIDGE_OK);
    step(a); /* Previously blocked original key is handled exactly once. */
    e = change();
    assert(cui_bridge_time_field_poll_change_v1(a, &e) == CUI_BRIDGE_OK);
    assert(e.value.hour == 13);
    key(a);
    render_status = CUI_BRIDGE_CORE_ERROR;
    assert(cui_bridge_step(a, .016f) == CUI_BRIDGE_CORE_ERROR);
    r = result();
    unsigned char output_before[sizeof r];
    memcpy(output_before, &r, sizeof r);
    assert(cui_bridge_time_field_get_result_v1(a, field, &r) == CUI_BRIDGE_CORE_ERROR);
    assert(memcmp(output_before, &r, sizeof r) == 0);
    e = change();
    assert(cui_bridge_time_field_poll_change_v1(a, &e) == CUI_BRIDGE_OK);
    assert(e.value.hour == 14 && e.frame_status == CUI_BRIDGE_CORE_ERROR);
    assert(cui_bridge_get_time(a, field, &now) == CUI_BRIDGE_OK && now.hour == 14);
    render_status = CUI_BRIDGE_OK;
    step(a);
    assert(cui_bridge_time_field_get_result_v1(a, field, &r) == CUI_BRIDGE_OK);
    /* Publicly reachable later-node failure: Slider's conservative preflight
     * needs more queue space than its capacity1, after TimeField already edits. */
    cui_bridge_slider_queue_options_v1 queue = {sizeof queue, 1, 1, 0};
    assert(cui_bridge_slider_queue_enable_v1(a, sizeof queue, &queue) == CUI_BRIDGE_OK);
    cui_bridge_slider_config_v1 slider_config = {.struct_size = sizeof slider_config,
                                                 .schema_version = 1,
                                                 .flags = CUI_BRIDGE_SLIDER_REQUESTS |
                                                          CUI_BRIDGE_SLIDER_REDUCED_MOTION,
                                                 .source_generation = 1,
                                                 .domain = {0, 100, 1, CUI_BRIDGE_SLIDER_LINEAR, 0},
                                                 .rem = 16,
                                                 .length = 240};
    cui_bridge_slider_value_v1 slider_value = {CUI_BRIDGE_SLIDER_SINGLE, 0, 0, 40};
    cui_bridge_node later;
    assert(cui_bridge_slider_model_create_v1(a, root, "later", sizeof slider_config, &slider_config,
                                             sizeof slider_value, &slider_value,
                                             &later) == CUI_BRIDGE_OK);
    step(a);
    step(a);
    key(a);
    assert(cui_bridge_step(a, .016f) == CUI_BRIDGE_CAPACITY);
    e = change();
    assert(cui_bridge_time_field_poll_change_v1(a, &e) == CUI_BRIDGE_OK);
    assert(e.value.hour == 15 && e.frame_status == CUI_BRIDGE_CAPACITY);
    r = result();
    assert(cui_bridge_time_field_get_result_v1(a, field, &r) == CUI_BRIDGE_CORE_ERROR);
    cui_clear_error(a->ui);
    assert(cui_bridge_time_field_get_result_v1(a, field, &r) == CUI_BRIDGE_CORE_ERROR);
    assert(cui_bridge_node_remove(a, later) == CUI_BRIDGE_OK);
    step(a);
    /* Two original-order keys can cancel without an aggregate CHANGE. */
    key(a);
    assert(cui_bridge_queue_key(a, CUI_BRIDGE_KEY_DOWN, CUI_BRIDGE_KEY_DOWN_ARROW, 0, 0) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_queue_key(a, CUI_BRIDGE_KEY_UP, CUI_BRIDGE_KEY_DOWN_ARROW, 0, 0) ==
           CUI_BRIDGE_OK);
    step(a);
    e = change();
    assert(cui_bridge_time_field_poll_change_v1(a, &e) == CUI_BRIDGE_EMPTY);
    r = result();
    assert(cui_bridge_time_field_get_result_v1(a, field, &r) == CUI_BRIDGE_OK);
    assert(!r.changed && !r.queued_change_count && r.value.hour == 15);
    /* Same-frame split input follows local selection without retargeting Core. */
    assert(cui_bridge_queue_text(a, "1") == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_text(a, "2") == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_text(a, "3") == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_text(a, "4") == CUI_BRIDGE_OK);
    step(a);
    e = change();
    assert(cui_bridge_time_field_poll_change_v1(a, &e) == CUI_BRIDGE_OK);
    assert(e.value.hour == 12 && e.value.minute == 34 && e.value.second == 56);
    /* Read-only freezes model despite genuine queued keyboard input. */
    c = config();
    c.flags = CUI_BRIDGE_TIME_FIELD_READONLY_V1;
    assert(cui_bridge_time_field_configure_v1(a, field, &c) == CUI_BRIDGE_OK);
    key(a);
    step(a);
    e = change();
    assert(cui_bridge_time_field_poll_change_v1(a, &e) == CUI_BRIDGE_EMPTY);
    assert(cui_bridge_get_time(a, field, &now) == CUI_BRIDGE_OK && now.hour == 12 &&
           now.minute == 34);
    /* A strict multiscalar16-byte vocabulary yields a real98-byte paste. */
    c.flags = 0;
    c.edit_generation++;
    for (unsigned i = 0; i < 10; i++) {
        c.tokens[i].length = 16;
        memcpy(c.tokens[i].bytes,
               "\xce\xb1"
               "aaaaaaaaaaaaa0",
               16);
        c.tokens[i].bytes[15] = (uint8_t)('0' + i);
    }
    assert(cui_bridge_time_field_configure_v1(a, field, &c) == CUI_BRIDGE_OK);
    step(a);
    step(a);
    cui_focus(a->ui, cui_time_segment_id(a->nodes[field.slot].id, 0));
    assert(cui_bridge_queue_text(a, "\xce\xb1") == CUI_BRIDGE_OK);
    step(a);
    assert(a->time_fields->nodes[field.slot]->state.pending_length == 2);
    char full[161];
    size_t length = 0;
    const unsigned digits[] = {1, 1, 2, 3, 4, 5};
    for (unsigned i = 0; i < 6; i++) {
        memcpy(full + length, c.tokens[digits[i]].bytes, 16);
        length += 16;
        if (i == 1 || i == 3)
            full[length++] = ':';
    }
    full[length] = 0;
    assert(length == 98);
    assert(cui_bridge_queue_text(a, full) == CUI_BRIDGE_OK);
    memset(full, 'x', length);
    step(a);
    e = change();
    assert(cui_bridge_time_field_poll_change_v1(a, &e) == CUI_BRIDGE_OK);
    assert(e.value.hour == 11 && e.value.minute == 23 && e.value.second == 45);
    assert(a->time_fields->nodes[field.slot]->state.pending_length == 0);
    /* Rejected complete paste clears prefix, not value, and can be cleared explicitly. */
    c.bounds_flags = CUI_BRIDGE_TIME_HAS_MAXIMUM_V1;
    c.maximum = (cui_bridge_time){11, 30, 0};
    c.edit_generation++;
    assert(cui_bridge_time_field_configure_v1(a, field, &c) == CUI_BRIDGE_OK);
    step(a);
    assert(cui_bridge_queue_text(a, "\xce\xb1") == CUI_BRIDGE_OK);
    step(a);
    assert(a->time_fields->nodes[field.slot]->state.pending_length == 2);
    length = 0;
    const unsigned rejected_digits[] = {1, 2, 0, 0, 0, 0};
    for (unsigned i = 0; i < 6; i++) {
        memcpy(full + length, c.tokens[rejected_digits[i]].bytes, 16);
        length += 16;
        if (i == 1 || i == 3)
            full[length++] = ':';
    }
    full[length] = 0;
    assert(cui_bridge_queue_text(a, full) == CUI_BRIDGE_OK);
    step(a);
    r = result();
    assert(cui_bridge_time_field_get_result_v1(a, field, &r) == CUI_BRIDGE_OK);
    assert(r.validation == CUI_BRIDGE_TIME_OUT_OF_RANGE_V1 && r.rejected_count == 1 && !r.changed);
    assert(a->time_fields->nodes[field.slot]->state.pending_length == 0);
    assert(cui_bridge_time_field_clear_validation_v1(a, field) == CUI_BRIDGE_OK);
    assert(a->time_fields->nodes[field.slot]->state.validation == CUI_TIME_TEXT_OK);
    assert(cui_bridge_get_time(a, field, &now) == CUI_BRIDGE_OK && now.hour == 11 &&
           now.minute == 23);
    assert(cui_bridge_queue_text(a, "\xce\xb1") == CUI_BRIDGE_OK);
    step(a);
    assert(a->time_fields->nodes[field.slot]->state.pending_length == 2);
    c.edit_generation++;
    assert(cui_bridge_time_field_configure_v1(a, field, &c) == CUI_BRIDGE_OK);
    step(a);
    assert(a->time_fields->nodes[field.slot]->state.pending_length == 0);
    c = config();
    assert(cui_bridge_time_field_configure_v1(a, field, &c) == CUI_BRIDGE_OK);
    step(a);
    step(a);
    cui_focus(a->ui, cui_time_segment_id(a->nodes[field.slot].id, 0));
    key(a);
    step(a);
    assert(cui_bridge_node_remove(a, field) == CUI_BRIDGE_OK);
    e = change();
    assert(cui_bridge_time_field_poll_change_v1(a, &e) == CUI_BRIDGE_EMPTY);
    assert(a->time_fields->configured == 0);
    uint64_t last = a->last_core_frame;
    a->last_core_frame = UINT64_MAX;
    time_before = a->now_seconds;
    assert(cui_bridge_step(a, .016f) == CUI_BRIDGE_CAPACITY && a->now_seconds == time_before);
    a->last_core_frame = last;
    cui_bridge_destroy(a);
    puts("localized TimeField bridge: owned config, nested UI, copied paste, thread, preflight and "
         "failed-frame observations");
    return 0;
}
