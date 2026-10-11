/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include "cui/cui_sdl_gpu.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static bool enqueue(cui_context *ui, SDL_Event *e) {
    cui_event v = {0};
    switch (e->type) {
    case SDL_EVENT_MOUSE_MOTION:
        v.type = CUI_EVENT_POINTER_MOVE;
        v.x = e->motion.x;
        v.y = e->motion.y;
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (e->button.button != SDL_BUTTON_LEFT)
            return true;
        v.type =
            e->type == SDL_EVENT_MOUSE_BUTTON_DOWN ? CUI_EVENT_POINTER_DOWN : CUI_EVENT_POINTER_UP;
        v.x = e->button.x;
        v.y = e->button.y;
        break;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
        v.type = e->type == SDL_EVENT_KEY_DOWN ? CUI_EVENT_KEY_DOWN : CUI_EVENT_KEY_UP;
        v.repeat = e->key.repeat;
        v.modifiers = (e->key.mod & SDL_KMOD_SHIFT) ? CUI_MOD_SHIFT : 0;
        switch (e->key.key) {
        case SDLK_TAB:
            v.key = CUI_KEY_TAB;
            break;
        case SDLK_RETURN:
            v.key = CUI_KEY_ENTER;
            break;
        case SDLK_SPACE:
            v.key = CUI_KEY_SPACE;
            break;
        default:
            return true;
        }
        break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        v.type = CUI_EVENT_CANCEL;
        break;
    default:
        cui_invalidate(ui);
        return true;
    }
    return cui_queue_event(ui, v);
}
int main(int argc, char **argv) {
    int frames = 0;
    const char *capture = NULL, *font_path = getenv("CUI_FONT");
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--smoke") && i + 1 < argc)
            frames = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--screenshot") && i + 1 < argc)
            capture = argv[++i];
        else if (!strcmp(argv[i], "--font") && i + 1 < argc)
            font_path = argv[++i];
        else
            return 2;
    }
    if (!font_path)
        font_path = "/usr/share/fonts/noto/NotoSans-Regular.ttf";
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "%s\n", SDL_GetError());
        return 1;
    }
    SDL_Window *window = SDL_CreateWindow("CUI / SDL_GPU renderer", 720, 480, SDL_WINDOW_RESIZABLE);
    SDL_GPUDevice *device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
    cui_font *font = cui_font_open(font_path);
    cui_sdl_gpu *gpu = NULL;
    cui_context *ui = NULL;
    int result = 1;
    if (!window || !device || !font || !SDL_ClaimWindowForGPUDevice(device, window))
        goto cleanup;
    gpu = cui_sdl_gpu_create(device, window, font);
    if (!gpu)
        goto cleanup;
    ui = cui_create((cui_config){
        .width = 720, .height = 480, .measure = cui_font_measure, .measure_user = font});
    if (!ui)
        goto cleanup;
    printf("SDL_GPU backend=%s video=%s\n", cui_sdl_gpu_driver(gpu), SDL_GetCurrentVideoDriver());
#if SDL_VERSION_ATLEAST(3, 4, 0)
    SDL_PropertiesID properties = SDL_GetGPUDeviceProperties(device);
    printf("Device=%s; driver=%s (reported by SDL; backend alone does not prove hardware)\n",
           SDL_GetStringProperty(properties, SDL_PROP_GPU_DEVICE_NAME_STRING, "unknown"),
           SDL_GetStringProperty(properties, SDL_PROP_GPU_DEVICE_DRIVER_NAME_STRING, "unknown"));
#endif
    bool quit = false, dark = false, enabled = true;
    int count = 0, drawn = 0;
    while (!quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED ||
                (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE))
                quit = true;
            else if (!enqueue(ui, &event))
                goto cleanup;
        }
        if (quit)
            break;
        if (!frames && !cui_needs_redraw(ui)) {
            SDL_Delay(10);
            continue;
        }
        int width, height;
        SDL_GetWindowSize(window, &width, &height);
        if (!width || !height) {
            SDL_Delay(10);
            continue;
        }
        cui_theme theme = dark ? cui_theme_dark() : cui_theme_light();
        if (!cui_begin_frame(ui, (float)width, (float)height, 1.f / 60))
            goto cleanup;
        cui_begin_box(ui, cui_id_from(ui, "root"),
                      (cui_box){.width = -1,
                                .height = -1,
                                .padding = 32,
                                .gap = 18,
                                .background = theme.background});
        cui_text(ui, cui_string("One UI. Independent renderers."), 26, theme.text);
        cui_label(ui, &theme,
                  cui_string("Clay layout / SDL_GPU Vulkan / cached shaped glyph atlas"));
        if (cui_button(ui, cui_id_from(ui, "increment"), &theme, cui_string("Activate counter"),
                       !enabled)) {
            count++;
            cui_invalidate(ui);
        }
        char label[80];
        snprintf(label, sizeof(label), "Activations: %d", count);
        cui_label(ui, &theme, cui_string(label));
        if (cui_switch(ui, cui_id_from(ui, "dark"), &theme, cui_string("Dark appearance"), &dark,
                       false))
            cui_invalidate(ui);
        if (cui_checkbox(ui, cui_id_from(ui, "enabled"), &theme, cui_string("Enable counter"),
                         &enabled, false))
            cui_invalidate(ui);
        cui_begin_box(ui, cui_id_from(ui, "clip"),
                      (cui_box){.width = 420,
                                .height = 64,
                                .padding = 12,
                                .clip = true,
                                .radius = 8,
                                .background = theme.surface});
        cui_label(ui, &theme, cui_string("Clipped text: cafe / café / Ελληνικά"));
        cui_end_box(ui);
        cui_end_box(ui);
        cui_draw_list list = cui_end_frame(ui);
        if (cui_last_error(ui) != CUI_OK || !cui_sdl_gpu_render(gpu, &list, theme.background))
            goto cleanup;
        if (frames && ++drawn >= frames) {
            if (capture && !cui_sdl_gpu_save_ppm(gpu, capture))
                goto cleanup;
            break;
        }
    }
    result = 0;
cleanup:
    if (result)
        fprintf(stderr, "SDL_GPU demo failed: %s (font: %s)\n", SDL_GetError(), font_path);
    cui_destroy(ui);
    cui_sdl_gpu_destroy(gpu);
    cui_font_close(font);
    if (device && window)
        SDL_ReleaseWindowFromGPUDevice(device, window);
    if (device)
        SDL_DestroyGPUDevice(device);
    if (window)
        SDL_DestroyWindow(window);
    SDL_Quit();
    return result;
}
