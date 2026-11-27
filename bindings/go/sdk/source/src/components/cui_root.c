/* SPDX-License-Identifier: MIT */
#include "../base/cui_root_internal.h"
#include "cui/cui_dialog_composition_styled.h"
#include <math.h>
#include <string.h>

/* The existing callback field tags composed entries without changing the
 * caller-owned Root or stack record ABI. This callback is never submitted. */
static void cui_root_composed_dialog_marker(cui_context *ui, void *user) {
    (void)ui;
    (void)user;
}

bool cui_root_dialog_open_composed_v1(cui_root *root, cui_context *ui,
                                      cui_root_composed_dialog_v1 *dialog, cui_id trigger) {
    if (!ui || !dialog || !dialog->layout || !dialog->layout->surface ||
        dialog->layout->surface->overlay.open ||
        !cui_dialog_composition_valid_v1(dialog->composition) ||
        !isfinite(dialog->options.margin) || dialog->options.margin < 0)
        return false;
    cui_root_surface entry = {.surface = dialog->layout->surface,
                              .content = cui_root_composed_dialog_marker,
                              .user = dialog};
    if (!cui_root_dialog_register(root, entry))
        return false;
    if (cui_dialog_open_v1(ui, dialog->layout, trigger))
        return true;
    root->dialogs[--root->dialog_count] = (cui_root_surface){0};
    return false;
}

static void cui_root_reclaim_composed_dialogs(cui_root *root) {
    size_t remaining = 0;
    for (size_t i = 0; i < root->dialog_count; ++i) {
        cui_root_surface entry = root->dialogs[i];
        if (entry.content != cui_root_composed_dialog_marker || entry.surface->overlay.open)
            root->dialogs[remaining++] = entry;
    }
    while (root->dialog_count > remaining)
        root->dialogs[--root->dialog_count] = (cui_root_surface){0};
}

static bool cui_root_render_dialog(cui_context *ui, const cui_theme *theme, cui_rect viewport,
                                   int16_t layer, cui_root_surface entry) {
    if (entry.content == cui_root_composed_dialog_marker) {
        const cui_root_composed_dialog_v1 *dialog = entry.user;
        cui_surface_options options = dialog->options;
        options.viewport = viewport;
        options.layer = layer;
        if (!isfinite(options.margin) || options.margin < 0 || !dialog->layout ||
            dialog->layout->surface != entry.surface) {
            cui_report_diagnostic(ui, &(cui_diagnostic){.error = CUI_ERROR_INVALID_ACTION});
            return false;
        }
        options.margin = fminf(options.margin, fminf(viewport.w, viewport.h) * .49f);
        cui_dialog_sizing_v1 sizing;
        if (!cui_dialog_sizing_default_v1(options, &sizing))
            return false;
        if (dialog->refine_sizing)
            dialog->refine_sizing(viewport, &sizing, dialog->sizing_user);
        return cui_dialog_compose_v1(ui, theme, dialog->layout, options, &sizing,
                                     dialog->composition)
            .valid;
    }
    entry.options.viewport = viewport;
    entry.options.layer = layer;
    if (cui_surface_begin(ui, theme, entry.surface, entry.title, entry.description,
                          entry.options)) {
        entry.content(ui, entry.user);
        cui_surface_end(ui);
    }
    return true;
}

typedef struct {
    cui_root *root;
    const cui_theme *theme;
    cui_rect viewport;
    cui_id id;
    double now_seconds;
    bool responsive_notifications;
    bool notifications_submitted;
    bool dialogs_valid;
    cui_notification_advance notification_advance;
} cui_root_facility_frame;
static void cui_root_facilities(cui_context *ui, void *user) {
    cui_root_facility_frame *frame = user;
    cui_root *root = frame->root;
    if (root->sheet.surface && root->sheet.surface->overlay.open) {
        cui_root_surface sheet = root->sheet;
        sheet.options.viewport = frame->viewport;
        sheet.options.layer = 1;
        if (cui_surface_begin(ui, frame->theme, sheet.surface, sheet.title, sheet.description,
                              sheet.options)) {
            sheet.content(ui, sheet.user);
            cui_surface_end(ui);
        }
    }
    /* Take the snapshot before a facility callback can open another dialog.
     * Application and Sheet content ran first and can mount immediately. */
    size_t count = root->dialog_count;
    cui_root_surface dialogs[8];
    if (count)
        memcpy(dialogs, root->dialogs, count * sizeof dialogs[0]);
    for (size_t i = 0; i < count; i++) {
        cui_root_surface dialog = dialogs[i];
        if (dialog.surface && dialog.surface->overlay.open &&
            !cui_root_render_dialog(ui, frame->theme, frame->viewport, (int16_t)(3 + i * 2),
                                    dialog))
            frame->dialogs_valid = false;
    }
    cui_root_reclaim_composed_dialogs(root);
    cui_rect notification_viewport = frame->viewport;
    if (root->sheet.surface && root->sheet.surface->overlay.open) {
        cui_surface_options options = root->sheet.options;
        options.viewport = frame->viewport;
        cui_rect sheet = cui_surface_current_geometry(root->sheet.surface, options);
        switch (options.edge) {
        case CUI_SHEET_LEFT:
            notification_viewport.x += sheet.w;
            notification_viewport.w -= sheet.w;
            break;
        case CUI_SHEET_RIGHT:
            notification_viewport.w -= sheet.w;
            break;
        case CUI_SHEET_TOP:
            notification_viewport.y += sheet.h;
            notification_viewport.h -= sheet.h;
            break;
        case CUI_SHEET_BOTTOM:
            notification_viewport.h -= sheet.h;
            break;
        }
    }
    if (root->notifications) {
        cui_id scope = frame->id ^ UINT64_C(0x94fbd6a19873c20e);
        frame->notifications_submitted =
            frame->responsive_notifications
                ? cui_notifications_responsive(ui, scope, frame->theme, root->notifications,
                                               notification_viewport, frame->now_seconds,
                                               &frame->notification_advance)
                : cui_notifications_checked(ui, scope, frame->theme, root->notifications,
                                            notification_viewport, frame->now_seconds,
                                            &frame->notification_advance);
    }
}
bool cui_root_render_with_options(cui_context *ui, cui_id id, cui_root *root,
                                  const cui_theme *theme, cui_rect viewport,
                                  const cui_root_style_override *instance_override,
                                  cui_root_content_fn content, void *content_user,
                                  double now_seconds, cui_root_render_options options) {
    if (!theme || !cui_root_dialog_stack_valid(root))
        return false;
    cui_root_facility_frame frame = {.root = root,
                                     .theme = theme,
                                     .viewport = viewport,
                                     .id = id,
                                     .dialogs_valid = true,
                                     .now_seconds = now_seconds,
                                     .responsive_notifications = options.responsive_notifications};
    cui_root_style defaults = {.background = theme->background, .foreground = theme->text};
    bool rendered = cui_base_root_render(ui, id, root, viewport, defaults, instance_override,
                                         content, content_user, cui_root_facilities, &frame);
    if (rendered && frame.dialogs_valid && frame.notifications_submitted &&
        options.notification_advance)
        *options.notification_advance = frame.notification_advance;
    return rendered && frame.dialogs_valid;
}
bool cui_root_render(cui_context *ui, cui_id id, cui_root *root, const cui_theme *theme,
                     cui_rect viewport, const cui_root_style_override *instance_override,
                     cui_root_content_fn content, void *content_user, double now_seconds) {
    return cui_root_render_with_options(ui, id, root, theme, viewport, instance_override, content,
                                        content_user, now_seconds, (cui_root_render_options){0});
}
