/* SPDX-License-Identifier: MIT */
#include "cui/cui_accessibility.h"
#include "cui_accessibility_test.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>
static unsigned activated;
static cui_context *make(void) {
    cui_context *ui = cui_create((cui_config){
        .width = 300, .height = 200, .max_elements = 64, .max_commands = 128, .frame_bytes = 4096});
    assert(ui);
    return ui;
}
static void scene(cui_context *ui, cui_role role, bool present, bool disabled, const char *name) {
    assert(cui_begin_frame(ui, 300, 200, .016f));
    if (present) {
        cui_begin_box(ui, 42, (cui_box){.width = 100, .height = 30});
        cui_interaction s = cui_interact(ui, 42, role, cui_string(name), disabled, false, 0);
        if (s.activated)
            ++activated;
        cui_end_box(ui);
    }
    cui_end_frame(ui);
}
static cui_accessibility_window window(void) {
    return (cui_accessibility_window){.outer = {10, 20, 300, 200},
                                      .inner = {10, 20, 300, 200},
                                      .title = {"Fixture", 7},
                                      .focused = true,
                                      .has_screen_position = true};
}
typedef struct {
    cui_accessibility *a;
    uint64_t token;
    unsigned successes;
} thread_request;
static int request_thread(void *opaque) {
    thread_request *p = opaque;
    for (unsigned i = 0; i < 64; ++i)
        if (cui_accessibility_request(p->a, p->token, CUI_ACCESSIBILITY_ACTIVATE))
            ++p->successes;
    return 0;
}
int main(void) {
    assert(!cui_accessibility_create((cui_accessibility_config){.max_nodes = SIZE_MAX}));
    cui_context *ui = make();
    cui_accessibility *a = cui_accessibility_create(
        (cui_accessibility_config){.max_nodes = 4, .string_bytes = 64, .max_actions = 2});
    assert(a);
    char label[16] = "Copied button";
    scene(ui, CUI_ROLE_BUTTON, true, false, label);
    assert(cui_accessibility_update(a, ui, window()));
    assert(cui_accessibility_update(a, ui, window()));
    assert(cui_accessibility_get_stats(a).snapshots ==
           1); /* no native work on unchanged idle snapshot */
    uint64_t original = cui_accessibility_token(a, 42);
    assert(original > 1);
    memset(label, 'x', sizeof(label)); /* update retained strings must not borrow this */
    assert(!cui_accessibility_request(a, original, (cui_accessibility_action)99));
    assert(cui_accessibility_request(a, original, CUI_ACCESSIBILITY_FOCUS));
    assert(cui_accessibility_request(a, original, CUI_ACCESSIBILITY_ACTIVATE));
    assert(!cui_accessibility_request(a, original, CUI_ACCESSIBILITY_ACTIVATE));
    assert(cui_accessibility_get_stats(a).pending == 2);
    assert(cui_accessibility_drain(a, ui) == 2);
    scene(ui, CUI_ROLE_BUTTON, true, false, "button");
    assert(activated == 1 && cui_focused(ui) == 42);
    assert(cui_accessibility_update(a, ui, window()));
    assert(cui_accessibility_token(a, 42) == original);
    cui_accessibility_window bad = window();
    bad.title = (cui_str){"\xff", 1};
    assert(!cui_accessibility_update(a, ui, bad));
    assert(cui_accessibility_get_stats(a).status == CUI_A11Y_INVALID &&
           cui_accessibility_token(a, 42) == original);
    bad = window();
    bad.inner.w = NAN;
    assert(!cui_accessibility_update(a, ui, bad) && cui_accessibility_token(a, 42) == original);
    for (unsigned invalid = 0; invalid < 2; ++invalid) {
        assert(cui_begin_frame(ui, 300, 200, .016f));
        cui_begin_box(ui, 42, (cui_box){.width = 100, .height = 30});
        cui_interact(ui, 42, CUI_ROLE_BUTTON, cui_string("button"), false, false, 0);
        cui_set_semantic_details(ui, 42,
                                 invalid
                                     ? (cui_semantic_details){.parent = 42}
                                     : (cui_semantic_details){.orientation = (cui_orientation)99});
        cui_end_box(ui);
        cui_end_frame(ui);
        assert(!cui_accessibility_update(a, ui, window()));
        assert(cui_accessibility_get_stats(a).status == CUI_A11Y_INVALID &&
               cui_accessibility_token(a, 42) == original);
    }
    scene(ui, CUI_ROLE_BUTTON, true, false, "button");
    assert(cui_accessibility_update(a, ui, window()));
    char huge[100];
    memset(huge, 'a', 99);
    huge[99] = 0;
    bad = window();
    bad.title = cui_string(huge);
    assert(!cui_accessibility_update(a, ui, bad));
    assert(cui_accessibility_token(a, 42) == original);
    assert(cui_begin_frame(ui, 300, 200, .016f));
    for (cui_id id = 50; id < 55; ++id) {
        cui_begin_box(ui, id, (cui_box){.width = 20, .height = 20});
        cui_interact(ui, id, CUI_ROLE_BUTTON, cui_string("many"), false, false, 0);
        cui_end_box(ui);
    }
    cui_end_frame(ui);
    assert(!cui_accessibility_update(a, ui, window()));
    assert(cui_accessibility_get_stats(a).status == CUI_A11Y_CAPACITY &&
           cui_accessibility_token(a, 42) == original);
    cui_accessibility *larger = cui_accessibility_create(
        (cui_accessibility_config){.max_nodes = 8, .string_bytes = 512, .max_actions = 4});
    assert(larger);
    assert(cui_accessibility_update(larger, ui, window()));
    assert(cui_accessibility_token(larger, 54));
    cui_accessibility_destroy(larger);
    scene(ui, CUI_ROLE_BUTTON, true, false, "button");
    assert(cui_accessibility_update(a, ui, window()));
    assert(cui_accessibility_token(a, 42) ==
           original); /* failed snapshot did not consume identity */
    assert(cui_accessibility_request(a, original, CUI_ACCESSIBILITY_ACTIVATE));
    scene(ui, CUI_ROLE_BUTTON, false, false, "");
    assert(cui_accessibility_update(a, ui, window()));
    scene(ui, CUI_ROLE_BUTTON, true, false, "replacement");
    assert(cui_accessibility_update(a, ui, window()));
    uint64_t replacement = cui_accessibility_token(a, 42);
    assert(replacement != original);
    assert(!cui_accessibility_request(a, original, CUI_ACCESSIBILITY_ACTIVATE));
    assert(cui_accessibility_drain(a, ui) == 0);
    assert(activated == 1);
    scene(ui, CUI_ROLE_CHECKBOX, true, false, "checkbox");
    assert(cui_accessibility_update(a, ui, window()));
    assert(cui_accessibility_token(a, 42) != replacement);
    assert(
        cui_accessibility_request(a, cui_accessibility_token(a, 42), CUI_ACCESSIBILITY_ACTIVATE));
    scene(ui, CUI_ROLE_CHECKBOX, true, true, "disabled");
    assert(cui_accessibility_drain(a, ui) ==
           0); /* current Core guard even before snapshot refresh */
    assert(cui_accessibility_update(a, ui, window()));
    assert(
        !cui_accessibility_request(a, cui_accessibility_token(a, 42), CUI_ACCESSIBILITY_ACTIVATE));
    scene(ui, CUI_ROLE_BUTTON, true, false, "button");
    assert(cui_accessibility_update(a, ui, window()));
    thread_request requests = {a, cui_accessibility_token(a, 42), 0};
    thrd_t thread;
    assert(thrd_create(&thread, request_thread, &requests) == thrd_success);
    thrd_join(thread, NULL);
    assert(requests.successes == 2 && cui_accessibility_get_stats(a).pending == 2);
    assert(cui_accessibility_drain(a, ui) == 2);
    scene(ui, CUI_ROLE_BUTTON, true, false, "button");
    assert(activated == 2); /* bool interaction coalesces */
    /* Core backpressure leaves requests queued and does not replay accepted work. */
    for (unsigned i = 0; i < 256; ++i)
        assert(cui_queue_accessibility_action(ui, 42, CUI_ACCESSIBILITY_ACTIVATE));
    assert(
        cui_accessibility_request(a, cui_accessibility_token(a, 42), CUI_ACCESSIBILITY_ACTIVATE));
    assert(cui_accessibility_drain(a, ui) == 0 && cui_accessibility_get_stats(a).pending == 1);
    scene(ui, CUI_ROLE_BUTTON, true, false, "button");
    assert(activated == 3);
    assert(cui_last_error(ui) == CUI_ERROR_CAPACITY);
    cui_clear_error(ui);
    assert(cui_accessibility_drain(a, ui) == 1);
    scene(ui, CUI_ROLE_BUTTON, true, false, "button");
    assert(activated == 4);
    assert(cui_accessibility_drain(a, ui) == 0);
    /* Published modal action mask suppresses behind-modal targets. */
    assert(cui_begin_frame(ui, 300, 200, .016f));
    cui_begin_box(ui, 42, (cui_box){.width = 100, .height = 30});
    cui_interact(ui, 42, CUI_ROLE_BUTTON, cui_string("behind modal"), false, false, 0);
    cui_end_box(ui);
    assert(cui_begin_focus_scope(ui, 500, true));
    cui_begin_box(ui, 43, (cui_box){.width = 100, .height = 30});
    cui_interact(ui, 43, CUI_ROLE_BUTTON, cui_string("modal button"), false, false, 0);
    cui_end_box(ui);
    cui_end_focus_scope(ui);
    cui_end_frame(ui);
    assert(cui_accessibility_update(a, ui, window()));
    assert(
        !cui_accessibility_request(a, cui_accessibility_token(a, 42), CUI_ACCESSIBILITY_ACTIVATE));
    assert(cui_accessibility_request(a, cui_accessibility_token(a, 43), CUI_ACCESSIBILITY_FOCUS));
    assert(cui_accessibility_drain(a, ui) == 1);
    cui_accessibility_destroy(a);
    cui_destroy(ui);
    /* Repeated asynchronous native removal: no leaking tombstone slots. */
    for (unsigned i = 0; i < 100; ++i) {
        a = cui_accessibility_create(
            (cui_accessibility_config){.max_nodes = 1, .string_bytes = 32, .max_actions = 1});
        assert(a);
        cui_accessibility_destroy(a);
    }
    for (unsigned i = 0; i < 1000 && cui_accessibility_test_live(); ++i) {
        struct timespec t = {0, 5000000};
        thrd_sleep(&t, NULL);
    }
    assert(cui_accessibility_test_live() == 0 && cui_accessibility_test_released() == 102);
    puts("accessibility snapshot/generation/mailbox tests passed");
    return 0;
}
