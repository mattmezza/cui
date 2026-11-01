/* SPDX-License-Identifier: MIT */
/* Private same-TU bridge module: reuse bridge_ready/node/create/string policy. */
#ifndef CUI_BRIDGE_SLIDER_INTERNAL_H
#define CUI_BRIDGE_SLIDER_INTERNAL_H
#include "cui/cui_styled_slider_model.h"
#include <stdatomic.h>
#if defined(CUI_BRIDGE_SLIDER_NO_TLS)
#define BRIDGE_SLIDER_SUPPORTED 0
#elif defined(__wasm__)
#if defined(CUI_BRIDGE_SLIDER_WASM_UNSHARED) && CUI_BRIDGE_SLIDER_WASM_UNSHARED == 1 &&            \
    !defined(__wasm_atomics__) && !defined(_REENTRANT)
#define BRIDGE_SLIDER_SUPPORTED 1
#else
#define BRIDGE_SLIDER_SUPPORTED 0
#endif
#else
#define BRIDGE_SLIDER_SUPPORTED 1
#endif
_Static_assert(sizeof(cui_bridge_slider_value_v1) == 24, "Slider value ABI");
_Static_assert(sizeof(cui_bridge_slider_domain_v1) == 32, "Slider domain ABI");
_Static_assert(sizeof(cui_bridge_slider_config_v1) == 72, "Slider config ABI");
_Static_assert(sizeof(cui_bridge_slider_receipt_v1) == 56, "Slider receipt ABI");
_Static_assert(sizeof(cui_bridge_slider_input_v1) == 72, "Slider input ABI");
_Static_assert(sizeof(cui_bridge_slider_request_v1) == 296, "Slider request ABI");
_Static_assert(sizeof(cui_bridge_slider_presentation_v1) == 32, "Slider presentation ABI");
_Static_assert(sizeof(cui_bridge_slider_queue_options_v1) == 16, "Slider queue ABI");
_Static_assert(offsetof(cui_bridge_slider_request_v1, input) == 224, "Slider input offset");
_Static_assert(offsetof(cui_bridge_slider_request_v1, receipt) == 32, "Slider receipt offset");
_Static_assert(offsetof(cui_bridge_slider_receipt_v1, request_sequence) == 48,
               "Slider sequence offset");
typedef struct {
    cui_slider_model_state_v1 model;
    cui_styled_slider_model_state_v1 presentation;
    cui_bridge_slider_config_v1 config;
    cui_bridge_slider_value_v1 value;
    cui_bridge_slider_presentation_v1 observation;
    uint64_t revision, valid_frame, first, last, accepted, seen_frame;
    uint32_t generation, slot;
    bool window, observed, layout_seen, emitted_error;
    cui_bridge_app *app;
} bridge_slider_owned;
struct bridge_slider_app {
    const void *owner;
    uint64_t epoch, frame;
    bridge_slider_owned **nodes;
    cui_bridge_slider_request_v1 *queue;
    uint32_t capacity, head, count;
    bool failed_frame;
};
#if BRIDGE_SLIDER_SUPPORTED
static _Thread_local unsigned char bridge_slider_thread;
static atomic_flag bridge_slider_epoch_gate = ATOMIC_FLAG_INIT;
static uint64_t bridge_slider_next_epoch = 1;
static bool bridge_slider_epoch_exhausted;
#endif
static const void *bridge_slider_owner(void) {
#if BRIDGE_SLIDER_SUPPORTED
    return &bridge_slider_thread;
#else
    return NULL; /* Disabled API stubs never use this as an owner identity. */
#endif
}
static bool bridge_slider_epoch(uint64_t *out) {
#if BRIDGE_SLIDER_SUPPORTED
    while (atomic_flag_test_and_set_explicit(&bridge_slider_epoch_gate, memory_order_acquire)) {
    }
    bool ok = !bridge_slider_epoch_exhausted;
    uint64_t value = bridge_slider_next_epoch;
    if (ok) {
        if (value == UINT64_MAX)
            bridge_slider_epoch_exhausted = true;
        else
            bridge_slider_next_epoch = value + 1;
    }
    atomic_flag_clear_explicit(&bridge_slider_epoch_gate, memory_order_release);
    if (ok)
        *out = value;
    return ok;
#else
    (void)out;
    return false;
#endif
}
static cui_bridge_status bridge_slider_thread_check(cui_bridge_app *app) {
    return app->slider && app->slider->owner != bridge_slider_owner() ? CUI_BRIDGE_INVALID_STATE
                                                                      : CUI_BRIDGE_OK;
}
static cui_bridge_status bridge_slider_ready(cui_bridge_app *app) {
    cui_bridge_status status = bridge_ready(app);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!BRIDGE_SLIDER_SUPPORTED)
        return CUI_BRIDGE_BACKEND_UNAVAILABLE;
    if (cui_frame_active(app->ui))
        return CUI_BRIDGE_INVALID_STATE;
    return CUI_BRIDGE_OK;
}
static cui_slider_model_domain_v1 bridge_slider_domain(cui_bridge_slider_domain_v1 d) {
    uint32_t scale = d.scale == CUI_BRIDGE_SLIDER_LINEAR        ? CUI_SLIDER_MODEL_LINEAR
                     : d.scale == CUI_BRIDGE_SLIDER_LOGARITHMIC ? CUI_SLIDER_MODEL_LOGARITHMIC
                                                                : UINT32_MAX;
    return (cui_slider_model_domain_v1){d.minimum, d.maximum, d.step, scale, d.reserved};
}
static cui_slider_model_value_v1 bridge_slider_value(cui_bridge_slider_value_v1 v) {
    uint32_t kind = v.kind == CUI_BRIDGE_SLIDER_SINGLE  ? CUI_SLIDER_MODEL_SINGLE
                    : v.kind == CUI_BRIDGE_SLIDER_RANGE ? CUI_SLIDER_MODEL_RANGE
                                                        : UINT32_MAX;
    return (cui_slider_model_value_v1){kind, v.reserved, v.start, v.end};
}
static cui_bridge_slider_value_v1 bridge_slider_copy_value(cui_slider_model_value_v1 v) {
    return (cui_bridge_slider_value_v1){v.kind == CUI_SLIDER_MODEL_SINGLE ? CUI_BRIDGE_SLIDER_SINGLE
                                                                          : CUI_BRIDGE_SLIDER_RANGE,
                                        0, v.kind == CUI_SLIDER_MODEL_SINGLE ? 0 : v.start, v.end};
}
static cui_bridge_slider_domain_v1 bridge_slider_copy_domain(cui_slider_model_domain_v1 d) {
    return (cui_bridge_slider_domain_v1){d.minimum, d.maximum, d.step,
                                         d.scale == CUI_SLIDER_MODEL_LINEAR
                                             ? CUI_BRIDGE_SLIDER_LINEAR
                                             : CUI_BRIDGE_SLIDER_LOGARITHMIC,
                                         0};
}
static bool bridge_slider_validate(uint32_t cs, const cui_bridge_slider_config_v1 *config,
                                   uint32_t vs, const cui_bridge_slider_value_v1 *value) {
    if (cs != sizeof *config || vs != sizeof *value || !config || !value ||
        config->struct_size != cs || config->schema_version != 1 || config->reserved ||
        (config->flags & ~31u) || config->orientation > 1 || config->layout > 1 ||
        !isfinite(config->rem) || config->rem <= 0 || !isfinite(1.5f * config->rem) ||
        (config->layout == CUI_BRIDGE_SLIDER_FIXED &&
         (!isfinite(config->length) || config->length <= 0)))
        return false;
    return cui_slider_model_domain_valid_v1(bridge_slider_domain(config->domain)) &&
           cui_slider_model_value_valid_v1(bridge_slider_domain(config->domain),
                                           bridge_slider_value(*value));
}
static cui_bridge_status bridge_slider_get(cui_bridge_app *app, cui_bridge_node handle,
                                           bridge_slider_owned **out) {
    cui_bridge_status status = bridge_slider_ready(app);
    if (status != CUI_BRIDGE_OK)
        return status;
    cui_bridge_node_record *node = bridge_node(app, handle);
    if (!node)
        return CUI_BRIDGE_STALE_HANDLE;
    if (node->kind != CUI_BRIDGE_SLIDER_MODEL || !app->slider || !app->slider->nodes[handle.slot])
        return CUI_BRIDGE_INVALID_STATE;
    *out = app->slider->nodes[handle.slot];
    return CUI_BRIDGE_OK;
}
static cui_bridge_status bridge_slider_revise(bridge_slider_owned *owned) {
    if (owned->revision == UINT64_MAX)
        return CUI_BRIDGE_CAPACITY;
    owned->revision++;
    owned->window = false;
    cui_invalidate(owned->app->ui);
    return CUI_BRIDGE_OK;
}
uint32_t cui_bridge_slider_model_versions(void) {
    return BRIDGE_SLIDER_SUPPORTED ? 1u : 0u;
}
uint32_t cui_bridge_slider_config_v1_size(void) {
    return sizeof(cui_bridge_slider_config_v1);
}
uint32_t cui_bridge_slider_value_v1_size(void) {
    return sizeof(cui_bridge_slider_value_v1);
}
uint32_t cui_bridge_slider_receipt_v1_size(void) {
    return sizeof(cui_bridge_slider_receipt_v1);
}
uint32_t cui_bridge_slider_request_v1_size(void) {
    return sizeof(cui_bridge_slider_request_v1);
}
uint32_t cui_bridge_slider_presentation_v1_size(void) {
    return sizeof(cui_bridge_slider_presentation_v1);
}
cui_bridge_status
cui_bridge_slider_queue_enable_v1(cui_bridge_app *app, uint32_t size,
                                  const cui_bridge_slider_queue_options_v1 *options) {
    cui_bridge_status status = bridge_slider_ready(app);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!options || size != sizeof *options || options->struct_size != size ||
        options->schema_version != 1 || options->reserved || options->capacity > 4096)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    uint32_t capacity = options->capacity ? options->capacity : 4096;
    if (app->slider)
        return app->slider->capacity == capacity ? CUI_BRIDGE_OK : CUI_BRIDGE_INVALID_STATE;
    size_t pointer_bytes = ((size_t)app->max_nodes + 1) * sizeof(bridge_slider_owned *);
    size_t offset = sizeof(bridge_slider_app) + pointer_bytes;
    size_t alignment = _Alignof(cui_bridge_slider_request_v1);
    offset = (offset + alignment - 1) / alignment * alignment;
    if (capacity > (SIZE_MAX - offset) / sizeof(cui_bridge_slider_request_v1))
        return CUI_BRIDGE_CAPACITY;
    bridge_slider_app *state = calloc(1, offset + capacity * sizeof(cui_bridge_slider_request_v1));
    if (!state)
        return CUI_BRIDGE_OUT_OF_MEMORY;
    if (!bridge_slider_epoch(&state->epoch)) {
        free(state);
        return CUI_BRIDGE_CAPACITY;
    }
    state->owner = bridge_slider_owner();
    state->nodes = (bridge_slider_owned **)(state + 1);
    state->queue = (cui_bridge_slider_request_v1 *)((unsigned char *)state + offset);
    state->capacity = capacity;
    app->slider = state;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_slider_model_create_v1(cui_bridge_app *app, cui_bridge_node parent,
                                                    const char *key, uint32_t cs,
                                                    const cui_bridge_slider_config_v1 *config,
                                                    uint32_t vs,
                                                    const cui_bridge_slider_value_v1 *value,
                                                    cui_bridge_node *out) {
    cui_bridge_status status = bridge_slider_ready(app);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!app->slider)
        return CUI_BRIDGE_INVALID_STATE;
    cui_bridge_node_record *p = bridge_node(app, parent);
    if (!p)
        return CUI_BRIDGE_STALE_HANDLE;
    for (uint32_t ancestor = parent.slot; ancestor; ancestor = app->nodes[ancestor].parent)
        if (app->nodes[ancestor].kind != CUI_BRIDGE_ROW &&
            app->nodes[ancestor].kind != CUI_BRIDGE_COLUMN)
            return CUI_BRIDGE_INVALID_STATE;
    if (!out || !bridge_slider_validate(cs, config, vs, value))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_bridge_slider_config_v1 c = *config;
    cui_bridge_slider_value_v1 v = bridge_slider_copy_value(bridge_slider_value(*value));
    if (c.layout == CUI_BRIDGE_SLIDER_PARENT)
        c.length = 0;
    bridge_slider_owned *owned = calloc(1, sizeof *owned);
    if (!owned)
        return CUI_BRIDGE_OUT_OF_MEMORY;
    cui_bridge_node handle;
    /* Reuse existing copied-key/ancestry/generation allocation with no callbacks. */
    status = cui_bridge_node_create(app, parent, CUI_BRIDGE_LABEL, key, &handle);
    if (status != CUI_BRIDGE_OK) {
        free(owned);
        return status;
    }
    owned->config = c;
    owned->value = v;
    owned->revision = 1;
    owned->app = app;
    owned->slot = handle.slot;
    owned->generation = handle.generation;
    cui_slider_model_state_init_v1(sizeof owned->model, &owned->model);
    cui_styled_slider_model_state_init_v1(sizeof owned->presentation, &owned->presentation);
    app->slider->nodes[handle.slot] = owned;
    app->nodes[handle.slot].kind = CUI_BRIDGE_SLIDER_MODEL;
    *out = handle;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_slider_model_configure_v1(cui_bridge_app *app, cui_bridge_node handle,
                                                       uint32_t cs,
                                                       const cui_bridge_slider_config_v1 *config,
                                                       uint32_t vs,
                                                       const cui_bridge_slider_value_v1 *value) {
    bridge_slider_owned *owned;
    cui_bridge_status status = bridge_slider_get(app, handle, &owned);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!bridge_slider_validate(cs, config, vs, value))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_bridge_slider_config_v1 c = *config;
    cui_bridge_slider_value_v1 v = bridge_slider_copy_value(bridge_slider_value(*value));
    if (c.layout == CUI_BRIDGE_SLIDER_PARENT)
        c.length = 0;
    status = bridge_slider_revise(owned);
    if (status != CUI_BRIDGE_OK)
        return status;
    owned->config = c;
    owned->value = v;
    cui_styled_slider_model_state_init_v1(sizeof owned->presentation, &owned->presentation);
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_slider_model_get_config_v1(cui_bridge_app *app, cui_bridge_node handle,
                                                        uint32_t size,
                                                        cui_bridge_slider_config_v1 *out) {
    bridge_slider_owned *owned;
    cui_bridge_status status = bridge_slider_get(app, handle, &owned);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (size != sizeof *out || !out)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    *out = owned->config;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_slider_model_get_value_v1(cui_bridge_app *app, cui_bridge_node handle,
                                                       uint32_t size,
                                                       cui_bridge_slider_value_v1 *out) {
    bridge_slider_owned *owned;
    cui_bridge_status status = bridge_slider_get(app, handle, &owned);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (size != sizeof *out || !out)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    *out = owned->value;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_slider_model_set_value_v1(cui_bridge_app *app, cui_bridge_node handle,
                                                       uint32_t size,
                                                       const cui_bridge_slider_value_v1 *value) {
    bridge_slider_owned *owned;
    cui_bridge_status status = bridge_slider_get(app, handle, &owned);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (size != sizeof *value || !value ||
        !cui_slider_model_value_valid_v1(bridge_slider_domain(owned->config.domain),
                                         bridge_slider_value(*value)) ||
        value->kind != owned->value.kind)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_bridge_slider_value_v1 copy = bridge_slider_copy_value(bridge_slider_value(*value));
    status = bridge_slider_revise(owned);
    if (status != CUI_BRIDGE_OK)
        return status;
    owned->value = copy;
    return CUI_BRIDGE_OK;
}
static cui_bridge_status bridge_slider_text(cui_bridge_app *app, uint32_t slot, const char *text) {
    bridge_slider_owned *owned = app->slider->nodes[slot];
    if (owned->revision == UINT64_MAX)
        return CUI_BRIDGE_CAPACITY;
    char *copy = NULL;
    cui_bridge_status status = bridge_copy(text, &copy);
    if (status != CUI_BRIDGE_OK)
        return status;
    status = bridge_slider_revise(owned);
    if (status != CUI_BRIDGE_OK) {
        free(copy);
        return status;
    }
    free(app->nodes[slot].text);
    app->nodes[slot].text = copy;
    return CUI_BRIDGE_OK;
}
static cui_bridge_status bridge_slider_disabled(cui_bridge_app *app, uint32_t slot,
                                                uint32_t disabled) {
    if (disabled > 1)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    if (app->nodes[slot].disabled == (disabled != 0))
        return CUI_BRIDGE_OK;
    cui_bridge_status status = bridge_slider_revise(app->slider->nodes[slot]);
    if (status == CUI_BRIDGE_OK)
        app->nodes[slot].disabled = disabled != 0;
    return status;
}
static void bridge_slider_remove(cui_bridge_app *app, uint32_t slot) {
    if (app && app->slider && slot && slot <= app->max_nodes) {
        free(app->slider->nodes[slot]);
        app->slider->nodes[slot] = NULL;
    }
}
static bool bridge_slider_map_request(bridge_slider_owned *owned,
                                      const cui_slider_model_request_v1 *request,
                                      cui_bridge_slider_request_v1 *out) {
    if ((request->current.kind != CUI_SLIDER_MODEL_SINGLE &&
         request->current.kind != CUI_SLIDER_MODEL_RANGE) ||
        (request->requested.kind != CUI_SLIDER_MODEL_SINGLE &&
         request->requested.kind != CUI_SLIDER_MODEL_RANGE) ||
        (request->domain.scale != CUI_SLIDER_MODEL_LINEAR &&
         request->domain.scale != CUI_SLIDER_MODEL_LOGARITHMIC) ||
        (request->gesture_domain.scale != CUI_SLIDER_MODEL_LINEAR &&
         request->gesture_domain.scale != CUI_SLIDER_MODEL_LOGARITHMIC))
        return false;
    cui_bridge_slider_request_v1 r;
    memset(&r, 0, sizeof r);
    r.struct_size = sizeof r;
    r.schema_version = 1;
    switch (request->phase) {
    case CUI_SLIDER_MODEL_BEGIN:
        r.phase = CUI_BRIDGE_SLIDER_BEGIN;
        break;
    case CUI_SLIDER_MODEL_UPDATE:
        r.phase = CUI_BRIDGE_SLIDER_UPDATE;
        break;
    case CUI_SLIDER_MODEL_COMMIT:
        r.phase = CUI_BRIDGE_SLIDER_COMMIT;
        break;
    case CUI_SLIDER_MODEL_CANCEL:
        r.phase = CUI_BRIDGE_SLIDER_CANCEL;
        break;
    default:
        return false;
    }
#define SLIDER_REASON(name)                                                                        \
    case CUI_SLIDER_MODEL_##name:                                                                  \
        r.reason = CUI_BRIDGE_SLIDER_REASON_##name;                                                \
        break
    switch (request->reason) {
        SLIDER_REASON(INPUT);
        SLIDER_REASON(DISABLED);
        SLIDER_REASON(READONLY);
        SLIDER_REASON(ZERO_SPAN);
        SLIDER_REASON(SOURCE_REPLACED);
        SLIDER_REASON(DOMAIN_CHANGED);
        SLIDER_REASON(MODE_CHANGED);
        SLIDER_REASON(AXIS_CHANGED);
        SLIDER_REASON(OMITTED);
        SLIDER_REASON(CAPTURE_LOST);
    default:
        return false;
    }
#undef SLIDER_REASON
    if (request->thumb != CUI_SLIDER_MODEL_START && request->thumb != CUI_SLIDER_MODEL_END)
        return false;
    r.thumb = request->thumb == CUI_SLIDER_MODEL_START ? 0u : 1u;
    switch (request->event.kind) {
    case CUI_COMPONENT_EVENT_REQUEST:
        r.event_kind = CUI_BRIDGE_SLIDER_EVENT_REQUEST;
        break;
    case CUI_COMPONENT_EVENT_COMMIT:
        r.event_kind = CUI_BRIDGE_SLIDER_EVENT_COMMIT;
        break;
    case CUI_COMPONENT_EVENT_LIFECYCLE:
        r.event_kind = CUI_BRIDGE_SLIDER_EVENT_LIFECYCLE;
        break;
    default:
        return false;
    }
    switch (request->event.source) {
    case CUI_COMPONENT_EVENT_SOURCE_UNKNOWN:
        r.event_source = CUI_BRIDGE_SLIDER_SOURCE_UNKNOWN;
        break;
    case CUI_COMPONENT_EVENT_SOURCE_PROGRAMMATIC:
        r.event_source = CUI_BRIDGE_SLIDER_SOURCE_PROGRAMMATIC;
        break;
    case CUI_COMPONENT_EVENT_SOURCE_POINTER:
        r.event_source = CUI_BRIDGE_SLIDER_SOURCE_POINTER;
        break;
    case CUI_COMPONENT_EVENT_SOURCE_KEYBOARD:
        r.event_source = CUI_BRIDGE_SLIDER_SOURCE_KEYBOARD;
        break;
    case CUI_COMPONENT_EVENT_SOURCE_ACCESSIBILITY:
        r.event_source = CUI_BRIDGE_SLIDER_SOURCE_ACCESSIBILITY;
        break;
    default:
        return false;
    }
    if (request->event.flags & ~(CUI_COMPONENT_EVENT_HAS_FRAME | CUI_COMPONENT_EVENT_HAS_INDEX |
                                 CUI_COMPONENT_EVENT_HAS_SOURCE))
        return false;
    if (request->event.flags & CUI_COMPONENT_EVENT_HAS_FRAME) {
        r.event_flags |= CUI_BRIDGE_SLIDER_HAS_FRAME;
        r.event_frame = request->event.frame_sequence;
    }
    r.event_index = UINT64_MAX;
    if (request->event.flags & CUI_COMPONENT_EVENT_HAS_INDEX) {
        r.event_flags |= CUI_BRIDGE_SLIDER_HAS_INDEX;
        r.event_index = (uint64_t)request->event.event_index;
    }
    if (request->event.flags & CUI_COMPONENT_EVENT_HAS_SOURCE)
        r.event_flags |= CUI_BRIDGE_SLIDER_HAS_SOURCE;
    r.event_target = request->event.target;
    r.receipt = (cui_bridge_slider_receipt_v1){.struct_size = sizeof r.receipt,
                                               .schema_version = 1,
                                               .app_epoch = owned->app->slider->epoch,
                                               .node = {owned->slot, owned->generation},
                                               .source_generation = request->source_generation,
                                               .configuration_revision = owned->revision,
                                               .submission_frame = request->submission_frame,
                                               .request_sequence = request->request_sequence};
    r.current = bridge_slider_copy_value(request->current);
    r.requested = bridge_slider_copy_value(request->requested);
    r.domain = bridge_slider_copy_domain(request->domain);
    r.gesture_domain = bridge_slider_copy_domain(request->gesture_domain);
    if (request->input.flags & ~(CUI_SLIDER_MODEL_HAS_INPUT | CUI_SLIDER_MODEL_HAS_NUMERIC_VALUE))
        return false;
    if ((request->input.flags & CUI_SLIDER_MODEL_HAS_NUMERIC_VALUE) &&
        (!(request->input.flags & CUI_SLIDER_MODEL_HAS_INPUT) ||
         request->input.type != CUI_EVENT_ACCESSIBILITY_ACTION ||
         request->input.accessibility_action != CUI_ACCESSIBILITY_SET_VALUE ||
         !isfinite(request->input.accessibility_value)))
        return false;
    if (request->input.flags & CUI_SLIDER_MODEL_HAS_INPUT) {
        const cui_slider_model_input_v1 *i = &request->input;
        r.input.flags = CUI_BRIDGE_SLIDER_HAS_INPUT;
        switch (i->type) {
        case CUI_EVENT_POINTER_MOVE:
            r.input.type = CUI_BRIDGE_SLIDER_INPUT_MOVE;
            break;
        case CUI_EVENT_POINTER_DOWN:
            r.input.type = CUI_BRIDGE_SLIDER_INPUT_DOWN;
            break;
        case CUI_EVENT_POINTER_UP:
            r.input.type = CUI_BRIDGE_SLIDER_INPUT_UP;
            break;
        case CUI_EVENT_KEY_DOWN:
            r.input.type = CUI_BRIDGE_SLIDER_INPUT_KEY;
            break;
        case CUI_EVENT_CANCEL:
            r.input.type = CUI_BRIDGE_SLIDER_INPUT_CANCEL;
            break;
        case CUI_EVENT_ACCESSIBILITY_ACTION:
            r.input.type = CUI_BRIDGE_SLIDER_INPUT_ACCESSIBILITY;
            break;
        default:
            return false;
        }
        if (i->modifiers & ~(CUI_MOD_SHIFT | CUI_MOD_CTRL | CUI_MOD_ALT | CUI_MOD_META))
            return false;
        if (i->modifiers & CUI_MOD_SHIFT)
            r.input.modifiers |= CUI_BRIDGE_MOD_SHIFT;
        if (i->modifiers & CUI_MOD_CTRL)
            r.input.modifiers |= CUI_BRIDGE_MOD_CTRL;
        if (i->modifiers & CUI_MOD_ALT)
            r.input.modifiers |= CUI_BRIDGE_MOD_ALT;
        if (i->modifiers & CUI_MOD_META)
            r.input.modifiers |= CUI_BRIDGE_MOD_META;
        if (i->type == CUI_EVENT_KEY_DOWN) {
#define SLIDER_KEY(name)                                                                           \
    case CUI_KEY_##name:                                                                           \
        r.input.key = CUI_BRIDGE_SLIDER_KEY_##name;                                                \
        break
            switch (i->key) {
                SLIDER_KEY(NONE);
                SLIDER_KEY(ESCAPE);
                SLIDER_KEY(LEFT);
                SLIDER_KEY(RIGHT);
                SLIDER_KEY(UP);
                SLIDER_KEY(DOWN);
                SLIDER_KEY(HOME);
                SLIDER_KEY(END);
                SLIDER_KEY(PAGEUP);
                SLIDER_KEY(PAGEDOWN);
            default:
                return false;
            }
#undef SLIDER_KEY
            r.input.repeat = i->repeat;
        } else if (i->type == CUI_EVENT_ACCESSIBILITY_ACTION) {
            switch (i->accessibility_action) {
            case CUI_ACCESSIBILITY_INCREMENT:
                r.input.accessibility_action = CUI_BRIDGE_SLIDER_INCREMENT;
                break;
            case CUI_ACCESSIBILITY_DECREMENT:
                r.input.accessibility_action = CUI_BRIDGE_SLIDER_DECREMENT;
                break;
            case CUI_ACCESSIBILITY_SET_VALUE:
                r.input.accessibility_action = CUI_BRIDGE_SLIDER_SET_VALUE;
                break;
            default:
                return false;
            }
            r.input.accessibility_target = i->accessibility_target;
            if (i->flags & CUI_SLIDER_MODEL_HAS_NUMERIC_VALUE) {
                r.input.flags |= CUI_BRIDGE_SLIDER_HAS_NUMERIC_VALUE;
                r.input.accessibility_value = i->accessibility_value;
            }
        } else {
            r.input.x = i->x;
            r.input.y = i->y;
            r.input.pointer_id = i->pointer_id;
            r.input.pointer_device = i->pointer_device;
            switch (i->pointer_type) {
            case CUI_POINTER_MOUSE:
                r.input.pointer_type = CUI_BRIDGE_SLIDER_MOUSE;
                break;
            case CUI_POINTER_TOUCH:
                r.input.pointer_type = CUI_BRIDGE_SLIDER_TOUCH;
                break;
            case CUI_POINTER_PEN:
                r.input.pointer_type = CUI_BRIDGE_SLIDER_PEN;
                break;
            default:
                return false;
            }
            switch (i->pointer_button) {
            case CUI_POINTER_PRIMARY:
                r.input.pointer_button = CUI_BRIDGE_SLIDER_PRIMARY;
                break;
            case CUI_POINTER_SECONDARY:
                r.input.pointer_button = CUI_BRIDGE_SLIDER_SECONDARY;
                break;
            case CUI_POINTER_MIDDLE:
                r.input.pointer_button = CUI_BRIDGE_SLIDER_MIDDLE;
                break;
            default:
                return false;
            }
        }
    }
    *out = r;
    return true;
}
static void bridge_slider_request(void *user, const cui_slider_model_request_v1 *request) {
    bridge_slider_owned *owned = user;
    bridge_slider_app *state = owned->app->slider;
    cui_bridge_slider_request_v1 copy;
    if (owned->emitted_error || state->count == state->capacity ||
        !bridge_slider_map_request(owned, request, &copy)) {
        owned->emitted_error = true;
        owned->app->frame_error = CUI_BRIDGE_CORE_ERROR;
        return;
    }
    state->queue[(state->head + state->count) % state->capacity] = copy;
    state->count++;
    if (!owned->first)
        owned->first = request->request_sequence;
    owned->last = request->request_sequence;
}
static void bridge_slider_begin(cui_bridge_app *app) {
    if (!app->slider)
        return;
    app->slider->frame = cui_frame_sequence(app->ui);
    for (uint32_t n = 1; n <= app->max_nodes; n++)
        if (app->slider->nodes[n]) {
            bridge_slider_owned *owned = app->slider->nodes[n];
            if (app->slider->frame && owned->model.owner &&
                !cui_frame_input_snapshot_ready(app->ui))
                app->slider->failed_frame = true;
            owned->seen_frame = 0;
            owned->layout_seen = false;
            owned->emitted_error = false;
        }
}
static void bridge_slider_render(cui_bridge_app *app, uint32_t slot) {
    bridge_slider_owned *owned = app->slider ? app->slider->nodes[slot] : NULL;
    if (!owned) {
        app->frame_error = CUI_BRIDGE_INVALID_STATE;
        return;
    }
    cui_bridge_node_record *node = &app->nodes[slot];
    uint64_t frame = cui_frame_sequence(app->ui);
    if (!frame)
        owned->layout_seen = true;
    else
        owned->first = owned->last = 0;
    bool requests = (owned->config.flags & CUI_BRIDGE_SLIDER_REQUESTS) != 0;
    if (frame && requests) {
        size_t count = 0;
        (void)cui_frame_events(app->ui, &count);
        uint64_t events = (uint64_t)count;
        if (events > (UINT64_MAX - 1) / 3 ||
            3 * events + 1 > app->slider->capacity - app->slider->count) {
            owned->window = false;
            app->frame_error = CUI_BRIDGE_CAPACITY;
            return;
        }
    }
    cui_styled_slider_model_options_v1 paint;
    if (!cui_styled_slider_model_options_from_theme_v1(
            &app->theme, owned->config.rem,
            (owned->config.orientation == CUI_BRIDGE_SLIDER_HORIZONTAL ? CUI_SLIDER_MODEL_HORIZONTAL
                                                                       : CUI_SLIDER_MODEL_VERTICAL),
            &paint)) {
        owned->window = false;
        app->frame_error = CUI_BRIDGE_INVALID_ARGUMENT;
        return;
    }
    paint.style.layout = owned->config.layout == CUI_BRIDGE_SLIDER_FIXED ? CUI_SLIDER_STYLE_FIXED
                                                                         : CUI_SLIDER_STYLE_PARENT;
    paint.style.length = owned->config.length;
    if (owned->config.flags & CUI_BRIDGE_SLIDER_REDUCED_MOTION)
        paint.flags |= CUI_STYLED_SLIDER_REDUCED_MOTION;
    if (owned->config.flags & CUI_BRIDGE_SLIDER_HIDE_FOCUS)
        paint.flags |= CUI_STYLED_SLIDER_HIDE_FOCUS;
    const char *name = node->text[0] ? node->text : "Selection";
    char start[4110], end[4110];
    snprintf(start, sizeof start, "%s start", name);
    snprintf(end, sizeof end, "%s end", name);
    cui_slider_model_options_v1 options = {
        .version = 1,
        .struct_size = sizeof options,
        .flags = (node->disabled ? CUI_SLIDER_MODEL_DISABLED_BIT : 0) |
                 ((owned->config.flags & CUI_BRIDGE_SLIDER_READONLY) ? CUI_SLIDER_MODEL_READONLY_BIT
                                                                     : 0) |
                 ((owned->config.flags & CUI_BRIDGE_SLIDER_REVERSE_FILL)
                      ? CUI_SLIDER_MODEL_REVERSE_FILL_BIT
                      : 0),
        .orientation = owned->config.orientation == CUI_BRIDGE_SLIDER_HORIZONTAL
                           ? CUI_SLIDER_MODEL_HORIZONTAL
                           : CUI_SLIDER_MODEL_VERTICAL,
        .source_generation = owned->config.source_generation,
        .domain = bridge_slider_domain(owned->config.domain),
        .name = cui_string(name),
        .start_name = cui_string(start),
        .end_name = cui_string(end)};
    cui_styled_slider_model_result_v1 result = cui_styled_slider_model_events_v1(
        app->ui, node->id, bridge_slider_value(owned->value), &owned->model, &options,
        &owned->presentation, &paint,
        (cui_slider_model_handler_v1){requests ? bridge_slider_request : NULL, owned});
    uint32_t render_status;
    switch (result.render_status) {
    case CUI_STYLED_SLIDER_RENDER_OK:
        render_status = CUI_BRIDGE_SLIDER_RENDER_OK;
        break;
    case CUI_STYLED_SLIDER_INVALID_ARGUMENT:
        render_status = CUI_BRIDGE_SLIDER_RENDER_INVALID_ARGUMENT;
        break;
    case CUI_STYLED_SLIDER_INVALID_STATE:
        render_status = CUI_BRIDGE_SLIDER_RENDER_INVALID_STATE;
        break;
    case CUI_STYLED_SLIDER_UNSUPPORTED:
        render_status = CUI_BRIDGE_SLIDER_RENDER_UNSUPPORTED;
        break;
    case CUI_STYLED_SLIDER_BASE_REJECTED:
        render_status = CUI_BRIDGE_SLIDER_RENDER_BASE_REJECTED;
        break;
    default:
        render_status = CUI_BRIDGE_SLIDER_RENDER_CORE_FAILURE;
        break;
    }
    owned->seen_frame = frame;
    if (frame)
        owned->observation = (cui_bridge_slider_presentation_v1){
            sizeof owned->observation, 1, render_status, 0, cui_frame_time(app->ui),
            result.next_wake_seconds};
    if (result.render_status != CUI_STYLED_SLIDER_RENDER_OK)
        app->frame_error = result.render_status == CUI_STYLED_SLIDER_UNSUPPORTED
                               ? CUI_BRIDGE_BACKEND_UNAVAILABLE
                               : CUI_BRIDGE_CORE_ERROR;
}
static void bridge_slider_finish(cui_bridge_app *app, cui_bridge_status step_status) {
    if (!app->slider)
        return;
    bool valid = step_status == CUI_BRIDGE_OK &&
                 cui_accessibility_numeric_snapshot_ready(app->ui) &&
                 cui_last_error(app->ui) == CUI_OK;
    if (!valid)
        app->slider->failed_frame = true;
    bool admitted = false, blocked = false;
    for (uint32_t n = 1; n <= app->max_nodes; n++)
        if (app->slider->nodes[n]) {
            bridge_slider_owned *owned = app->slider->nodes[n];
            if (!valid) {
                owned->window = false;
                continue;
            }
            if (owned->layout_seen) {
                blocked = true;
                continue;
            }
            if (owned->seen_frame != app->slider->frame || !owned->seen_frame) {
                owned->window = false;
                owned->observed = false;
                continue;
            }
            bool state_frame = owned->model.last_painted_frame == owned->seen_frame;
            admitted |= state_frame;
            blocked |= !state_frame;
            owned->observed = state_frame;
            owned->valid_frame = owned->seen_frame;
            owned->window = !owned->emitted_error &&
                            owned->observation.render_status == CUI_BRIDGE_SLIDER_RENDER_OK &&
                            state_frame && owned->first != 0;
        }
    if (valid && admitted && !blocked)
        app->slider->failed_frame = false;
}
cui_bridge_status cui_bridge_poll_slider_request_v1(cui_bridge_app *app, uint32_t size,
                                                    cui_bridge_slider_request_v1 *out) {
    cui_bridge_status status = bridge_slider_ready(app);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!app->slider)
        return CUI_BRIDGE_INVALID_STATE;
    if (size != sizeof *out || !out)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    bridge_slider_app *state = app->slider;
    while (state->count) {
        cui_bridge_slider_request_v1 copy = state->queue[state->head];
        state->head = (state->head + 1) % state->capacity;
        state->count--;
        if (!bridge_node(app, copy.receipt.node))
            continue;
        *out = copy;
        return CUI_BRIDGE_OK;
    }
    return CUI_BRIDGE_EMPTY;
}
cui_bridge_status cui_bridge_accept_slider_request_v1(cui_bridge_app *app, uint32_t rs,
                                                      const cui_bridge_slider_receipt_v1 *receipt,
                                                      uint32_t vs,
                                                      const cui_bridge_slider_value_v1 *value) {
    cui_bridge_status status = bridge_slider_ready(app);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (rs != sizeof *receipt || !receipt || receipt->struct_size != rs ||
        receipt->schema_version != 1 || vs != sizeof *value || !value)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_bridge_slider_receipt_v1 r = *receipt;
    bridge_slider_owned *owned;
    status = bridge_slider_get(app, r.node, &owned);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (app->slider->failed_frame || !owned->window ||
        !cui_accessibility_numeric_snapshot_ready(app->ui) || cui_last_error(app->ui) != CUI_OK ||
        r.app_epoch != app->slider->epoch ||
        r.source_generation != owned->config.source_generation ||
        r.configuration_revision != owned->revision || r.submission_frame != owned->valid_frame ||
        !r.request_sequence || r.request_sequence < owned->first ||
        r.request_sequence > owned->last || r.request_sequence <= owned->accepted ||
        app->nodes[r.node.slot].disabled || !(owned->config.flags & CUI_BRIDGE_SLIDER_REQUESTS) ||
        (owned->config.flags & CUI_BRIDGE_SLIDER_READONLY))
        return CUI_BRIDGE_INVALID_STATE;
    if (value->kind != owned->value.kind ||
        !cui_slider_model_value_valid_v1(bridge_slider_domain(owned->config.domain),
                                         bridge_slider_value(*value)))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    owned->value = bridge_slider_copy_value(bridge_slider_value(*value));
    owned->accepted = r.request_sequence;
    cui_invalidate(app->ui);
    return CUI_BRIDGE_OK;
}
cui_bridge_status
cui_bridge_slider_model_get_presentation_v1(cui_bridge_app *app, cui_bridge_node handle,
                                            uint32_t size, cui_bridge_slider_presentation_v1 *out) {
    bridge_slider_owned *owned;
    cui_bridge_status status = bridge_slider_get(app, handle, &owned);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (size != sizeof *out || !out)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    if (app->slider->failed_frame)
        return CUI_BRIDGE_CORE_ERROR;
    if (!owned->observed)
        return CUI_BRIDGE_EMPTY;
    if (!cui_accessibility_numeric_snapshot_ready(app->ui) || cui_last_error(app->ui) != CUI_OK)
        return CUI_BRIDGE_CORE_ERROR;
    *out = owned->observation;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_slider_model_part_bounds_v1(cui_bridge_app *app,
                                                         cui_bridge_node handle, uint32_t part,
                                                         cui_bridge_rect *out) {
    bridge_slider_owned *owned;
    cui_bridge_status status = bridge_slider_get(app, handle, &owned);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (part > 4 || !out)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    if (app->slider->failed_frame)
        return CUI_BRIDGE_CORE_ERROR;
    if (!owned->observed)
        return CUI_BRIDGE_EMPTY;
    if (!cui_accessibility_numeric_snapshot_ready(app->ui) || cui_last_error(app->ui) != CUI_OK)
        return CUI_BRIDGE_CORE_ERROR;
    if ((part == CUI_BRIDGE_SLIDER_START_THUMB && owned->value.kind == CUI_BRIDGE_SLIDER_SINGLE) ||
        (part >= CUI_BRIDGE_SLIDER_START_THUMB && app->nodes[handle.slot].disabled))
        return CUI_BRIDGE_EMPTY;
    const uint32_t parts[] = {CUI_SLIDER_MODEL_ROOT, CUI_SLIDER_MODEL_TRACK, CUI_SLIDER_MODEL_FILL,
                              CUI_SLIDER_MODEL_START_THUMB, CUI_SLIDER_MODEL_END_THUMB};
    cui_rect r = cui_bounds(
        app->ui, cui_base_slider_model_part_id_v1(app->nodes[handle.slot].id, parts[part]));
    if (!isfinite(r.x) || !isfinite(r.y) || !isfinite(r.w) || !isfinite(r.h) || r.w <= 0 ||
        r.h <= 0)
        return CUI_BRIDGE_EMPTY;
    *out = (cui_bridge_rect){r.x, r.y, r.w, r.h};
    return CUI_BRIDGE_OK;
}
#endif
