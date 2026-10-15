/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include "cui/cui_content.h"
#include "cui/cui_scroll.h"
#include <stdio.h>

/* Acceptance fixture, not a production mail service. State lives until exit. */
static cui_scroll_state folders_scroll, messages_scroll, reader_scroll;
static unsigned selected;
static bool stars[12];

void app_frame(cui_context *ui, const cui_theme *theme, bool *composing) {
    cui_begin_box(ui, cui_id_from(ui, "mail"),
                  (cui_box){.width = -1,
                            .height = -1,
                            .row = true,
                            .gap = 8,
                            .padding = 8,
                            .clip = true,
                            .background = theme->background});
    cui_scroll_options folders = {.width = 140,
                                  .height = 390,
                                  .content_height = 680,
                                  .vertical = true,
                                  .hide_scrollbars = true};
    cui_id folder_id = cui_id_from(ui, "folders");
    if (cui_scrollable_begin(ui, folder_id, theme, &folders_scroll, folders)) {
        if (cui_button(ui, cui_id_from(ui, "compose"), theme, cui_string("Compose"), false))
            *composing = true;
        for (unsigned i = 0; i < 18; ++i) {
            char key[32];
            snprintf(key, sizeof key, "Folder %u", i);
            (void)cui_button(ui, cui_id_from(ui, key), theme, cui_string(key), false);
        }
        cui_scrollable_end_ex(ui, folder_id, theme, &folders_scroll, folders);
    }
    cui_scroll_options messages = {.width = 285,
                                   .height = 390,
                                   .content_height = 1200,
                                   .vertical = true,
                                   .hide_scrollbars = true};
    cui_id messages_id = cui_id_from(ui, "messages");
    if (cui_scrollable_begin(ui, messages_id, theme, &messages_scroll, messages)) {
        for (unsigned i = 0; i < 12; ++i) {
            char key[24], label[24];
            snprintf(key, sizeof key, "message-%u", i);
            snprintf(label, sizeof label, "Open %u", i);
            if (!cui_push_id(ui, key))
                continue;
            cui_begin_box(ui, cui_id_from(ui, "row"),
                          (cui_box){.width = -1,
                                    .height = 100,
                                    .fixed_height = true,
                                    .row = true,
                                    .gap = 4,
                                    .clip = true});
            cui_avatar(ui, cui_id_from(ui, "avatar"), theme, cui_string("Alex Morgan"), 28);
            cui_begin_box(ui, cui_id_from(ui, "text"),
                          (cui_box){.width = -1, .max_width = 175, .clip = true});
            cui_text_line(ui, cui_string("An intentionally long subject about the next release"),
                          14, theme->text, 0, (cui_text_style){0});
            cui_badge(ui, cui_id_from(ui, "tag"), theme, cui_string("Work"));
            if (cui_button(ui, cui_id_from(ui, "open"), theme, cui_string(label), false)) {
                selected = i;
                *composing = false;
            }
            cui_end_box(ui);
            cui_begin_box(ui, cui_id_from(ui, "actions"),
                          (cui_box){.width = 62, .fixed_width = true});
            cui_text(ui, cui_string("10:42"), 12, theme->text);
            if (cui_button(ui, cui_id_from(ui, "star"), theme,
                           cui_string(stars[i] ? "Unstar" : "Star"), false))
                stars[i] = !stars[i];
            cui_end_box(ui);
            cui_end_box(ui);
            cui_pop_id(ui);
        }
        cui_scrollable_end_ex(ui, messages_id, theme, &messages_scroll, messages);
    }
    cui_scroll_options reader = {.width = 260,
                                 .height = 390,
                                 .content_height = 900,
                                 .vertical = true,
                                 .hide_scrollbars = true};
    cui_id reader_id = cui_id_from(ui, "reader");
    if (cui_scrollable_begin(ui, reader_id, theme, &reader_scroll, reader)) {
        char heading[64];
        snprintf(heading, sizeof heading, "Message %u%s", selected,
                 stars[selected] ? " (starred)" : "");
        cui_text(ui, cui_string(*composing ? "New message" : heading), 20, theme->text);
        if (*composing) {
            cui_text(ui, cui_string("Compose preview: editing and delivery are not implemented."),
                     16, theme->text);
            if (cui_button(ui, cui_id_from(ui, "discard"), theme, cui_string("Discard"), false))
                *composing = false;
        } else {
            for (unsigned i = 0; i < 15; ++i)
                cui_text(ui,
                         cui_string(
                             "Long message body wraps inside its independently scrollable reader."),
                         16, theme->text);
        }
        cui_scrollable_end_ex(ui, reader_id, theme, &reader_scroll, reader);
    }
    cui_end_box(ui);
}
