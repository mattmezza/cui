/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include "cui/cui_sdl3.h"
#include "cui/cui_software.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {
    bool dark, contrast, disabled, checked, notifications;
    int section, choice, clicks;
    float volume;
    cui_edit *edit;
} gallery_state;
static cui_id id(cui_context *ui, const char *key) {
    return cui_id_from(ui, key);
}
static void label(cui_context *ui, const cui_theme *t, const char *s) {
    cui_label(ui, t, cui_string(s));
}
static void heading(cui_context *ui, const cui_theme *t, const char *s) {
    cui_text_ex(ui, cui_string(s), 24, t->text, 0,
                (cui_text_style){.weight = CUI_TEXT_WEIGHT_SEMIBOLD});
}
static void box(cui_context *ui, const char *key, float width, float height, bool row, float pad,
                float gap, cui_color bg) {
    cui_begin_box(ui, id(ui, key),
                  (cui_box){.width = width,
                            .height = height,
                            .row = row,
                            .padding = pad,
                            .gap = gap,
                            .background = bg});
}
static cui_draw_list frame(cui_context *ui, gallery_state *s, float w, float h) {
    gallery_state before = *s;
    uint64_t revision = cui_edit_get_state(s->edit).revision;
    cui_theme t = s->contrast ? cui_theme_high_contrast()
                  : s->dark   ? cui_theme_dark()
                              : cui_theme_light();
    cui_begin_frame(ui, w, h, 0.016f);
    cui_begin_box(ui, id(ui, "root"),
                  (cui_box){.width = w, .height = h, .clip = true, .background = t.background});
    /* Below the supported specimen size, do not submit controls that cannot be
       seen. This also removes their focus targets from the current tree. */
    if (w < 600 || h < 700) {
        box(ui, "resize_notice", -1, -1, false, 12, 10, t.background);
        cui_text(ui, cui_string("Window too small"), 16, t.text);
        cui_text(ui, cui_string("Resize to 600 x 700"), 14, t.text);
        cui_end_box(ui);
        cui_end_box(ui);
        return cui_end_frame(ui);
    }
    bool compact = w < 800;
    box(ui, "toolbar", -1, compact ? 64 : 84, true, compact ? 16 : 24, compact ? 16 : 32,
        t.surface);
    heading(ui, &t, "CUI / component workbench");
    cui_badge(ui, id(ui, "version"), &t, cui_string("Development"));
    cui_end_box(ui);
    box(ui, "workspace", -1, -1, true, compact ? 16 : 24, 32, t.background);
    if (w >= 800) {
        box(ui, "navigation", 210, -1, false, 0, 10, (cui_color){0});
        cui_text(ui, cui_string("Explore"), 18, t.text);
        if (cui_button(ui, id(ui, "nav_controls"), &t, cui_string("Controls"), false))
            s->section = 0;
        if (cui_button(ui, id(ui, "nav_selection"), &t, cui_string("Selection & values"), false))
            s->section = 1;
        if (cui_button(ui, id(ui, "nav_text"), &t, cui_string("Text input"), false))
            s->section = 2;
        if (cui_button(ui, id(ui, "nav_about"), &t, cui_string("About this build"), false))
            s->section = 3;
        cui_separator(ui, id(ui, "nav_separator"), &t, 210);
        cui_text(ui, cui_string("Appearance"), 18, t.text);
        cui_switch(ui, id(ui, "dark"), &t, cui_string("Dark theme"), &s->dark, false);
        cui_checkbox(ui, id(ui, "contrast"), &t, cui_string("High contrast"), &s->contrast, false);
        cui_checkbox(ui, id(ui, "disabled"), &t, cui_string("Disable examples"), &s->disabled,
                     false);
        cui_text(ui, cui_string("Tab / Shift+Tab: focus"), 12, t.muted);
        cui_text(ui, cui_string("Enter / Space: activate"), 12, t.muted);
        cui_text(ui, cui_string("Arrows: adjust values"), 12, t.muted);
        cui_end_box(ui);
    }
    box(ui, "content", -1, -1, false, 0, compact ? 10 : 18, (cui_color){0});
    if (w < 800) {
        cui_switch(ui, id(ui, "dark"), &t, cui_string("Dark theme"), &s->dark, false);
        box(ui, "compact_navigation", -1, 0, true, 0, 10, (cui_color){0});
        if (cui_button(ui, id(ui, "nav_controls"), &t, cui_string("Controls"), false))
            s->section = 0;
        if (cui_button(ui, id(ui, "nav_selection"), &t, cui_string("Values"), false))
            s->section = 1;
        if (cui_button(ui, id(ui, "nav_text"), &t, cui_string("Text"), false))
            s->section = 2;
        if (cui_button(ui, id(ui, "nav_about"), &t, cui_string("About"), false))
            s->section = 3;
        cui_end_box(ui);
    }
    if (s->section == 0) {
        heading(ui, &t, "Everyday controls");
        label(ui, &t, "Real pointer and keyboard behavior, built on unstyled CUI Base.");
        cui_separator(ui, id(ui, "main_separator"), &t, -1);
        cui_text(ui, cui_string("Actions"), 18, t.text);
        box(ui, "actions", -1, 0, true, 0, 12, (cui_color){0});
        if (cui_button(ui, id(ui, "action"), &t, cui_string("Run action"), s->disabled))
            s->clicks++;
        cui_button(ui, id(ui, "disabled_action"), &t, cui_string("Unavailable"), true);
        cui_toggle(ui, id(ui, "toggle"), &t, cui_string("Pinned"), &s->checked, s->disabled);
        cui_end_box(ui);
        char result[80];
        snprintf(result, sizeof(result), "Action activated %d time%s", s->clicks,
                 s->clicks == 1 ? "" : "s");
        label(ui, &t, result);
        cui_text(ui, cui_string("Preferences"), 18, t.text);
        cui_checkbox(ui, id(ui, "check"), &t, cui_string("Keep this workspace open"), &s->checked,
                     s->disabled);
        cui_switch(ui, id(ui, "notify"), &t, cui_string("Desktop notifications"), &s->notifications,
                   s->disabled);
        cui_checkbox(ui, id(ui, "disabled_check"), &t, cui_string("Managed by administrator"),
                     &s->checked, true);
        cui_text(ui, cui_string("One shared value links the checkbox and pin toggle."), 13,
                 t.muted);
    } else if (s->section == 1) {
        heading(ui, &t, "Selection & values");
        label(ui, &t,
              "Controlled values remain in the application, independent of the "
              "theme.");
        cui_separator(ui, id(ui, "selection_separator"), &t, -1);
        cui_text(ui, cui_string("Update channel"), 18, t.text);
        if (compact)
            box(ui, "channel_options", -1, 0, true, 0, 10, (cui_color){0});
        cui_radio(ui, id(ui, "stable"), &t, cui_string(compact ? "Stable" : "Stable releases"),
                  &s->choice, 0, s->disabled);
        cui_radio(ui, id(ui, "preview"), &t, cui_string(compact ? "Preview" : "Preview releases"),
                  &s->choice, 1, s->disabled);
        cui_radio(ui, id(ui, "nightly"), &t, cui_string(compact ? "Nightly" : "Nightly builds"),
                  &s->choice, 2, s->disabled);
        if (compact)
            cui_end_box(ui);
        cui_text(ui, cui_string("Volume"), 18, t.text);
        cui_slider(ui, id(ui, "volume"), &t, cui_string("Volume"), &s->volume, 0, 100, 5,
                   s->disabled);
        char value[64];
        snprintf(value, sizeof(value), "%.0f / 100", s->volume);
        label(ui, &t, value);
        cui_progress(ui, id(ui, "progress"), &t, s->volume / 100, 280);
        cui_badge(ui, id(ui, "channel"), &t,
                  cui_string(s->choice == 0   ? "Stable"
                             : s->choice == 1 ? "Preview"
                                              : "Nightly"));
    } else if (s->section == 2) {
        heading(ui, &t, "Text input / preview");
        label(ui, &t, "UTF-8 editing with keyboard selection and bounded undo history.");
        cui_separator(ui, id(ui, "text_separator"), &t, -1);
        cui_input_result result = cui_input(ui, id(ui, "workspace_name"), &t,
                                            cui_string("Workspace name"), s->edit, s->disabled);
        if (result.error == CUI_EDIT_CAPACITY)
            label(ui, &t, "Name too long; shorten it and try again.");
        else if (result.error != CUI_EDIT_OK)
            label(ui, &t, "That edit could not be applied.");
        if (result.submitted)
            label(ui, &t, "Name submitted.");
        label(ui, &t, "Arrow keys move; Shift selects; Ctrl+A selects all.");
        label(ui, &t, "Ctrl+Z undoes; Ctrl+Shift+Z redoes.");
        cui_text(ui, cui_string("Preview limitation: caret and selection are not painted yet."), 13,
                 t.muted);
        cui_text(ui, cui_string("IME preedit and grapheme-aware cursor movement remain planned."),
                 13, t.muted);
    } else {
        heading(ui, &t, "Small core. Honest scope.");
        label(ui, &t, "C11 public APIs / Clay layout / independent renderer adapters");
        cui_separator(ui, id(ui, "about_separator"), &t, -1);
        label(ui, &t, "This gallery demonstrates the implemented vertical slice.");
        label(ui, &t,
              "The complete 78-component target is tracked in the repository "
              "manifest.");
        label(ui, &t, "International text uses FreeType rasterization and HarfBuzz shaping.");
        label(ui, &t,
              "SDL_GPU is available through a separate explicit adapter "
              "demonstration.");
        label(ui, &t, "Native accessibility bridges remain planned.");
        cui_text(ui,
                 cui_string("Shaping sample: caf\xc3\xa9 / "
                            "\xce\x95\xce\xbb\xce\xbb\xce\xb7\xce\xbd\xce\xb9\xce\xba\xce\xac"),
                 18, t.text);
    }
    cui_end_box(ui);
    cui_end_box(ui);
    box(ui, "footer", -1, compact ? 36 : 44, true, compact ? 8 : 12, 16, t.surface);
    cui_text(ui,
             cui_string(compact ? "CUI development / Tab moves focus"
                                : "CUI 0.1 / state and semantics are independent of "
                                  "painted pixels"),
             12, t.muted);
    cui_end_box(ui);
    cui_end_box(ui);
    cui_draw_list result = cui_end_frame(ui);
    if (before.dark != s->dark || before.contrast != s->contrast ||
        before.disabled != s->disabled || before.section != s->section ||
        before.clicks != s->clicks || before.checked != s->checked ||
        before.notifications != s->notifications || before.choice != s->choice ||
        before.volume != s->volume || revision != cui_edit_get_state(s->edit).revision)
        cui_invalidate(ui);
    return result;
}
static const char *font_path(const char *requested) {
    if (requested)
        return requested;
    const char *env = getenv("CUI_FONT_PATH");
    if (env && *env)
        return env;
    const char *paths[] = {
        "/usr/share/fonts/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf", "C:/Windows/Fonts/segoeui.ttf"};
    for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); i++) {
        FILE *f = fopen(paths[i], "rb");
        if (f) {
            fclose(f);
            return paths[i];
        }
    }
    return NULL;
}
static void register_matching_font_styles(cui_font *font, const char *path) {
    /* Only use variants of the actual selected family. A custom --font or
     * CUI_FONT_PATH keeps regular fallback unless it names this known face. */
    if (!font || !path || strcmp(path, "/usr/share/fonts/noto/NotoSans-Regular.ttf"))
        return;
    (void)cui_font_register_style(font, (cui_text_style){.weight = CUI_TEXT_WEIGHT_MEDIUM},
                                  "/usr/share/fonts/noto/NotoSans-Medium.ttf");
    (void)cui_font_register_style(font, (cui_text_style){.weight = CUI_TEXT_WEIGHT_SEMIBOLD},
                                  "/usr/share/fonts/noto/NotoSans-SemiBold.ttf");
    (void)cui_font_register_style(font, (cui_text_style){.weight = CUI_TEXT_WEIGHT_BOLD},
                                  "/usr/share/fonts/noto/NotoSans-Bold.ttf");
    (void)cui_font_register_style(font, (cui_text_style){.italic = true},
                                  "/usr/share/fonts/noto/NotoSans-Italic.ttf");
    (void)cui_font_register_style(font, (cui_text_style){.monospace = true},
                                  "/usr/share/fonts/noto/NotoSansMono-Regular.ttf");
}
int main(int argc, char **argv) {
    bool headless = false, dark = false;
    unsigned smoke = 0, width = 1100, height = 740;
    int section = 0;
    const char *capture = NULL, *requested_font = NULL;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--headless"))
            headless = true;
        else if (!strcmp(argv[i], "--dark"))
            dark = true;
        else if (!strcmp(argv[i], "--smoke") && i + 1 < argc)
            smoke = (unsigned)strtoul(argv[++i], NULL, 10);
        else if (!strcmp(argv[i], "--screenshot") && i + 1 < argc)
            capture = argv[++i];
        else if (!strcmp(argv[i], "--font") && i + 1 < argc)
            requested_font = argv[++i];
        else if (!strcmp(argv[i], "--section") && i + 1 < argc) {
            const char *value = argv[++i];
            if (!strcmp(value, "controls"))
                section = 0;
            else if (!strcmp(value, "values"))
                section = 1;
            else if (!strcmp(value, "text"))
                section = 2;
            else if (!strcmp(value, "about"))
                section = 3;
            else {
                fprintf(stderr, "Unknown section: %s\n", value);
                return 2;
            }
        } else if ((!strcmp(argv[i], "--width") || !strcmp(argv[i], "--height")) && i + 1 < argc) {
            bool is_width = !strcmp(argv[i], "--width");
            char *end;
            errno = 0;
            unsigned long value = strtoul(argv[++i], &end, 10);
            if (errno || *end || !value || value > 16384) {
                fprintf(stderr, "Dimensions must be integers from 1 to 16384.\n");
                return 2;
            }
            if (is_width)
                width = (unsigned)value;
            else
                height = (unsigned)value;
        } else {
            fprintf(stderr,
                    "Usage: %s [--headless] [--dark] [--smoke N] [--screenshot FILE] "
                    "[--font FILE] [--section controls|values|text|about] [--width "
                    "N] [--height N]\n",
                    argv[0]);
            return 2;
        }
    }
    const char *selected_font = font_path(requested_font);
    cui_font *font = cui_font_open(selected_font);
    if (!font) {
        fprintf(stderr, "No usable font. Pass --font FILE or set CUI_FONT_PATH.\n");
        return 1;
    }
    register_matching_font_styles(font, selected_font);
    cui_context *ui = cui_create((cui_config){.width = (float)width,
                                              .height = (float)height,
                                              .measure = cui_font_measure,
                                              .measure_styled = cui_font_measure_styled,
                                              .measure_user = font});
    if (!ui) {
        cui_font_close(font);
        return 1;
    }
    char workspace_name[128] = "Personal workspace";
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = workspace_name,
                                                       .capacity = sizeof(workspace_name),
                                                       .history_bytes = 2048,
                                                       .max_history_steps = 24});
    if (!edit) {
        cui_destroy(ui);
        cui_font_close(font);
        return 1;
    }
    gallery_state state = {
        .dark = dark, .notifications = true, .volume = 65, .edit = edit, .section = section};
    bool ok = true;
    if (headless) {
        cui_software *r = cui_software_create(width, height, font);
        cui_draw_list list = frame(ui, &state, (float)width, (float)height);
        ok = r && cui_software_render(
                      r, &list, dark ? cui_theme_dark().background : cui_theme_light().background);
        if (ok && capture)
            ok = cui_software_save_ppm(r, capture);
        printf("renderer=CUI software; frame commands=%zu; capture=%s\n", list.count,
               capture ? capture : "none");
        cui_software_destroy(r);
    } else {
        cui_sdl3 *s = cui_sdl3_create("CUI / component workbench", (int)width, (int)height, font);
        if (!s) {
            fprintf(stderr, "SDL3: %s\n", cui_sdl3_error());
            ok = false;
        } else {
            printf("video=%s; renderer=%s; SDL_Renderer compatibility adapter\n",
                   cui_sdl3_video_driver(), cui_sdl3_driver(s));
            unsigned frames = 0;
            while (cui_sdl3_poll(s, ui)) {
                if (cui_needs_redraw(ui) || frames == 0 || smoke) {
                    float w, h;
                    cui_sdl3_size(s, &w, &h);
                    cui_draw_list list = frame(ui, &state, w, h);
                    cui_theme t = state.contrast ? cui_theme_high_contrast()
                                  : state.dark   ? cui_theme_dark()
                                                 : cui_theme_light();
                    const char *shot = capture && smoke && frames + 1 >= smoke ? capture : NULL;
                    if (!cui_sdl3_render_capture(s, &list, t.background, shot)) {
                        fprintf(stderr, "Render: %s\n", cui_sdl3_error());
                        ok = false;
                        break;
                    }
                    frames++;
                    if (smoke && frames >= smoke)
                        break;
                } else
                    cui_sdl3_wait(8);
            }
            cui_sdl3_destroy(s);
        }
    }
    if (cui_last_error(ui) != CUI_OK) {
        fprintf(stderr, "Core error: %d\n", cui_last_error(ui));
        ok = false;
    }
    cui_edit_destroy(edit);
    cui_destroy(ui);
    cui_font_close(font);
    return ok ? 0 : 1;
}
