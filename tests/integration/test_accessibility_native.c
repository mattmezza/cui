/* SPDX-License-Identifier: MIT */
#define _POSIX_C_SOURCE 200809L
#include "cui/cui_accessibility.h"
#include "cui/cui_sdl3.h"
#include "cui_accessibility_test.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <atspi/atspi.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <threads.h>
#include <time.h>
#include <unistd.h>
typedef struct {
    unsigned clicks;
    bool checked;
    cui_id focus;
    size_t pending;
} report;
typedef struct {
    thrd_t ui_thread;
    atomic_uint calls;
} wake_probe;
static void wake(void *opaque) {
    wake_probe *p = opaque;
    assert(!thrd_equal(p->ui_thread, thrd_current()));
    atomic_fetch_add(&p->calls, 1);
}
static void sleep_ms(unsigned ms) {
    struct timespec t = {ms / 1000, (long)(ms % 1000) * 1000000};
    nanosleep(&t, NULL);
}
static void node(cui_context *ui, cui_id id, cui_role role, const char *name, cui_rect bounds,
                 bool disabled, bool *checked, unsigned *clicks) {
    cui_begin_absolute_box(ui, id, (cui_box){0}, bounds);
    cui_interaction s =
        cui_interact(ui, id, role, cui_string(name), disabled, checked ? *checked : false, 0);
    if (s.activated) {
        if (checked)
            *checked = !*checked;
        if (clicks)
            ++*clicks;
    }
    if (checked)
        cui_update_semantic(ui, id, *checked, 0);
    cui_end_box(ui);
}
static int provider(int commands, int output) {
    assert(SDL_Init(SDL_INIT_VIDEO));
    cui_sdl3 *backend = cui_sdl3_create("CUI native accessibility fixture", 320, 200, NULL);
    assert(backend);
    int window_count = 0;
    SDL_Window **windows = SDL_GetWindows(&window_count);
    assert(windows && window_count == 1);
    SDL_Window *w = windows[0];
    SDL_free(windows);
    assert(SDL_SetWindowPosition(w, 60, 80));
    SDL_RaiseWindow(w);
    cui_context *ui = cui_create((cui_config){.width = 320, .height = 200});
    assert(ui);
    wake_probe probe = {.ui_thread = thrd_current()};
    atomic_init(&probe.calls, 0);
    cui_accessibility *bridge =
        cui_accessibility_create((cui_accessibility_config){.wake = wake, .user = &probe});
    assert(bridge);
    assert(fcntl(commands, F_SETFL, O_NONBLOCK) != -1);
    report state = {0};
    bool running = true, disabled = false, present = true, hold = false;
    for (unsigned frame = 0; running && frame < 3000; ++frame) {
        char cmd;
        while (read(commands, &cmd, 1) == 1) {
            if (cmd == 'h')
                hold = true;
            if (cmd == 'q')
                running = false;
            if (cmd == 'd')
                disabled = true;
            if (cmd == 'r')
                present = false;
            if (cmd == 'n') {
                present = true;
                disabled = false;
            }
        }
        if (!running)
            break;
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT)
                running = false;
        }
        if (!hold)
            cui_accessibility_drain(bridge, ui);
        assert(cui_begin_frame(ui, 320, 200, .01f));
        cui_begin_box(ui, 1, (cui_box){.width = 320, .height = 200});
        char frame_label[32] = "Native button";
        if (present)
            node(ui, 42, CUI_ROLE_BUTTON, frame_label, (cui_rect){20, 30, 100, 30}, disabled, NULL,
                 &state.clicks);
        node(ui, 43, CUI_ROLE_CHECKBOX, "Native checkbox", (cui_rect){20, 75, 130, 30}, false,
             &state.checked, NULL);
        node(ui, 44, CUI_ROLE_LABEL, "Native label", (cui_rect){170, 75, 100, 30}, false, NULL,
             NULL);
        cui_set_semantic_details(
            ui, 43,
            (cui_semantic_details){
                .labelled_by = 44, .controls = 44, .description = {"Checkbox description", 20}});
        cui_end_box(ui);
        cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK);
        state.focus = cui_focused(ui);
        cui_accessibility_window window;
        assert(cui_sdl3_accessibility_window(backend, &window));
        assert(window.has_screen_position && window.inner.w == 320 && window.inner.h == 200);
        /* Xvfb has no WM; SDL's actual input focus remains the host source. */
        assert(cui_accessibility_update(bridge, ui, window));
        state.pending = cui_accessibility_get_stats(bridge).pending;
        memset(frame_label, 'x', sizeof(frame_label)); /* native snapshot owns its label */
        if (frame % 5 == 0)
            assert(write(output, &state, sizeof(state)) == (ssize_t)sizeof(state));
        sleep_ms(10);
    }
    assert(atomic_load(&probe.calls) >= 5 && state.pending == 1);
    cui_accessibility_destroy(bridge);
    cui_destroy(ui);
    cui_sdl3_destroy(backend);
    SDL_Quit();
    /* Give asynchronous RemoveAdapter a chance to unregister before process exit. */
    for (unsigned i = 0; i < 200 && cui_accessibility_test_live(); ++i)
        sleep_ms(5);
    assert(cui_accessibility_test_live() == 0 && cui_accessibility_test_released() == 1);
    return 0;
}
static AtspiAccessible *find(AtspiAccessible *root, const char *name, unsigned depth) {
    if (depth > 5)
        return NULL;
    atspi_accessible_clear_cache_single(root);
    GError *error = NULL;
    char *label = atspi_accessible_get_name(root, &error);
    if (error) {
        g_clear_error(&error);
        g_free(label);
        return NULL;
    }
    bool equal = label && strcmp(label, name) == 0;
    g_free(label);
    if (equal)
        return g_object_ref(root);
    int n = atspi_accessible_get_child_count(root, &error);
    g_clear_error(&error);
    for (int i = 0; i < n; ++i) {
        AtspiAccessible *child = atspi_accessible_get_child_at_index(root, i, &error);
        g_clear_error(&error);
        if (!child)
            continue;
        AtspiAccessible *found = find(child, name, depth + 1);
        g_object_unref(child);
        if (found)
            return found;
    }
    return NULL;
}
static report latest(int input) {
    report state = {0}, next;
    struct pollfd fd = {input, POLLIN, 0};
    assert(poll(&fd, 1, 2000) == 1);
    assert(read(input, &state, sizeof(state)) == (ssize_t)sizeof(state));
    while (read(input, &next, sizeof(next)) == (ssize_t)sizeof(next))
        state = next;
    return state;
}
static void click(AtspiAccessible *node) {
    GError *error = NULL;
    AtspiAction *action = atspi_accessible_get_action_iface(node);
    assert(action);
    int n = atspi_action_get_n_actions(action, &error);
    assert(!error && n > 0);
    bool clicked = false;
    for (int i = 0; i < n; ++i) {
        char *name = atspi_action_get_action_name(action, i, &error);
        assert(!error);
        if (strcmp(name, "click") == 0) {
            assert(atspi_action_do_action(action, i, &error));
            assert(!error);
            clicked = true;
        }
        g_free(name);
    }
    assert(clicked);
    g_object_unref(action);
}
int main(int argc, char **argv) {
    if (argc == 4 && strcmp(argv[1], "--provider") == 0)
        return provider(atoi(argv[2]), atoi(argv[3]));
    /* Private dbus-run-session only; do not change the desktop user's a11y setting. */
    assert(getenv("CUI_A11Y_PRIVATE_SESSION"));
    int command[2], output[2];
    assert(pipe(command) == 0 && pipe(output) == 0);
    pid_t child = fork();
    assert(child >= 0);
    if (child == 0) {
        close(command[1]);
        close(output[0]);
        char a[32], b[32];
        snprintf(a, sizeof(a), "%d", command[0]);
        snprintf(b, sizeof(b), "%d", output[1]);
        execl(argv[0], argv[0], "--provider", a, b, (char *)NULL);
        _exit(127);
    }
    close(command[0]);
    close(output[1]);
    assert(fcntl(output[0], F_SETFL, O_NONBLOCK) != -1);
    assert(atspi_init() == 0);
    atspi_set_timeout(2000, 2000);
    AtspiAccessible *desktop = atspi_get_desktop(0), *button = NULL, *checkbox = NULL;
    assert(desktop);
    for (unsigned i = 0; i < 200 && !button; ++i) {
        while (g_main_context_iteration(NULL, FALSE)) {
        }
        atspi_accessible_clear_cache(desktop);
        button = find(desktop, "Native button", 0);
        if (!button)
            sleep_ms(50);
    }
    assert(button);
    checkbox = find(desktop, "Native checkbox", 0);
    assert(checkbox);
    GError *error = NULL;
    assert(atspi_accessible_get_role(button, &error) == ATSPI_ROLE_PUSH_BUTTON && !error);
    assert(atspi_accessible_get_role(checkbox, &error) == ATSPI_ROLE_CHECK_BOX && !error);
    AtspiComponent *component = atspi_accessible_get_component_iface(button);
    assert(component);
    AtspiRect *rect = atspi_component_get_extents(component, ATSPI_COORD_TYPE_SCREEN, &error);
    assert(rect && !error && rect->x == 80 && rect->y == 110 && rect->width == 100 &&
           rect->height == 30);
    g_free(rect);
    assert(atspi_component_grab_focus(component, &error) && !error);
    sleep_ms(150);
    report state = latest(output[0]);
    assert(state.focus == 42 && state.clicks == 0);
    click(button);
    sleep_ms(150);
    state = latest(output[0]);
    assert(state.clicks == 1);
    click(checkbox);
    sleep_ms(150);
    state = latest(output[0]);
    assert(state.checked && state.clicks == 1);
    atspi_accessible_clear_cache_single(checkbox);
    AtspiStateSet *states = atspi_accessible_get_state_set(checkbox);
    assert(atspi_state_set_contains(states, ATSPI_STATE_CHECKED));
    g_object_unref(states);
    GArray *relations = atspi_accessible_get_relation_set(checkbox, &error);
    assert(relations && !error && relations->len > 0);
    bool controller = false;
    for (unsigned i = 0; i < relations->len; ++i) {
        AtspiRelation *relation = g_array_index(relations, AtspiRelation *, i);
        if (atspi_relation_get_relation_type(relation) == ATSPI_RELATION_CONTROLLER_FOR) {
            assert(atspi_relation_get_n_targets(relation) == 1);
            AtspiAccessible *target = atspi_relation_get_target(relation, 0);
            char *name = atspi_accessible_get_name(target, &error);
            assert(!error && strcmp(name, "Native label") == 0);
            g_free(name);
            g_object_unref(target);
            controller = true;
        }
    }
    assert(controller);
    g_array_unref(relations);
    assert(write(command[1], "d", 1) == 1);
    sleep_ms(150);
    AtspiAction *actions = atspi_accessible_get_action_iface(button);
    if (actions) {
        assert(atspi_action_get_n_actions(actions, &error) == 0 && !error);
        g_object_unref(actions);
    }
    assert(write(command[1], "r", 1) == 1);
    sleep_ms(150);
    AtspiAccessible *removed = find(desktop, "Native button", 0);
    assert(!removed);
    assert(write(command[1], "n", 1) == 1);
    sleep_ms(150);
    AtspiAccessible *replacement = find(desktop, "Native button", 0);
    assert(replacement);
    click(replacement);
    sleep_ms(150);
    state = latest(output[0]);
    assert(state.clicks == 2);
    assert(write(command[1], "h", 1) == 1);
    sleep_ms(100);
    click(replacement);
    sleep_ms(150);
    state = latest(output[0]);
    assert(state.pending == 1 && state.clicks == 2); /* real native action pending at teardown */
    assert(write(command[1], "q", 1) == 1);
    int status = 0;
    assert(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0);
    g_object_unref(replacement);
    g_object_unref(component);
    g_object_unref(checkbox);
    g_object_unref(button);
    g_object_unref(desktop);
    close(command[1]);
    close(output[0]);
    atspi_exit();
    puts("native AT-SPI roles/bounds/focus/actions/state/removal passed");
    return 0;
}
