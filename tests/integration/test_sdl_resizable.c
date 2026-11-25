/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include "cui/cui_resizable.h"
#include "cui/cui_sdl3.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { GROUP_ID = 100, FIRST_PANE = 101, SECOND_PANE = 102, EXTENT = 320 };
static const cui_color pane_colors[] = {{35, 85, 150, 255}, {145, 65, 45, 255}};

typedef struct {
    cui_sdl3 *host;
    cui_context *ui;
    SDL_WindowID window;
    cui_resizable_axis axis;
    cui_resizable_slot slots[2];
    cui_resizable_state state;
    unsigned completions;
} split_scene;

static void completed(void *user, const cui_resizable_state *state) {
    split_scene *scene = user;
    assert(state == &scene->state);
    ++scene->completions;
}

static void check_near(float actual, float expected, const char *expression) {
    if (fabsf(actual - expected) >= .01f)
        fprintf(stderr, "%s expected %g, got %g\n", expression, (double)expected, (double)actual);
    assert(fabsf(actual - expected) < .01f);
}
#define near(actual, expected) check_near(actual, expected, #actual)

static float position(cui_resizable_axis axis, cui_rect rect) {
    return axis == CUI_RESIZABLE_HORIZONTAL ? rect.x : rect.y;
}

static float size(cui_resizable_axis axis, cui_rect rect) {
    return axis == CUI_RESIZABLE_HORIZONTAL ? rect.w : rect.h;
}

static void frame(split_scene *scene, float first_size, unsigned completions) {
    cui_resizable_panel panels[] = {cui_resizable_panel_default(FIRST_PANE),
                                    cui_resizable_panel_default(SECOND_PANE)};
    for (size_t i = 0; i < 2; ++i) {
        panels[i].custom_range = true;
        panels[i].minimum = 80;
        panels[i].maximum = 240;
        panels[i].box.background = pane_colors[i];
    }
    cui_theme theme = cui_theme_light();
    float width, height;
    assert(cui_sdl3_poll(scene->host, scene->ui));
    cui_sdl3_size(scene->host, &width, &height);
    assert(width == EXTENT && height == EXTENT);
    assert(cui_begin_frame(scene->ui, width, height, .016f));
    assert(cui_resizable(scene->ui, GROUP_ID, &theme, &scene->state, panels, 2,
                         (cui_resizable_options){.axis = scene->axis,
                                                 .extent = EXTENT,
                                                 .cross_size = EXTENT,
                                                 .reduced_motion = true}) == CUI_RESIZABLE_OK);
    cui_draw_list draw = cui_end_frame(scene->ui);
    assert(cui_last_error(scene->ui) == CUI_OK);
    assert(scene->completions == completions);
    near(scene->slots[0].size, first_size);
    near(scene->slots[1].size, EXTENT - first_size);
    cui_rect first = cui_bounds(scene->ui, FIRST_PANE);
    cui_rect second = cui_bounds(scene->ui, SECOND_PANE);
    near(position(scene->axis, first), 0);
    near(position(scene->axis, second), first_size);
    near(size(scene->axis, first), first_size);
    near(size(scene->axis, second), EXTENT - first_size);

    /* Assert both pane fills move in the same list we present. A moved
     * divider over stale content would otherwise pass model-only assertions. */
    bool painted[2] = {false, false};
    for (size_t i = 0; i < draw.count; ++i) {
        const cui_draw_command *command = &draw.commands[i];
        if (command->type != CUI_DRAW_RECT)
            continue;
        for (size_t pane = 0; pane < 2; ++pane) {
            cui_color color = pane_colors[pane];
            if (command->color.r != color.r || command->color.g != color.g ||
                command->color.b != color.b || command->color.a != color.a)
                continue;
            cui_rect bounds = pane ? second : first;
            near(position(scene->axis, command->bounds), position(scene->axis, bounds));
            near(size(scene->axis, command->bounds), size(scene->axis, bounds));
            painted[pane] = true;
        }
    }
    assert(painted[0] && painted[1]);
    assert(cui_sdl3_render(scene->host, &draw, theme.background));
}

static SDL_FPoint point(split_scene *scene, float along) {
    return scene->axis == CUI_RESIZABLE_HORIZONTAL ? (SDL_FPoint){along, 60}
                                                   : (SDL_FPoint){60, along};
}

static void mouse_at(split_scene *scene, SDL_EventType type, SDL_FPoint at) {
    SDL_Event event = {.type = type};
    if (type == SDL_EVENT_MOUSE_MOTION) {
        event.motion.windowID = scene->window;
        event.motion.x = at.x;
        event.motion.y = at.y;
        event.motion.state = SDL_BUTTON_LMASK;
    } else {
        event.button.windowID = scene->window;
        event.button.button = SDL_BUTTON_LEFT;
        event.button.x = at.x;
        event.button.y = at.y;
        event.button.down = type == SDL_EVENT_MOUSE_BUTTON_DOWN;
        event.button.clicks = 1;
    }
    assert(SDL_PushEvent(&event));
}

static void mouse(split_scene *scene, SDL_EventType type, float along) {
    mouse_at(scene, type, point(scene, along));
}

static void finger(split_scene *scene, SDL_EventType type, float along) {
    SDL_FPoint at = point(scene, along);
    SDL_Event event = {.type = type};
    event.tfinger.windowID = scene->window;
    event.tfinger.fingerID = 1;
    event.tfinger.touchID = 1;
    event.tfinger.x = at.x / EXTENT;
    event.tfinger.y = at.y / EXTENT;
    assert(SDL_PushEvent(&event));
}

static void grow_by_key(split_scene *scene) {
    SDL_Event event = {.type = SDL_EVENT_KEY_DOWN};
    event.key.windowID = scene->window;
    event.key.key = scene->axis == CUI_RESIZABLE_HORIZONTAL ? SDLK_RIGHT : SDLK_DOWN;
    event.key.down = true;
    assert(SDL_PushEvent(&event));
    event.type = SDL_EVENT_KEY_UP;
    event.key.down = false;
    assert(SDL_PushEvent(&event));
}

static void exercise(split_scene *scene) {
    assert(cui_resizable_state_init(&scene->state, scene->slots, 2));
    scene->state.on_resize = completed;
    scene->state.resize_user = scene;
    scene->completions = 0;
    cui_id handle = cui_resizable_handle_id(GROUP_ID, FIRST_PANE);
    frame(scene, 160, 0);
    cui_rect band = cui_bounds(scene->ui, handle);
    near(position(scene->axis, band) + 4, 160);
    near(size(scene->axis, band), 9);

    mouse(scene, SDL_EVENT_MOUSE_BUTTON_DOWN, 160);
    frame(scene, 160, 0);
    assert(scene->state.active_handle == handle && !scene->state.dragging);
    mouse(scene, SDL_EVENT_MOUSE_MOTION, 204);
    frame(scene, 204, 0);
    assert(scene->state.dragging);
    mouse(scene, SDL_EVENT_MOUSE_MOTION, 282);
    frame(scene, 240, 0);
    mouse(scene, SDL_EVENT_MOUSE_BUTTON_UP, 282);
    frame(scene, 240, 1);
    assert(!scene->state.active_handle && !scene->state.dragging);

    mouse(scene, SDL_EVENT_MOUSE_BUTTON_DOWN, 240);
    frame(scene, 240, 1);
    mouse(scene, SDL_EVENT_MOUSE_MOTION, 30);
    frame(scene, 80, 1);
    mouse(scene, SDL_EVENT_MOUSE_MOTION, 92);
    frame(scene, 92, 1);
    mouse(scene, SDL_EVENT_MOUSE_BUTTON_UP, 92);
    frame(scene, 92, 2);
    assert(cui_focused(scene->ui) == handle);

    grow_by_key(scene);
    frame(scene, 102, 3);

    finger(scene, SDL_EVENT_FINGER_DOWN, 102);
    frame(scene, 102, 3);
    finger(scene, SDL_EVENT_FINGER_MOTION, 144);
    frame(scene, 144, 3);
    finger(scene, SDL_EVENT_FINGER_UP, 144);
    frame(scene, 144, 4);
    assert(!scene->state.active_handle && !scene->state.dragging);
}

enum {
    WORKSPACE_ROOT = 200,
    WORKSPACE_HEADER,
    WORKSPACE_FOOTER,
    WORKSPACE_DOCUMENT,
    WORKSPACE_FILE
};
typedef struct {
    split_scene split;
    cui_theme theme;
    cui_edit *document;
    cui_input_state input;
} workspace;

static void explorer_pane(cui_context *ui, void *user) {
    workspace *app = user;
    (void)cui_button(ui, WORKSPACE_FILE, &app->theme, cui_string("Notes"), false);
}

static void document_pane(cui_context *ui, void *user) {
    workspace *app = user;
    assert(cui_input_ex(ui, WORKSPACE_DOCUMENT, &app->theme, cui_string("Notes"), app->document,
                        &app->input, (cui_input_options){.width = -1})
               .error == CUI_EDIT_OK);
}

static void workspace_frame(workspace *app, float width, float height, float left,
                            unsigned completions) {
    split_scene *scene = &app->split;
    cui_context *ui = scene->ui;
    float host_width, host_height;
    assert(cui_sdl3_poll(scene->host, ui));
    cui_sdl3_size(scene->host, &host_width, &host_height);
    near(host_width, width);
    near(host_height, height);
    assert(cui_begin_frame(ui, host_width, host_height, .016f));
    cui_begin_box(ui, WORKSPACE_ROOT, (cui_box){.width = -1, .height = -1, .clip = true});
    cui_begin_box(ui, WORKSPACE_HEADER, (cui_box){.width = -1, .height = 32});
    cui_label(ui, &app->theme, cui_string("Workspace"));
    cui_end_box(ui);
    cui_resizable_panel panels[] = {
        {.id = FIRST_PANE,
         .initial_size = 152,
         .has_initial_size = true,
         .custom_range = true,
         .minimum = 112,
         .maximum = 800,
         .box = {.padding = 8, .background = pane_colors[0]},
         .content = explorer_pane,
         .content_user = app},
        {.id = SECOND_PANE,
         .custom_range = true,
         .minimum = 112,
         .maximum = 800,
         .box = {.padding = 12, .background = pane_colors[1]},
         .content = document_pane,
         .content_user = app},
    };
    cui_resizable_options options = {
        .extent = host_width,
        .cross_size = host_height - 56,
        .reduced_motion = true,
        .box = {.custom_padding = true, .padding_edges = {8, 12, 6, 14}},
    };
    assert(cui_resizable(ui, GROUP_ID, &app->theme, &scene->state, panels, 2, options) ==
           CUI_RESIZABLE_OK);
    cui_begin_box(ui, WORKSPACE_FOOTER, (cui_box){.width = -1, .height = 24});
    cui_label(ui, &app->theme, cui_string("Local draft"));
    cui_end_box(ui);
    cui_end_box(ui);
    cui_draw_list draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK && scene->completions == completions);
    cui_rect first = cui_bounds(ui, FIRST_PANE), second = cui_bounds(ui, SECOND_PANE);
    cui_rect handle = cui_bounds(ui, cui_resizable_handle_id(GROUP_ID, FIRST_PANE));
    cui_rect input = cui_bounds(ui, WORKSPACE_DOCUMENT);
    near(cui_bounds(ui, WORKSPACE_FOOTER).y, height - 24);
    near(first.x, 8);
    near(first.w, left);
    near(second.x, first.x + first.w);
    near(second.x + second.w, width - 12);
    near(handle.x + 4, second.x);
    near(handle.y, second.y);
    near(handle.h, second.h);
    near(input.x, second.x + 12);
    near(input.y, second.y + 12);
    near(input.w, second.w - 24);
    assert(cui_sdl3_render(scene->host, &draw, app->theme.background));
}

static void size_window(split_scene *scene, int width, int height) {
    SDL_Window *window = SDL_GetWindowFromID(scene->window);
    assert(window && SDL_SetWindowSize(window, width, height) && SDL_SyncWindow(window));
}

static int exercise_workspace(const char *font_path) {
    const char *isolated = getenv("CUI_TEST_VIRTUAL_DISPLAY");
    if (!isolated || strcmp(isolated, "1"))
        return 77;
    cui_font *font = cui_font_open(font_path);
    assert(font);
    char text[80] = "Draft";
    workspace app = {
        .split =
            {
                .host = cui_sdl3_create_with_options(
                    "Resizable workspace", 420, 240, font,
                    (cui_sdl3_window_options){.initially_hidden = true}),
                .ui = cui_create((cui_config){.measure = cui_font_measure,
                                              .measure_styled = cui_font_measure_styled,
                                              .font_metrics = cui_font_get_metrics,
                                              .measure_user = font}),
            },
        .theme = cui_theme_light(),
        .document = cui_edit_create((cui_edit_config){.buffer = text, .capacity = sizeof text}),
    };
    split_scene *scene = &app.split;
    assert(scene->host && scene->ui && app.document);
    assert(cui_resizable_state_init(&scene->state, scene->slots, 2));
    scene->state.on_resize = completed;
    scene->state.resize_user = scene;
    int count;
    SDL_Window **windows = SDL_GetWindows(&count);
    assert(windows && count == 1);
    scene->window = SDL_GetWindowID(windows[0]);
    SDL_free(windows);
    SDL_PumpEvents();
    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);

    workspace_frame(&app, 420, 240, 152, 0);
    mouse(scene, SDL_EVENT_MOUSE_BUTTON_DOWN, 160);
    workspace_frame(&app, 420, 240, 152, 0);
    mouse(scene, SDL_EVENT_MOUSE_MOTION, 228);
    workspace_frame(&app, 420, 240, 220, 0);
    assert(scene->state.dragging);
    size_window(scene, 380, 260);
    mouse(scene, SDL_EVENT_MOUSE_MOTION, 318);
    workspace_frame(&app, 380, 260, 248, 0);
    assert(scene->state.dragging);
    mouse(scene, SDL_EVENT_MOUSE_BUTTON_UP, 318);
    workspace_frame(&app, 380, 260, 248, 1);
    assert(!scene->state.dragging);

    cui_rect input = cui_bounds(scene->ui, WORKSPACE_DOCUMENT);
    SDL_FPoint center = {input.x + input.w / 2, input.y + input.h / 2};
    mouse_at(scene, SDL_EVENT_MOUSE_BUTTON_DOWN, center);
    mouse_at(scene, SDL_EVENT_MOUSE_BUTTON_UP, center);
    workspace_frame(&app, 380, 260, 248, 1);
    assert(cui_focused(scene->ui) == WORKSPACE_DOCUMENT);
    assert(cui_edit_select(app.document, 5, 5) == CUI_EDIT_OK);
    size_window(scene, 360, 220);
    SDL_Event commit = {.type = SDL_EVENT_TEXT_INPUT};
    commit.text.windowID = scene->window;
    commit.text.text = " edit";
    assert(SDL_PushEvent(&commit));
    workspace_frame(&app, 360, 220, 228, 1);
    assert(cui_focused(scene->ui) == WORKSPACE_DOCUMENT && !strcmp(text, "Draft edit"));
    input = cui_bounds(scene->ui, WORKSPACE_DOCUMENT);
    cui_rect caret;
    assert(cui_text_input_rect(scene->ui, &caret));
    assert(caret.x >= input.x && caret.x + caret.w <= input.x + input.w);
    assert(caret.y >= input.y && caret.y + caret.h <= input.y + input.h);

    cui_edit_destroy(app.document);
    cui_destroy(scene->ui);
    cui_sdl3_destroy(scene->host);
    cui_font_close(font);
    return 0;
}

int main(int argc, char **argv) {
    assert(SDL_SetHintWithPriority(SDL_HINT_RENDER_DRIVER, "software", SDL_HINT_OVERRIDE));
    if (argc == 2)
        return exercise_workspace(argv[1]);
    split_scene scene = {
        .host = cui_sdl3_create_with_options("SDL split panes", EXTENT, EXTENT, NULL,
                                             (cui_sdl3_window_options){.initially_hidden = true}),
        .ui = cui_create((cui_config){.max_elements = 128, .max_commands = 256}),
    };
    assert(scene.host && scene.ui);
    int window_count;
    SDL_Window **windows = SDL_GetWindows(&window_count);
    assert(windows && window_count == 1);
    scene.window = SDL_GetWindowID(windows[0]);
    SDL_free(windows);
    SDL_PumpEvents();
    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
    scene.axis = CUI_RESIZABLE_HORIZONTAL;
    exercise(&scene);
    scene.axis = CUI_RESIZABLE_VERTICAL;
    exercise(&scene);
    cui_destroy(scene.ui);
    cui_sdl3_destroy(scene.host);
    return 0;
}
