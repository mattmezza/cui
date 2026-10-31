/* SPDX-License-Identifier: MIT */
/* Test-only Linux interposer: independently prove constructor flags and actual
 * first-present SDL state in every language runtime. Never a production hook. */
#define _GNU_SOURCE
#include <SDL3/SDL.h>
#include <X11/Xlib.h>
#include <assert.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
static unsigned creations, first_frames;
static SDL_Window *current;
static bool first_present, expected_hidden;
static void actual_os_state(bool hidden) {
    /* The private Xvfb proof is deliberately X11-specific. Query the real server,
     * independently of SDL's cached flag; no desktop or Wayland fallback. */
    assert(strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0);
    SDL_PropertiesID props = SDL_GetWindowProperties(current);
    Display *display = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);
    Window window = (Window)SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
    assert(display && window);
    void *xlib = dlopen("libX11.so.6", RTLD_NOW | RTLD_LOCAL);
    assert(xlib);
    int (*sync)(Display *, Bool) = NULL;
    Status (*attributes)(Display *, Window, XWindowAttributes *) = NULL;
    void *symbol = dlsym(xlib, "XSync");
    assert(symbol);
    memcpy(&sync, &symbol, sizeof(sync));
    symbol = dlsym(xlib, "XGetWindowAttributes");
    assert(symbol);
    memcpy(&attributes, &symbol, sizeof(attributes));
    XWindowAttributes value;
    sync(display, False);
    assert(attributes(display, window, &value));
    assert(value.map_state == (hidden ? IsUnmapped : IsViewable));
    dlclose(xlib);
}
SDL_Window *SDL_CreateWindow(const char *title, int width, int height, SDL_WindowFlags flags) {
    SDL_Window *(*real_create)(const char *, int, int, SDL_WindowFlags) = NULL;
    void *symbol = dlsym(RTLD_NEXT, "SDL_CreateWindow");
    assert(symbol);
    memcpy(&real_create, &symbol, sizeof(real_create));
    assert(getenv("CUI_TEST_VIRTUAL_DISPLAY") &&
           strcmp(getenv("CUI_TEST_VIRTUAL_DISPLAY"), "1") == 0);
    expected_hidden = creations == 0;
    assert(creations < 2);
    assert(((flags & SDL_WINDOW_HIDDEN) != 0) == expected_hidden);
    current = real_create(title, width, height, flags);
    assert(current);
    assert(((SDL_GetWindowFlags(current) & SDL_WINDOW_HIDDEN) != 0) == expected_hidden);
    actual_os_state(expected_hidden);
    creations++;
    first_present = true;
    return current;
}
bool SDL_RenderPresent(SDL_Renderer *renderer) {
    bool (*real_present)(SDL_Renderer *) = NULL;
    void *symbol = dlsym(RTLD_NEXT, "SDL_RenderPresent");
    assert(symbol);
    memcpy(&real_present, &symbol, sizeof(real_present));
    if (first_present) {
        assert(((SDL_GetWindowFlags(current) & SDL_WINDOW_HIDDEN) != 0) == expected_hidden);
        actual_os_state(expected_hidden);
        first_frames++;
        first_present = false;
    }
    return real_present(renderer);
}
__attribute__((destructor)) static void verify(void) {
    assert(creations == 2 && first_frames == 2);
}
