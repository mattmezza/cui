/* SPDX-License-Identifier: MIT */
#ifndef CUI_ROOT_H
#define CUI_ROOT_H
#include "cui_dialog_composition.h"
#include "cui_status.h"
#include "cui_surface.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct cui_root cui_root;
typedef struct {
    cui_color background, foreground;
    float padding;
} cui_root_style;
typedef struct {
    cui_root_style value;
    bool background, foreground, padding; /* Refine only named fields. */
} cui_root_style_override;
typedef void (*cui_root_content_fn)(cui_context *, void *);
/* Host retrieves its selected text and writes to its clipboard explicitly. */
typedef bool (*cui_root_copy_fn)(cui_context *, void *);
typedef void *(*cui_root_plugin_build_fn)(void *registration_user, void *window_user);
typedef struct {
    uint64_t key; /* Application-owned stable type key. */
    cui_root_plugin_build_fn build;
    void (*destroy)(void *registration_user, void *state);
    void (*prepare)(cui_context *, void *state);
    void (*style)(cui_context *, void *state, cui_root_style *);
    void (*decorate_begin)(cui_context *, void *state, cui_root_style);
    void (*decorate_end)(cui_context *, void *state);
    void (*overlay)(cui_context *, void *state, cui_rect viewport);
    void *registration_user;
} cui_root_plugin;
typedef struct {
    cui_root_plugin plugin;
    void *state;
} cui_root_plugin_instance;
typedef struct {
    cui_root_plugin *entries;
    size_t count, capacity;
} cui_root_registry;
/* Replacement keeps its original order and affects only later root snapshots. */
bool cui_root_registry_init(cui_root_registry *, cui_root_plugin *, size_t capacity);
bool cui_root_register(cui_root_registry *, cui_root_plugin);
typedef struct {
    cui_surface_state *surface;
    cui_str title, description;
    cui_surface_options options;
    cui_root_content_fn content;
    void *user;
} cui_root_surface;
struct cui_root {
    cui_root_plugin_instance *plugins;
    size_t plugin_count, plugin_capacity;
    cui_root_surface *dialogs;
    size_t dialog_count, dialog_capacity;
    cui_root_surface sheet;
    cui_notification_state *notifications;
    cui_root_copy_fn copy_selection;
    void *copy_user;
    bool initialized;
};
/* Zero-initialize the root before its first init; dispose before reinitializing.
 * All arrays and plugin states are caller-owned and must outlive this root.
 * Invalid capacity is rejected before any factory is called or state is changed. */
bool cui_root_init(cui_root *, cui_root_plugin_instance *, size_t capacity,
                   const cui_root_registry *, void *window_user);
/* Calls optional plugin cleanup in reverse order; caller owns plugin memory. */
void cui_root_dispose(cui_root *);
void *cui_root_plugin_state(const cui_root *, uint64_t key);
bool cui_root_bind_facilities(cui_root *, cui_root_surface *dialogs, size_t dialog_capacity,
                              cui_notification_state *notifications);
bool cui_root_dialog_open(cui_root *, cui_context *, cui_root_surface, cui_id trigger);
typedef void (*cui_root_dialog_sizing_fn)(cui_rect viewport, cui_dialog_sizing_v1 *, void *);
typedef struct {
    cui_dialog_layout_v1 *layout;
    cui_surface_options options;
    const cui_dialog_composition_v1 *composition;
    cui_root_dialog_sizing_fn refine_sizing;
    void *sizing_user;
} cui_root_composed_dialog_v1;
/* Mount a closed composed Dialog or AlertDialog in Styled Root's existing stack.
 * Root supplies the current viewport and layer. The optional sizing callback
 * refines fresh defaults once per submission. Root borrows this descriptor,
 * the layout, and its referenced data. Keep them live until a Root frame
 * observes the close and ends, or dispose Root after the last frame. New
 * dialogs opened during application or Sheet content can mount that frame.
 * Dialogs opened from another dialog's slots mount on the next Root frame. */
bool cui_root_dialog_open_composed_v1(cui_root *, cui_context *, cui_root_composed_dialog_v1 *,
                                      cui_id trigger);
/* Request close of the top open Dialog or AlertDialog in the shared stack.
 * Its next submission runs the usual decision and focus policy. */
bool cui_root_dialog_close_top(cui_root *, cui_context *, cui_surface_result);
bool cui_root_sheet_open(cui_root *, cui_context *, cui_root_surface, cui_id trigger);
bool cui_root_notification_push(cui_root *, cui_notification, double now_seconds);
bool cui_root_set_copy_hook(cui_root *, cui_root_copy_fn, void *user);
/* Base composition. Call during a frame; content and overlays each run once.
 * prepare -> style -> instance override -> decorators -> content -> overlays. */
bool cui_base_root_render(cui_context *, cui_id, cui_root *, cui_rect viewport,
                          cui_root_style defaults, const cui_root_style_override *instance_override,
                          cui_root_content_fn content, void *content_user,
                          cui_root_content_fn facilities, void *facilities_user);
/* Styled composition mounts bound Sheet, Dialog and Notification facilities
 * above application content without requiring the content callback to mention them. */
bool cui_root_render(cui_context *, cui_id, cui_root *, const cui_theme *, cui_rect viewport,
                     const cui_root_style_override *instance_override, cui_root_content_fn content,
                     void *content_user, double now_seconds);
typedef struct {
    bool responsive_notifications;
    cui_notification_advance *notification_advance;
} cui_root_render_options;
/* Same facilities, geometry and ordering as cui_root_render. Enable responsive_notifications
 * for wrapped cards and an independently reachable Close button. Supply optional,
 * independent caller-owned notification_advance storage for this call. It is
 * written only if Root rendered and its bound notification submission succeeded.
 * Use it to schedule a wake in the same clock as now_seconds. */
bool cui_root_render_with_options(cui_context *, cui_id, cui_root *, const cui_theme *,
                                  cui_rect viewport,
                                  const cui_root_style_override *instance_override,
                                  cui_root_content_fn content, void *content_user,
                                  double now_seconds, cui_root_render_options);
#ifdef __cplusplus
}
#endif
#endif
