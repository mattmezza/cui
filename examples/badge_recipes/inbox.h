/* SPDX-License-Identifier: MIT */
#ifndef CUI_BADGE_INBOX_EXAMPLE_H
#define CUI_BADGE_INBOX_EXAMPLE_H

#include "cui/cui_components.h"
#include "cui/cui_content.h"
#include <stdio.h>

typedef enum {
    BADGE_INBOX_NO_SELECTION,
    BADGE_INBOX_MESSAGES,
    BADGE_INBOX_ESCALATIONS,
    BADGE_INBOX_ALEX,
    BADGE_INBOX_JAMIE,
    BADGE_INBOX_ROBIN
} badge_inbox_selection;

typedef struct {
    size_t messages, escalations, alex_messages;
    badge_inbox_selection selection;
} badge_inbox_state;

typedef struct {
    cui_bitmap bitmap;
    cui_avatar_image *cache;
} badge_inbox_photo;

static inline badge_inbox_state badge_inbox_defaults(void) {
    return (badge_inbox_state){.messages = 7, .escalations = 125, .alex_messages = 5};
}

static inline cui_badge_options badge_inbox_count(const char *name, size_t count, size_t maximum,
                                                  cui_color color, char *label, size_t label_size) {
    /* The marker displays the bounded count. Its accessible name gives the full number. */
    snprintf(label, label_size, "%s: %zu unread", name, count);
    cui_badge_options mark = cui_badge_defaults();
    mark.count = count;
    mark.maximum = maximum;
    mark.name = cui_string(label);
    mark.custom_color = true;
    mark.color = color;
    return mark;
}

typedef struct {
    badge_inbox_selection *selection;
    badge_inbox_selection target;
    size_t *unread;
} badge_inbox_action;

static inline void badge_inbox_activate(cui_context *ui, badge_inbox_action action) {
    *action.selection =
        *action.selection == action.target ? BADGE_INBOX_NO_SELECTION : action.target;
    if (action.unread)
        *action.unread = 0;
    /* Badge captured its count before submitting the child. Draw the updated count next. */
    cui_invalidate(ui);
}

typedef struct {
    const cui_theme *theme;
    const char *label, *icon;
    bool disabled;
    badge_inbox_action action;
} badge_inbox_icon_slot;

static inline void badge_inbox_icon(cui_context *ui, void *user) {
    const badge_inbox_icon_slot *slot = user;
    if (cui_button_ex(ui, cui_id_from(ui, slot->label), slot->theme, cui_string(slot->label),
                      (cui_button_options){.variant = CUI_BUTTON_SECONDARY,
                                           .height = 44,
                                           .icon = cui_string(slot->icon),
                                           .icon_only = true,
                                           .disabled = slot->disabled}))
        badge_inbox_activate(ui, slot->action);
}

typedef struct {
    const cui_theme *theme;
    const char *name;
    badge_inbox_photo photo;
    bool disabled;
    badge_inbox_action action;
} badge_inbox_member;

static inline void badge_inbox_avatar(cui_context *ui, void *user) {
    const badge_inbox_member *member = user;
    if (cui_avatar_ex(ui, cui_id_from(ui, member->name), member->theme, cui_string(member->name),
                      (cui_avatar_options){.size = 48,
                                           .ring_width = 2,
                                           .interactive = true,
                                           .disabled = member->disabled,
                                           .identity_colors = true,
                                           .image = member->photo.bitmap,
                                           .image_state = member->photo.cache}))
        badge_inbox_activate(ui, member->action);
}

static inline void badge_inbox_verified(cui_context *ui, void *user) {
    const badge_inbox_member *member = user;
    cui_badge_options mark = cui_badge_defaults();
    mark.variant = CUI_BADGE_ICON;
    mark.icon = cui_string("check");
    mark.name = cui_string("Alex Morgan is verified");
    mark.custom_color = true;
    mark.color = (cui_color){21, 110, 77, 255};
    cui_badge_ex(ui, cui_id_from(ui, "verified"), member->theme, mark, badge_inbox_avatar, user);
}

/* Initialize state with badge_inbox_defaults, keep it between frames, and submit
 * with a stable unique key. Provide three optional photo/cache pairs in displayed
 * order. Initialize caches with cui_avatar_image_init and keep caches and source
 * pixels valid through rendering. NULL photos uses initials. */
static inline void badge_inbox_draw(cui_context *ui, const char *key, const cui_theme *theme,
                                    badge_inbox_state *state, const badge_inbox_photo *photos,
                                    float available_width, bool disabled) {
    if (!theme || !state || !cui_push_id(ui, key))
        return;
    cui_begin_box(ui, cui_id_from(ui, "card"),
                  (cui_box){.width = -1,
                            .max_width = 420,
                            .padding = 14,
                            .gap = 14,
                            .radius = theme->radius_lg,
                            .border_width = 1,
                            .border = theme->border,
                            .background = theme->surface});
    cui_text(ui, cui_string("Support inbox"), theme->font_size + 3, theme->text);

    badge_inbox_icon_slot slots[] = {
        {theme,
         "Open inbox",
         "inbox",
         disabled,
         {&state->selection, BADGE_INBOX_MESSAGES, &state->messages}},
        {theme,
         "Open escalations",
         "bell",
         disabled,
         {&state->selection, BADGE_INBOX_ESCALATIONS, &state->escalations}}};
    char counts[2][64];
    cui_badge_options marks[] = {
        badge_inbox_count("Inbox", state->messages, 99, (cui_color){25, 75, 145, 255}, counts[0],
                          sizeof counts[0]),
        badge_inbox_count("Escalations", state->escalations, 99, (cui_color){157, 39, 61, 255},
                          counts[1], sizeof counts[1])};
    cui_begin_box(ui, cui_id_from(ui, "notifications"),
                  (cui_box){.row = true, .align_y = CUI_ALIGN_CENTER, .gap = 20});
    cui_badge_ex(ui, cui_id_from(ui, "inbox-count"), theme, marks[0], badge_inbox_icon, &slots[0]);
    cui_badge_ex(ui, cui_id_from(ui, "escalation-count"), theme, marks[1], badge_inbox_icon,
                 &slots[1]);
    cui_end_box(ui);

    cui_text(ui, cui_string("On-call team"), theme->font_size, theme->muted);
    badge_inbox_member members[] = {
        {.theme = theme,
         .name = "Alex Morgan",
         .disabled = disabled,
         .action = {&state->selection, BADGE_INBOX_ALEX, &state->alex_messages}},
        {.theme = theme,
         .name = "Jamie Chen",
         .disabled = disabled,
         .action = {&state->selection, BADGE_INBOX_JAMIE, NULL}},
        {.theme = theme,
         .name = "Robin Hayes",
         .disabled = disabled,
         .action = {&state->selection, BADGE_INBOX_ROBIN, NULL}}};
    if (photos)
        for (size_t i = 0; i < sizeof members / sizeof members[0]; ++i)
            members[i].photo = photos[i];
    char alex_count[64];
    cui_badge_options alex =
        badge_inbox_count("Alex Morgan", state->alex_messages, 99, (cui_color){157, 39, 61, 255},
                          alex_count, sizeof alex_count);
    cui_badge_options jamie = cui_badge_defaults();
    jamie.variant = CUI_BADGE_DOT;
    jamie.name = cui_string("Jamie Chen is online");
    jamie.custom_color = true;
    jamie.color = theme->success;
    cui_badge_options robin = jamie;
    robin.name = cui_string("Robin Hayes is away");
    robin.color = theme->warning;
    cui_begin_box(ui, cui_id_from(ui, "team"),
                  (cui_box){.row = available_width >= 220, .align_y = CUI_ALIGN_CENTER, .gap = 14});
    cui_badge_ex(ui, cui_id_from(ui, "alex-count"), theme, alex, badge_inbox_verified, &members[0]);
    cui_badge_ex(ui, cui_id_from(ui, "jamie-status"), theme, jamie, badge_inbox_avatar,
                 &members[1]);
    cui_badge_ex(ui, cui_id_from(ui, "robin-status"), theme, robin, badge_inbox_avatar,
                 &members[2]);
    cui_end_box(ui);

    if (cui_button_ex(ui, cui_id_from(ui, "restore"), theme, cui_string("Restore notifications"),
                      (cui_button_options){.size = CUI_CONTROL_SMALL,
                                           .variant = CUI_BUTTON_GHOST,
                                           .disabled = disabled})) {
        *state = badge_inbox_defaults();
        cui_invalidate(ui);
    }
    static const char *const details[] = {"Select an inbox or team member",
                                          "Inbox opened",
                                          "Escalations opened",
                                          "Alex Morgan opened",
                                          "Jamie Chen opened",
                                          "Robin Hayes opened"};
    size_t selection = (size_t)state->selection;
    cui_str detail = cui_string(
        details[selection < sizeof details / sizeof details[0] ? selection
                                                               : BADGE_INBOX_NO_SELECTION]);
    cui_id summary = cui_id_from(ui, "selection");
    cui_begin_box(ui, summary, (cui_box){0});
    cui_interact(ui, summary, CUI_ROLE_STATUS, detail, false, false, 0);
    cui_set_semantic_details(ui, summary, (cui_semantic_details){.no_focus = true});
    cui_set_tab_stop(ui, summary, false);
    cui_text(ui, detail, theme->font_size, theme->muted);
    cui_end_box(ui);
    cui_end_box(ui);
    cui_pop_id(ui);
}
#endif
