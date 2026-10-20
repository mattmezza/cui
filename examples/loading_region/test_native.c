/* SPDX-License-Identifier: MIT */
#define _POSIX_C_SOURCE 200809L
#include "cui/cui_accessibility.h"
#include "scene.h"
#include <assert.h>
#include <atspi/atspi.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
static void sleep_ms(unsigned ms) {
    struct timespec t = {ms / 1000, (long)(ms % 1000) * 1000000};
    nanosleep(&t, NULL);
}
static int provider(int input) {
    assert(fcntl(input, F_SETFL, O_NONBLOCK) != -1);
    cui_context *ui = cui_create((cui_config){.max_elements = 128});
    cui_accessibility *bridge = cui_accessibility_create((cui_accessibility_config){0});
    assert(ui && bridge);
    cui_theme theme = cui_theme_light();
    loading_scene scene = {.alert_visible = true, .alert_name = "Records unavailable"};
    unsigned ticks = 0;
    for (;;) {
        char command;
        ssize_t received = read(input, &command, 1);
        if (received == 0)
            break;
        if (received == 1) {
            if (command == 'q')
                break;
            if (command == 'f')
                scene.phase = LOADING_REGION_FAILED;
            if (command == 'n')
                scene.alert_name = "Retry failed";
            if (command == 'h')
                scene.alert_visible = false;
            if (command == 's')
                scene.alert_visible = true;
            if (command == 'r')
                scene.phase = LOADING_REGION_READY;
        }
        assert(cui_begin_frame(ui, 400, 400, 0));
        loading_scene_frame(ui, &theme, &scene, ticks++ * .01);
        cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK);
        assert(cui_accessibility_update(bridge, ui,
                                        (cui_accessibility_window){.outer = {0, 0, 400, 400},
                                                                   .inner = {0, 0, 400, 400},
                                                                   .title = {"Loading fixture", 15},
                                                                   .focused = true,
                                                                   .has_screen_position = true}));
        sleep_ms(10);
    }
    cui_accessibility_destroy(bridge);
    cui_destroy(ui);
    close(input);
    return 0;
}
static AtspiAccessible *find(AtspiAccessible *root, const char *name, unsigned depth) {
    if (depth > 12)
        return NULL;
    GError *error = NULL;
    char *label = atspi_accessible_get_name(root, &error);
    g_clear_error(&error);
    bool equal = label && !strcmp(label, name);
    g_free(label);
    if (equal)
        return g_object_ref(root);
    int count = atspi_accessible_get_child_count(root, &error);
    g_clear_error(&error);
    for (int i = 0; i < count; i++) {
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
static unsigned unavailable, failed, loaded;
static void event(AtspiEvent *e, void *user) {
    (void)user;
    assert(!strcmp(e->type, "object:announcement"));
    if (G_VALUE_HOLDS_STRING(&e->any_data)) {
        const char *text = g_value_get_string(&e->any_data);
        if (!strcmp(text, "Records unavailable")) {
            assert(e->detail1 == 2);
            unavailable++;
        } else if (!strcmp(text, "Retry failed")) {
            assert(e->detail1 == 2);
            failed++;
        } else if (!strcmp(text, "Records loaded")) {
            assert(e->detail1 == 1);
            loaded++;
        }
    }
    g_boxed_free(ATSPI_TYPE_EVENT, e);
}
static void pump(unsigned ms) {
    for (unsigned i = 0; i < ms / 10; i++) {
        while (g_main_context_iteration(NULL, FALSE)) {
        }
        sleep_ms(10);
    }
}
static void wait_count(unsigned *count, unsigned expected) {
    for (unsigned i = 0; i < 300 && *count < expected; i++)
        pump(10);
    assert(*count == expected);
}
static void command(int fd, char c) {
    assert(write(fd, &c, 1) == 1);
}
int main(int argc, char **argv) {
    if (argc == 3 && !strcmp(argv[1], "--provider"))
        return provider(atoi(argv[2]));
    assert(getenv("CUI_A11Y_PRIVATE_SESSION"));
    int commands[2];
    assert(pipe(commands) == 0);
    pid_t child = fork();
    assert(child >= 0);
    if (!child) {
        close(commands[1]);
        char input[32];
        snprintf(input, sizeof(input), "%d", commands[0]);
        execl(argv[0], argv[0], "--provider", input, (char *)NULL);
        _exit(127);
    }
    close(commands[0]);
    assert(atspi_init() == 0);
    atspi_set_timeout(1000, 1000);
    GError *error = NULL;
    AtspiEventListener *listener = atspi_event_listener_new(event, NULL, NULL);
    assert(atspi_event_listener_register(listener, "object:announcement", &error) && !error);
    AtspiAccessible *desktop = atspi_get_desktop(0), *region = NULL;
    assert(desktop);
    for (unsigned i = 0; i < 300 && !region; i++) {
        pump(10);
        atspi_accessible_clear_cache(desktop);
        region = find(desktop, "Records", 0);
    }
    assert(region);
    AtspiStateSet *states = atspi_accessible_get_state_set(region);
    assert(atspi_state_set_contains(states, ATSPI_STATE_BUSY));
    g_object_unref(states);
    command(commands[1], 'f');
    wait_count(&unavailable, 1);
    atspi_accessible_clear_cache(desktop);
    AtspiAccessible *alert = find(desktop, "Records unavailable", 0);
    assert(alert && atspi_accessible_get_role(alert, &error) == ATSPI_ROLE_NOTIFICATION && !error);
    atspi_accessible_clear_cache(region);
    states = atspi_accessible_get_state_set(region);
    assert(!atspi_state_set_contains(states, ATSPI_STATE_BUSY));
    g_object_unref(states);
    pump(200);
    assert(unavailable == 1); /* unchanged UI polls never repeat the announcement */
    command(commands[1], 'n');
    wait_count(&failed, 1);
    command(commands[1], 'h');
    pump(200);
    assert(failed == 1);
    atspi_accessible_clear_cache(desktop);
    AtspiAccessible *hidden = find(desktop, "Retry failed", 0);
    assert(!hidden);
    command(commands[1], 's');
    wait_count(&failed, 2);
    command(commands[1], 'r');
    wait_count(&loaded, 1);
    pump(200);
    assert(unavailable == 1 && failed == 2 && loaded == 1);
    command(commands[1], 'q');
    close(commands[1]);
    int status;
    assert(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0);
    assert(atspi_event_listener_deregister(listener, "object:announcement", &error) && !error);
    g_object_unref(alert);
    g_object_unref(region);
    g_object_unref(desktop);
    g_object_unref(listener);
    atspi_exit();
    puts("Real private AT-SPI Alert assertive / status polite / busy transitions PASS");
    return 0;
}
