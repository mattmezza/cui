/* SPDX-License-Identifier: MIT */
#include "cui/cui_accessibility.h"
#include <accesskit.h>
#include <math.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

#ifdef CUI_A11Y_TESTING
static atomic_uint_fast64_t test_live, test_released;
uint64_t cui_accessibility_test_live(void) {
    return atomic_load(&test_live);
}
uint64_t cui_accessibility_test_released(void) {
    return atomic_load(&test_released);
}
#endif

/* Private extension built from cui_lifetime.rs; the normal upstream ABI stays intact. */
extern accesskit_unix_adapter *cui_accesskit_unix_adapter_new_owned(
    accesskit_activation_handler_callback, accesskit_action_handler_callback,
    accesskit_deactivation_handler_callback, void *, void (*)(void *));
typedef struct {
    cui_semantic semantic;
    uint64_t token, parent;
    unsigned actions;
} record;
typedef struct {
    record *nodes;
    char *strings;
    size_t count;
    cui_accessibility_window window;
} snapshot;
typedef struct {
    uint64_t token;
    cui_accessibility_action action;
} request;
struct cui_accessibility {
    mtx_t mutex;
    atomic_uint references;
    bool closed;
    cui_accessibility_config config;
    snapshot banks[2];
    unsigned live;
    uint64_t next_token;
    request *requests;
    size_t head, count;
    cui_accessibility_stats stats;
    accesskit_unix_adapter *adapter;
};
static void release(void *opaque) {
    cui_accessibility *a = opaque;
    if (atomic_fetch_sub_explicit(&a->references, 1, memory_order_acq_rel) != 1)
        return;
#ifdef CUI_A11Y_TESTING
    atomic_fetch_sub(&test_live, 1);
    atomic_fetch_add(&test_released, 1);
#endif
    free(a->banks[0].nodes);
    free(a->banks[1].nodes);
    free(a->banks[0].strings);
    free(a->banks[1].strings);
    free(a->requests);
    mtx_destroy(&a->mutex);
    free(a);
}
static record *find_id(snapshot *s, cui_id id) {
    for (size_t i = 0; i < s->count; ++i)
        if (s->nodes[i].semantic.id == id)
            return &s->nodes[i];
    return NULL;
}
static record *find_token(snapshot *s, uint64_t token) {
    for (size_t i = 0; i < s->count; ++i)
        if (s->nodes[i].token == token)
            return &s->nodes[i];
    return NULL;
}
static uint64_t relation(snapshot *s, cui_id id) {
    record *r = find_id(s, id);
    return r ? r->token : 0;
}
static accesskit_role role(cui_semantic s) {
    static const accesskit_role roles[] = {ACCESSKIT_ROLE_GENERIC_CONTAINER,
                                           ACCESSKIT_ROLE_BUTTON,
                                           ACCESSKIT_ROLE_CHECK_BOX,
                                           ACCESSKIT_ROLE_SWITCH,
                                           ACCESSKIT_ROLE_RADIO_BUTTON,
                                           ACCESSKIT_ROLE_SLIDER,
                                           ACCESSKIT_ROLE_TEXT_INPUT,
                                           ACCESSKIT_ROLE_LABEL,
                                           ACCESSKIT_ROLE_TAB,
                                           ACCESSKIT_ROLE_GROUP,
                                           ACCESSKIT_ROLE_TAB_LIST,
                                           ACCESSKIT_ROLE_TAB_PANEL,
                                           ACCESSKIT_ROLE_LIST,
                                           ACCESSKIT_ROLE_LIST_BOX,
                                           ACCESSKIT_ROLE_LIST_BOX_OPTION,
                                           ACCESSKIT_ROLE_MENU,
                                           ACCESSKIT_ROLE_MENU_ITEM,
                                           ACCESSKIT_ROLE_DIALOG,
                                           ACCESSKIT_ROLE_ALERT,
                                           ACCESSKIT_ROLE_STATUS,
                                           ACCESSKIT_ROLE_IMAGE,
                                           ACCESSKIT_ROLE_PROGRESS_INDICATOR,
                                           ACCESSKIT_ROLE_TREE,
                                           ACCESSKIT_ROLE_TREE_ITEM,
                                           ACCESSKIT_ROLE_MENU_ITEM_CHECK_BOX,
                                           ACCESSKIT_ROLE_MENU_ITEM_RADIO,
                                           ACCESSKIT_ROLE_TABLE,
                                           ACCESSKIT_ROLE_ROW,
                                           ACCESSKIT_ROLE_CELL,
                                           ACCESSKIT_ROLE_COLUMN_HEADER,
                                           ACCESSKIT_ROLE_GRID_CELL,
                                           ACCESSKIT_ROLE_GRID,
                                           ACCESSKIT_ROLE_SCROLL_BAR,
                                           ACCESSKIT_ROLE_ROW_HEADER,
                                           ACCESSKIT_ROLE_ROW_GROUP,
                                           ACCESSKIT_ROLE_ALERT_DIALOG,
                                           ACCESSKIT_ROLE_LINK,
                                           ACCESSKIT_ROLE_LIST_ITEM,
                                           ACCESSKIT_ROLE_LOG,
                                           ACCESSKIT_ROLE_TOOLBAR,
                                           ACCESSKIT_ROLE_SPLITTER,
                                           ACCESSKIT_ROLE_REGION,
                                           ACCESSKIT_ROLE_RADIO_GROUP};
    _Static_assert(sizeof roles / sizeof roles[0] == CUI_ROLE_RADIOGROUP + 1, "role mapping");
    return s.role == CUI_ROLE_TEXTBOX && s.details.password    ? ACCESSKIT_ROLE_PASSWORD_INPUT
           : s.role == CUI_ROLE_TEXTBOX && s.details.multiline ? ACCESSKIT_ROLE_MULTILINE_TEXT_INPUT
                                                               : roles[s.role];
}
static accesskit_rect rectangle(cui_rect r) {
    return (accesskit_rect){r.x, r.y, (double)r.x + r.w, (double)r.y + r.h};
}
static accesskit_tree_update *make_tree(void *opaque) {
    cui_accessibility *a = opaque;
    mtx_lock(&a->mutex);
    if (a->closed) {
        mtx_unlock(&a->mutex);
        return NULL;
    }
    a->stats.native_active = true;
    snapshot *s = &a->banks[a->live];
    uint64_t focus = 1;
    for (size_t i = 0; i < s->count; ++i)
        if (s->nodes[i].semantic.focused)
            focus = s->nodes[i].token;
    accesskit_tree_update *tree =
        accesskit_tree_update_with_capacity_and_focus(s->count + 1, focus);
    accesskit_tree_update_set_tree_info(tree, accesskit_tree_info_new(1));
    accesskit_node *root = accesskit_node_new(ACCESSKIT_ROLE_WINDOW);
    accesskit_node_set_label_with_length(root, s->window.title.data, s->window.title.length);
    accesskit_node_set_bounds(root,
                              rectangle((cui_rect){0, 0, s->window.inner.w, s->window.inner.h}));
    for (size_t i = 0; i < s->count; ++i)
        if (s->nodes[i].parent == 1)
            accesskit_node_push_child(root, s->nodes[i].token);
    accesskit_tree_update_push_node(tree, 1, root);
    for (size_t i = 0; i < s->count; ++i) {
        record *r = &s->nodes[i];
        cui_semantic v = r->semantic;
        cui_semantic_details d = v.details;
        accesskit_node *n = accesskit_node_new(role(v));
        if (v.role == CUI_ROLE_LABEL)
            accesskit_node_set_value_with_length(n, v.name.data, v.name.length);
        else
            accesskit_node_set_label_with_length(n, v.name.data, v.name.length);
        accesskit_node_set_description_with_length(n, d.description.data, d.description.length);
        accesskit_node_set_bounds(n, rectangle(v.bounds));
        if (v.disabled)
            accesskit_node_set_disabled(n);
        if (d.readonly)
            accesskit_node_set_read_only(n);
        if (d.required)
            accesskit_node_set_required(n);
        if (d.busy)
            accesskit_node_set_busy(n);
        if (d.multiselectable)
            accesskit_node_set_multiselectable(n);
        if (d.invalid)
            accesskit_node_set_invalid(n, ACCESSKIT_INVALID_TRUE);
        if (v.role == CUI_ROLE_CHECKBOX || v.role == CUI_ROLE_SWITCH || v.role == CUI_ROLE_RADIO ||
            v.role == CUI_ROLE_MENUITEMCHECKBOX || v.role == CUI_ROLE_MENUITEMRADIO)
            accesskit_node_set_toggled(n, v.checked ? ACCESSKIT_TOGGLED_TRUE
                                                    : ACCESSKIT_TOGGLED_FALSE);
        if (v.role == CUI_ROLE_OPTION || v.role == CUI_ROLE_TAB || v.role == CUI_ROLE_TREEITEM ||
            v.role == CUI_ROLE_ROW)
            accesskit_node_set_selected(n, d.selected);
        if (d.expanded || v.role == CUI_ROLE_TREEITEM)
            accesskit_node_set_expanded(n, d.expanded);
        if (v.role == CUI_ROLE_STATUS || v.role == CUI_ROLE_LOG)
            accesskit_node_set_live(n, ACCESSKIT_LIVE_POLITE);
        if (v.role == CUI_ROLE_ALERT || v.role == CUI_ROLE_ALERTDIALOG)
            accesskit_node_set_live(n, ACCESSKIT_LIVE_ASSERTIVE);
        if (d.scrollable) {
            accesskit_node_set_scroll_x(n, d.scroll_x);
            accesskit_node_set_scroll_x_min(n, 0);
            accesskit_node_set_scroll_x_max(n, d.scroll_max_x);
            accesskit_node_set_scroll_y(n, d.scroll_y);
            accesskit_node_set_scroll_y_min(n, 0);
            accesskit_node_set_scroll_y_max(n, d.scroll_max_y);
        }
        if (d.orientation)
            accesskit_node_set_orientation(n, d.orientation == CUI_ORIENTATION_HORIZONTAL
                                                  ? ACCESSKIT_ORIENTATION_HORIZONTAL
                                                  : ACCESSKIT_ORIENTATION_VERTICAL);
        if (v.role == CUI_ROLE_SLIDER || v.role == CUI_ROLE_PROGRESSBAR ||
            v.role == CUI_ROLE_SCROLLBAR) {
            accesskit_node_set_numeric_value(n, v.value);
            accesskit_node_set_min_numeric_value(n, d.minimum);
            accesskit_node_set_max_numeric_value(n, d.maximum);
        }
        if (d.level)
            accesskit_node_set_level(n, d.level);
        if (d.pos_in_set)
            accesskit_node_set_position_in_set(n, d.pos_in_set);
        if (d.set_size)
            accesskit_node_set_size_of_set(n, d.set_size);
        if (d.row_count)
            accesskit_node_set_row_count(n, d.row_count);
        if (d.column_count)
            accesskit_node_set_column_count(n, d.column_count);
        if (d.row_index)
            accesskit_node_set_row_index(n, d.row_index - 1);
        if (d.column_index)
            accesskit_node_set_column_index(n, d.column_index - 1);
        uint64_t target = relation(s, d.controls);
        if (target)
            accesskit_node_set_controls(n, 1, &target);
        target = relation(s, d.labelled_by);
        if (target)
            accesskit_node_set_labelled_by(n, 1, &target);
        for (size_t j = 0; j < s->count; ++j)
            if (s->nodes[j].parent == r->token)
                accesskit_node_push_child(n, s->nodes[j].token);
        if (r->actions & 1u)
            accesskit_node_add_action(n, ACCESSKIT_ACTION_FOCUS);
        if (r->actions & 2u)
            accesskit_node_add_action(n, ACCESSKIT_ACTION_CLICK);
        accesskit_tree_update_push_node(tree, r->token, n);
    }
    mtx_unlock(&a->mutex);
    return tree;
}
static void action(accesskit_action_request *r, void *opaque) {
    cui_accessibility *a = opaque;
    if (r->action == ACCESSKIT_ACTION_FOCUS || r->action == ACCESSKIT_ACTION_CLICK)
        cui_accessibility_request(a, r->target_node,
                                  r->action == ACCESSKIT_ACTION_FOCUS ? CUI_ACCESSIBILITY_FOCUS
                                                                      : CUI_ACCESSIBILITY_ACTIVATE);
    accesskit_action_request_free(r);
}
static void deactivate(void *opaque) {
    cui_accessibility *a = opaque;
    mtx_lock(&a->mutex);
    a->stats.native_active = false;
    mtx_unlock(&a->mutex);
}
cui_accessibility *cui_accessibility_create(cui_accessibility_config c) {
    if (!c.max_nodes)
        c.max_nodes = 2048;
    if (!c.string_bytes)
        c.string_bytes = 65536;
    if (!c.max_actions)
        c.max_actions = 128;
    if (c.max_nodes > SIZE_MAX / sizeof(record) || c.max_actions > SIZE_MAX / sizeof(request))
        return NULL;
    cui_accessibility *a = calloc(1, sizeof(*a));
    if (!a)
        return NULL;
    if (mtx_init(&a->mutex, mtx_plain) != thrd_success) {
        free(a);
        return NULL;
    }
#ifdef CUI_A11Y_TESTING
    atomic_fetch_add(&test_live, 1);
#endif
    atomic_init(&a->references, 1);
    a->config = c;
    a->next_token = 2;
    for (unsigned i = 0; i < 2; ++i) {
        a->banks[i].nodes = calloc(c.max_nodes, sizeof(record));
        a->banks[i].strings = malloc(c.string_bytes);
        if (!a->banks[i].nodes || !a->banks[i].strings) {
            release(a);
            return NULL;
        }
        a->banks[i].strings[0] = 0;
        a->banks[i].window.title = (cui_str){a->banks[i].strings, 0};
    }
    a->requests = calloc(c.max_actions, sizeof(request));
    if (!a->requests) {
        release(a);
        return NULL;
    }
    atomic_fetch_add_explicit(&a->references, 1, memory_order_relaxed);
    a->adapter = cui_accesskit_unix_adapter_new_owned(make_tree, action, deactivate, a, release);
    return a;
}
/* Reject invalid UTF-8/NUL instead of panicking inside AccessKit's FFI. */
static bool valid_string(cui_str s) {
    if (s.length && !s.data)
        return false;
    for (size_t i = 0; i < s.length;) {
        unsigned char c = (unsigned char)s.data[i++];
        unsigned n = 0;
        uint32_t cp = 0, min = 0;
        if (!c)
            return false;
        if (c < 128)
            continue;
        if (c >= 0xc2 && c <= 0xdf) {
            n = 1;
            cp = c & 31u;
            min = 0x80;
        } else if (c >= 0xe0 && c <= 0xef) {
            n = 2;
            cp = c & 15u;
            min = 0x800;
        } else if (c >= 0xf0 && c <= 0xf4) {
            n = 3;
            cp = c & 7u;
            min = 0x10000;
        } else
            return false;
        if (n > s.length - i)
            return false;
        while (n--) {
            unsigned char b = (unsigned char)s.data[i++];
            if ((b & 0xc0) != 0x80)
                return false;
            cp = (cp << 6) | (b & 63u);
        }
        if (cp < min || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff))
            return false;
    }
    return true;
}
static bool valid_rect(cui_rect r) {
    return isfinite(r.x) && isfinite(r.y) && isfinite(r.w) && isfinite(r.h) && r.w >= 0 && r.h >= 0;
}
static bool add_string(size_t *size, cui_str s, size_t limit) {
    if (*size >= limit || s.length > limit - *size - 1)
        return false;
    *size += s.length + 1;
    return true;
}
static cui_str copy_string(snapshot *s, size_t *offset, cui_str text) {
    char *p = s->strings + *offset;
    if (text.length)
        memcpy(p, text.data, text.length);
    p[text.length] = 0;
    *offset += text.length + 1;
    return (cui_str){p, text.length};
}
static bool same_string(cui_str a, cui_str b) {
    return a.length == b.length && (!a.length || memcmp(a.data, b.data, a.length) == 0);
}
static bool same_rect(cui_rect a, cui_rect b) {
    return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h;
}
static bool same_snapshot(snapshot *a, snapshot *b) {
    if (a->count != b->count || !same_rect(a->window.outer, b->window.outer) ||
        !same_rect(a->window.inner, b->window.inner) || a->window.focused != b->window.focused ||
        a->window.has_screen_position != b->window.has_screen_position ||
        !same_string(a->window.title, b->window.title))
        return false;
    for (size_t i = 0; i < a->count; ++i) {
        record x = a->nodes[i], y = b->nodes[i];
        cui_semantic u = x.semantic, v = y.semantic;
        cui_semantic_details d = u.details, e = v.details;
        if (x.token != y.token || x.parent != y.parent || x.actions != y.actions ||
            u.role != v.role || u.disabled != v.disabled || u.checked != v.checked ||
            u.focused != v.focused || u.value != v.value || !same_rect(u.bounds, v.bounds) ||
            !same_string(u.name, v.name) || !same_string(d.description, e.description))
            return false;
#define EQ(field)                                                                                  \
    if (d.field != e.field)                                                                        \
    return false
        EQ(secret);
        EQ(password);
        EQ(readonly);
        EQ(required);
        EQ(busy);
        EQ(multiselectable);
        EQ(invalid);
        EQ(selected);
        EQ(expanded);
        EQ(multiline);
        EQ(orientation);
        EQ(minimum);
        EQ(maximum);
        EQ(level);
        EQ(pos_in_set);
        EQ(set_size);
        EQ(row_count);
        EQ(column_count);
        EQ(row_index);
        EQ(column_index);
        EQ(controls);
        EQ(labelled_by);
        EQ(scrollable);
        EQ(scroll_x);
        EQ(scroll_y);
        EQ(scroll_max_x);
        EQ(scroll_max_y);
#undef EQ
    }
    return true;
}
bool cui_accessibility_update(cui_accessibility *a, cui_context *ui,
                              cui_accessibility_window window) {
    if (!a || !ui)
        return false;
    size_t count = 0;
    const cui_semantic *sem = cui_semantics(ui, &count);
    cui_accessibility_status error = cui_last_error(ui) == CUI_OK ? CUI_A11Y_OK : CUI_A11Y_INVALID;
    size_t bytes = 0;
    if (count > a->config.max_nodes || !add_string(&bytes, window.title, a->config.string_bytes))
        error = CUI_A11Y_CAPACITY;
    if (!valid_rect(window.outer) || !valid_rect(window.inner) || !valid_string(window.title))
        error = CUI_A11Y_INVALID;
    for (size_t i = 0; i < count && error == CUI_A11Y_OK; ++i) {
        if (!sem[i].id || sem[i].role < CUI_ROLE_NONE || sem[i].role > CUI_ROLE_RADIOGROUP ||
            sem[i].details.orientation < CUI_ORIENTATION_UNSPECIFIED ||
            sem[i].details.orientation > CUI_ORIENTATION_VERTICAL || !valid_rect(sem[i].bounds) ||
            !valid_string(sem[i].name) || !valid_string(sem[i].details.description) ||
            !isfinite(sem[i].value) || !isfinite(sem[i].details.minimum) ||
            !isfinite(sem[i].details.maximum) || !isfinite(sem[i].details.scroll_x) ||
            !isfinite(sem[i].details.scroll_y) || !isfinite(sem[i].details.scroll_max_x) ||
            !isfinite(sem[i].details.scroll_max_y))
            error = CUI_A11Y_INVALID;
        else if (!add_string(&bytes, sem[i].name, a->config.string_bytes) ||
                 !add_string(&bytes, sem[i].details.description, a->config.string_bytes))
            error = CUI_A11Y_CAPACITY;
    }
    mtx_lock(&a->mutex);
    if (a->closed)
        error = CUI_A11Y_CLOSED;
    if (error != CUI_A11Y_OK) {
        a->stats.status = error;
        mtx_unlock(&a->mutex);
        return false;
    }
    snapshot *old = &a->banks[a->live], *next = &a->banks[a->live ^ 1u];
    uint64_t generation = a->next_token;
    size_t offset = 0;
    next->count = count;
    next->window = window;
    next->window.title = copy_string(next, &offset, window.title);
    for (size_t i = 0; i < count; ++i) {
        record *r = &next->nodes[i], *previous = find_id(old, sem[i].id);
        *r = (record){.semantic = sem[i]};
        r->semantic.name = copy_string(next, &offset, sem[i].name);
        r->semantic.details.description = copy_string(next, &offset, sem[i].details.description);
        if (previous && previous->semantic.role == sem[i].role)
            r->token = previous->token;
        else {
            if (generation == UINT64_MAX) {
                error = CUI_A11Y_CAPACITY;
                break;
            }
            r->token = generation++;
        }
        r->actions =
            (cui_accessibility_action_supported(ui, sem[i].id, CUI_ACCESSIBILITY_FOCUS) ? 1u : 0u) |
            (cui_accessibility_action_supported(ui, sem[i].id, CUI_ACCESSIBILITY_ACTIVATE) ? 2u
                                                                                           : 0u);
    }
    for (size_t i = 0; i < count && error == CUI_A11Y_OK; ++i) {
        record *r = &next->nodes[i];
        r->parent = relation(next, cui_semantic_parent(ui, r->semantic.id));
        if (!r->parent)
            r->parent = 1;
        uint64_t ancestor = r->parent;
        for (size_t depth = 0; ancestor != 1; ++depth) {
            record *p = find_token(next, ancestor);
            if (!p || depth >= count || ancestor == r->token) {
                error = CUI_A11Y_INVALID;
                break;
            }
            ancestor = relation(next, cui_semantic_parent(ui, p->semantic.id));
            if (!ancestor)
                ancestor = 1;
        }
    }
    a->stats.status = error;
    if (error != CUI_A11Y_OK) {
        mtx_unlock(&a->mutex);
        return false;
    }
    if (same_snapshot(old, next)) {
        mtx_unlock(&a->mutex);
        return true;
    }
    a->live ^= 1u;
    a->next_token = generation;
    ++a->stats.snapshots;
    mtx_unlock(&a->mutex);
    if (window.has_screen_position)
        accesskit_unix_adapter_set_root_window_bounds(a->adapter, rectangle(window.outer),
                                                      rectangle(window.inner));
    accesskit_unix_adapter_update_window_focus_state(a->adapter, window.focused);
    accesskit_unix_adapter_update_if_active(a->adapter, make_tree, a);
    return true;
}
uint64_t cui_accessibility_token(cui_accessibility *a, cui_id id) {
    if (!a)
        return 0;
    mtx_lock(&a->mutex);
    record *r = find_id(&a->banks[a->live], id);
    uint64_t token = r && !a->closed ? r->token : 0;
    mtx_unlock(&a->mutex);
    return token;
}
bool cui_accessibility_request(cui_accessibility *a, uint64_t token,
                               cui_accessibility_action kind) {
    if (!a)
        return false;
    mtx_lock(&a->mutex);
    record *r = find_token(&a->banks[a->live], token);
    if (a->closed || (kind != CUI_ACCESSIBILITY_FOCUS && kind != CUI_ACCESSIBILITY_ACTIVATE) ||
        !r || !(r->actions & (1u << (unsigned)kind))) {
        ++a->stats.rejected;
        mtx_unlock(&a->mutex);
        return false;
    }
    if (a->count == a->config.max_actions) {
        ++a->stats.overflow;
        a->stats.status = CUI_A11Y_CAPACITY;
        mtx_unlock(&a->mutex);
        return false;
    }
    a->requests[(a->head + a->count) % a->config.max_actions] = (request){token, kind};
    ++a->count;
    /* Locked invocation lets destroy synchronously stop the caller-owned wake user.
     * wake must not reenter this backend or block on the UI thread. */
    if (a->config.wake)
        a->config.wake(a->config.user);
    mtx_unlock(&a->mutex);
    return true;
}
size_t cui_accessibility_drain(cui_accessibility *a, cui_context *ui) {
    if (!a || !ui)
        return 0;
    size_t accepted = 0;
    mtx_lock(&a->mutex);
    while (a->count && !a->closed) {
        request q = a->requests[a->head];
        record *r = find_token(&a->banks[a->live], q.token);
        if (!r || !cui_accessibility_action_supported(ui, r->semantic.id, q.action))
            ++a->stats.rejected;
        else {
            if (!cui_queue_accessibility_action(ui, r->semantic.id, q.action)) {
                a->stats.status = CUI_A11Y_CAPACITY;
                break;
            }
            ++a->stats.accepted;
            ++accepted;
        }
        a->head = (a->head + 1) % a->config.max_actions;
        --a->count;
    }
    mtx_unlock(&a->mutex);
    return accepted;
}
cui_accessibility_stats cui_accessibility_get_stats(cui_accessibility *a) {
    if (!a)
        return (cui_accessibility_stats){.status = CUI_A11Y_INVALID};
    mtx_lock(&a->mutex);
    cui_accessibility_stats s = a->stats;
    s.pending = a->count;
    mtx_unlock(&a->mutex);
    return s;
}
void cui_accessibility_destroy(cui_accessibility *a) {
    if (!a)
        return;
    mtx_lock(&a->mutex);
    a->closed = true;
    a->config.wake = NULL;
    a->count = 0;
    mtx_unlock(&a->mutex);
    accesskit_unix_adapter_free(a->adapter);
    release(a);
}
