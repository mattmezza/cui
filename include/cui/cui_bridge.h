/* SPDX-License-Identifier: MIT */
#ifndef CUI_BRIDGE_H
#define CUI_BRIDGE_H
#include <stdint.h>
#if defined(_WIN32)
#if defined(CUI_BRIDGE_BUILD)
#define CUI_BRIDGE_API __declspec(dllexport)
#else
#define CUI_BRIDGE_API __declspec(dllimport)
#endif
#else
#define CUI_BRIDGE_API
#endif
#ifdef __cplusplus
extern "C" {
#endif

#define CUI_BRIDGE_ABI_VERSION 1u
#define CUI_BRIDGE_MAX_DEPTH 128u

typedef struct cui_bridge_app cui_bridge_app;
typedef struct {
    uint32_t slot, generation;
} cui_bridge_node;
typedef struct {
    float x, y, width, height;
} cui_bridge_rect;
typedef enum {
    CUI_BRIDGE_OK = 0,
    CUI_BRIDGE_EMPTY,
    CUI_BRIDGE_CLOSED,
    CUI_BRIDGE_INVALID_ARGUMENT,
    CUI_BRIDGE_STALE_HANDLE,
    CUI_BRIDGE_CAPACITY,
    CUI_BRIDGE_OUT_OF_MEMORY,
    CUI_BRIDGE_INVALID_UTF8,
    CUI_BRIDGE_INVALID_STATE,
    CUI_BRIDGE_BACKEND_UNAVAILABLE,
    CUI_BRIDGE_CORE_ERROR
} cui_bridge_status;
typedef enum { CUI_BRIDGE_BACKEND_NONE = 0, CUI_BRIDGE_BACKEND_SDL = 1 } cui_bridge_backend;
typedef enum {
    CUI_BRIDGE_ROW = 1,
    CUI_BRIDGE_COLUMN,
    CUI_BRIDGE_LABEL,
    CUI_BRIDGE_BUTTON
} cui_bridge_kind;
typedef enum {
    CUI_BRIDGE_BUTTON_PRIMARY = 0,
    CUI_BRIDGE_BUTTON_SECONDARY,
    CUI_BRIDGE_BUTTON_OUTLINE,
    CUI_BRIDGE_BUTTON_GHOST
} cui_bridge_button_variant;
typedef enum {
    CUI_BRIDGE_SIZE_INHERIT = 0,
    CUI_BRIDGE_SIZE_XS,
    CUI_BRIDGE_SIZE_SM,
    CUI_BRIDGE_SIZE_BASE,
    CUI_BRIDGE_SIZE_MD,
    CUI_BRIDGE_SIZE_LG,
    CUI_BRIDGE_SIZE_XL,
    CUI_BRIDGE_SIZE_2XL,
    CUI_BRIDGE_SIZE_3XL,
    CUI_BRIDGE_SIZE_4XL,
    CUI_BRIDGE_SIZE_5XL,
    CUI_BRIDGE_SIZE_6XL,
    CUI_BRIDGE_SIZE_7XL,
    CUI_BRIDGE_SIZE_8XL,
    CUI_BRIDGE_SIZE_9XL
} cui_bridge_button_size;
typedef enum {
    CUI_BRIDGE_ROUND_THEME = 0,
    CUI_BRIDGE_ROUND_NONE,
    CUI_BRIDGE_ROUND_SMALL,
    CUI_BRIDGE_ROUND_LARGE,
    CUI_BRIDGE_ROUND_CUSTOM
} cui_bridge_button_rounding;
typedef enum {
    CUI_BRIDGE_POINTER_MOVE = 0,
    CUI_BRIDGE_POINTER_DOWN,
    CUI_BRIDGE_POINTER_UP
} cui_bridge_pointer_phase;
typedef enum { CUI_BRIDGE_KEY_DOWN = 0, CUI_BRIDGE_KEY_UP = 1 } cui_bridge_key_phase;
typedef enum {
    CUI_BRIDGE_KEY_TAB = 1,
    CUI_BRIDGE_KEY_ENTER,
    CUI_BRIDGE_KEY_SPACE,
    CUI_BRIDGE_KEY_ESCAPE,
    CUI_BRIDGE_KEY_LEFT,
    CUI_BRIDGE_KEY_RIGHT,
    CUI_BRIDGE_KEY_UP_ARROW,
    CUI_BRIDGE_KEY_DOWN_ARROW
} cui_bridge_key;
enum {
    CUI_BRIDGE_MOD_SHIFT = 1u,
    CUI_BRIDGE_MOD_CTRL = 2u,
    CUI_BRIDGE_MOD_ALT = 4u,
    CUI_BRIDGE_MOD_META = 8u
};
typedef enum { CUI_BRIDGE_EVENT_CLICK = 1 } cui_bridge_event_type;
typedef struct {
    cui_bridge_event_type type;
    cui_bridge_node node;
    uint64_t frame_sequence;
} cui_bridge_event;
typedef struct {
    uint32_t struct_size; /* Must equal cui_bridge_options_size(). */
    uint32_t abi_version;
    cui_bridge_backend backend;
    uint32_t max_nodes, max_events;
    float width, height;        /* Logical pixels; positive, finite, <= 32768. */
    const char *title_utf8;     /* Copied during create; optional for headless. */
    const char *font_path_utf8; /* Required for SDL; borrowed only during create. */
} cui_bridge_options;

/* All calls for an app must be confined to one thread. Strings are UTF-8,
 * NUL-terminated, and limited to 4096 bytes excluding NUL. */
CUI_BRIDGE_API uint32_t cui_bridge_abi_version(void);
CUI_BRIDGE_API uint32_t cui_bridge_options_size(void);
CUI_BRIDGE_API const char *cui_bridge_status_message(cui_bridge_status status);
CUI_BRIDGE_API cui_bridge_app *cui_bridge_create(const cui_bridge_options *options,
                                                 cui_bridge_status *out_status);
CUI_BRIDGE_API void cui_bridge_destroy(cui_bridge_app *app);
/* Zero parent creates the single root. Keys are copied, unique among live nodes;
 * stale handles never alias a replacement node within an app. Handle provenance
 * is app-scoped: callers must use each handle only with its creating app.
 * Mutations are rejected during step. Trees are limited to 128 levels including
 * the root; deeper creation returns CAPACITY without changing the tree. */
CUI_BRIDGE_API cui_bridge_status cui_bridge_node_create(cui_bridge_app *app, cui_bridge_node parent,
                                                        cui_bridge_kind kind, const char *key_utf8,
                                                        cui_bridge_node *out_node);
CUI_BRIDGE_API cui_bridge_status cui_bridge_node_remove(cui_bridge_app *app, cui_bridge_node node);
CUI_BRIDGE_API cui_bridge_status cui_bridge_set_text(cui_bridge_app *app, cui_bridge_node node,
                                                     const char *text_utf8);
CUI_BRIDGE_API cui_bridge_status cui_bridge_set_gap(cui_bridge_app *app, cui_bridge_node node,
                                                    float gap);
CUI_BRIDGE_API cui_bridge_status cui_bridge_set_button_variant(cui_bridge_app *app,
                                                               cui_bridge_node node,
                                                               cui_bridge_button_variant variant);
CUI_BRIDGE_API cui_bridge_status cui_bridge_set_button_size(cui_bridge_app *app,
                                                            cui_bridge_node node,
                                                            cui_bridge_button_size size);
CUI_BRIDGE_API cui_bridge_status cui_bridge_set_button_rounding(cui_bridge_app *app,
                                                                cui_bridge_node node,
                                                                cui_bridge_button_rounding rounding,
                                                                float custom_radius);
CUI_BRIDGE_API cui_bridge_status cui_bridge_set_button_disabled(cui_bridge_app *app,
                                                                cui_bridge_node node,
                                                                uint32_t disabled);
CUI_BRIDGE_API cui_bridge_status cui_bridge_set_button_hover_rgba(cui_bridge_app *app,
                                                                  cui_bridge_node node, uint8_t r,
                                                                  uint8_t g, uint8_t b, uint8_t a);
/* Host events are queued before step. SDL step also polls its own native queue.
 * Clicks are copied into a bounded queue; language callbacks run after step. */
CUI_BRIDGE_API cui_bridge_status cui_bridge_queue_pointer(cui_bridge_app *app,
                                                          cui_bridge_pointer_phase phase, float x,
                                                          float y);
CUI_BRIDGE_API cui_bridge_status cui_bridge_queue_key(cui_bridge_app *app,
                                                      cui_bridge_key_phase phase,
                                                      cui_bridge_key key, uint32_t modifiers,
                                                      uint32_t repeat);
CUI_BRIDGE_API cui_bridge_status cui_bridge_step(cui_bridge_app *app, float dt);
CUI_BRIDGE_API cui_bridge_status cui_bridge_poll_event(cui_bridge_app *app,
                                                       cui_bridge_event *out_event);
CUI_BRIDGE_API cui_bridge_status cui_bridge_node_bounds(cui_bridge_app *app, cui_bridge_node node,
                                                        cui_bridge_rect *out_bounds);
CUI_BRIDGE_API cui_bridge_status cui_bridge_request_close(cui_bridge_app *app);

#ifdef __cplusplus
}
#endif
#endif
