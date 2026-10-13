/* SPDX-License-Identifier: MIT */
/* Include the adapter unchanged, observing its actual SDL property call while
 * still calling SDL. This is queued-event/Xvfb proof, not OS keyboard evidence. */
#include <SDL3/SDL.h>
#include <assert.h>
static bool observed_start(SDL_Window *, SDL_PropertiesID);
#define SDL_StartTextInputWithProperties observed_start
#include "../../backends/sdl3/cui_sdl3.c"
#undef SDL_StartTextInputWithProperties
#include "cui/cui_components.h"
static unsigned starts;
static SDL_TextInputType observed_type;
static bool autocorrect;
static bool observed_start(SDL_Window *window, SDL_PropertiesID properties) {
    starts++;
    observed_type =
        (SDL_TextInputType)SDL_GetNumberProperty(properties, SDL_PROP_TEXTINPUT_TYPE_NUMBER, -1);
    autocorrect = SDL_GetBooleanProperty(properties, SDL_PROP_TEXTINPUT_AUTOCORRECT_BOOLEAN, true);
    return SDL_StartTextInputWithProperties(window, properties);
}
static void frame(cui_context *ui, cui_edit *edit, cui_input_state *state, cui_input_options o) {
    assert(cui_begin_frame(ui, 400, 120, .016f));
    cui_theme t = cui_theme_light();
    cui_begin_box(ui, 1, (cui_box){.width = -1, .height = -1, .padding = 16});
    assert(cui_input_ex(ui, 10, &t, cui_string("Password"), edit, state, o).error == CUI_EDIT_OK);
    cui_end_box(ui);
    cui_end_frame(ui);
}
int main(void) {
    cui_sdl3 *backend = cui_sdl3_create("CUI password adapter proof", 400, 120, NULL);
    if (!backend)
        return 77;
    cui_context *ui = cui_create((cui_config){0});
    char value[128] = "seed";
    cui_edit *edit = cui_edit_create((cui_edit_config){
        .buffer = value, .capacity = sizeof value, .history_bytes = 4096, .max_history_steps = 16});
    assert(ui && edit);
    cui_input_state state = {0};
    cui_input_options o = {.masked = true, .content_type = CUI_TEXT_INPUT_PURPOSE_NEW_PASSWORD};
    frame(ui, edit, &state, o);
    cui_focus(ui, 10);
    frame(ui, edit, &state, o);
    assert(cui_sdl3_poll(backend, ui));
    assert(starts == 1 && observed_type == SDL_TEXTINPUT_TYPE_TEXT_PASSWORD_HIDDEN &&
           !autocorrect && SDL_TextInputActive(backend->window));
    assert(cui_edit_select(edit, 0, 4) == CUI_EDIT_OK);
    SDL_Event event = {.type = SDL_EVENT_TEXT_EDITING};
    event.edit.text = "😀é";
    event.edit.start = 1;
    event.edit.length = 1;
    assert(SDL_PushEvent(&event));
    assert(cui_sdl3_poll(backend, ui));
    frame(ui, edit, &state, o);
    assert(!strcmp(value, "seed") && state.behavior.composition->active &&
           state.behavior.composition->anchor == 4 && state.behavior.composition->cursor == 6);
    event = (SDL_Event){.type = SDL_EVENT_TEXT_INPUT};
    event.text.text = "😀é";
    assert(SDL_PushEvent(&event));
    assert(cui_sdl3_poll(backend, ui));
    frame(ui, edit, &state, o);
    assert(!strcmp(value, "😀é"));
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && !strcmp(value, "seed"));
    event = (SDL_Event){.type = SDL_EVENT_TEXT_INPUT};
    event.text.text = "stale old mode";
    assert(SDL_PushEvent(&event));
    o.masked = false;
    frame(ui, edit, &state, o);
    assert(cui_sdl3_poll(backend, ui));
    assert(starts == 2 && observed_type == SDL_TEXTINPUT_TYPE_TEXT_PASSWORD_VISIBLE &&
           !autocorrect);
    frame(ui, edit, &state, o);
    assert(!strcmp(value, "seed"));
    o.readonly = true;
    frame(ui, edit, &state, o);
    assert(cui_sdl3_poll(backend, ui));
    assert(!SDL_TextInputActive(backend->window));
    o.readonly = false;
    o.content_type = CUI_TEXT_INPUT_PURPOSE_UNSPECIFIED;
    frame(ui, edit, &state, o);
    assert(cui_sdl3_poll(backend, ui));
    assert(starts == 3 && observed_type == SDL_TEXTINPUT_TYPE_TEXT);
    cui_focus(ui, 0);
    assert(cui_sdl3_poll(backend, ui));
    assert(!SDL_TextInputActive(backend->window));
    cui_destroy(ui);
    cui_edit_destroy(edit);
    cui_sdl3_destroy(backend);
    return 0;
}
