/* SPDX-License-Identifier: MIT */
#include "cui/cui_dock.h"
#include <float.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CUI_DOCK_PRIVATE_NONE SIZE_MAX
#define CUI_DOCK_PRIVATE_MAX_DEPTH 64
#define CUI_DOCK_PRIVATE_MAX_DIMENSION 1000000000.0
typedef struct {
    bool used;
    uint32_t generation;
    cui_dock_kind kind;
    size_t parent, a, b, first, last, active, count;
    double ratio, minw, minh;
} cui_dock_private_node;
typedef struct {
    bool used;
    uint64_t key;
    size_t node, prev, next;
} cui_dock_private_panel;
typedef struct {
    cui_dock_private_node *nodes;
    cui_dock_private_panel *panels;
    size_t root;
} cui_dock_private_state;
struct cui_dock {
    cui_dock_config config;
    cui_dock_private_state state, scratch;
    uint32_t generation;
    size_t memory_bytes;
};
static size_t cui_dock_private_node_index(const cui_dock *d, cui_dock_node id) {
    size_t n = (size_t)(uint32_t)id;
    if (!n || n > d->config.node_capacity)
        return SIZE_MAX;
    n--;
    return d->state.nodes[n].used && d->state.nodes[n].generation == (uint32_t)(id >> 32)
               ? n
               : SIZE_MAX;
}
static cui_dock_node cui_dock_private_id(const cui_dock_private_state *s, size_t n) {
    return n == SIZE_MAX ? 0 : ((uint64_t)s->nodes[n].generation << 32) | (uint64_t)(n + 1);
}
static size_t cui_dock_private_find_panel(const cui_dock *d, const cui_dock_private_state *s,
                                          uint64_t key) {
    for (size_t i = 0; i < d->config.panel_capacity; i++)
        if (s->panels[i].used && s->panels[i].key == key)
            return i;
    return SIZE_MAX;
}
static size_t cui_dock_private_free_node(const cui_dock *d, const cui_dock_private_state *s) {
    for (size_t i = 0; i < d->config.node_capacity; i++)
        if (!s->nodes[i].used)
            return i;
    return SIZE_MAX;
}
static size_t cui_dock_private_free_panel(const cui_dock *d, const cui_dock_private_state *s) {
    for (size_t i = 0; i < d->config.panel_capacity; i++)
        if (!s->panels[i].used)
            return i;
    return SIZE_MAX;
}
static void cui_dock_private_init_node(cui_dock_private_state *s, size_t n, uint32_t gen,
                                       size_t parent) {
    s->nodes[n] = (cui_dock_private_node){.used = true,
                                          .generation = gen,
                                          .kind = CUI_DOCK_TABS,
                                          .parent = parent,
                                          .a = SIZE_MAX,
                                          .b = SIZE_MAX,
                                          .first = SIZE_MAX,
                                          .last = SIZE_MAX,
                                          .active = SIZE_MAX,
                                          .ratio = 0.5};
}
static bool cui_dock_private_dimension(double x) {
    return isfinite(x) && x >= 0 && x <= CUI_DOCK_PRIVATE_MAX_DIMENSION;
}
static bool cui_dock_private_ratio(double x) {
    return isfinite(x) && x > 0 && x < 1;
}
cui_dock *cui_dock_create(cui_dock_config c) {
    if (!c.node_capacity)
        c.node_capacity = 128;
    if (!c.panel_capacity)
        c.panel_capacity = 256;
    if (!c.max_depth)
        c.max_depth = 32;
    if (!c.max_serialized_bytes)
        c.max_serialized_bytes = 65536;
    if (c.node_capacity > CUI_DOCK_NODE_LIMIT || c.panel_capacity > CUI_DOCK_PANEL_LIMIT ||
        c.max_serialized_bytes > CUI_DOCK_SERIALIZED_LIMIT ||
        c.max_depth > CUI_DOCK_PRIVATE_MAX_DEPTH || c.node_capacity > UINT32_MAX ||
        c.node_capacity > (size_t)PTRDIFF_MAX / (2 * sizeof(cui_dock_private_node)) ||
        c.panel_capacity > (size_t)PTRDIFF_MAX / (2 * sizeof(cui_dock_private_panel)))
        return NULL;
    size_t nb = c.node_capacity * 2 * sizeof(cui_dock_private_node),
           pb = c.panel_capacity * 2 * sizeof(cui_dock_private_panel);
    if (nb > SIZE_MAX - sizeof(cui_dock) || pb > SIZE_MAX - sizeof(cui_dock) - nb)
        return NULL;
    cui_dock *d = calloc(1, sizeof(*d));
    if (!d)
        return NULL;
    d->config = c;
    d->memory_bytes = sizeof(*d) + nb + pb;
    d->state.nodes = calloc(c.node_capacity, sizeof(*d->state.nodes));
    d->scratch.nodes = calloc(c.node_capacity, sizeof(*d->scratch.nodes));
    d->state.panels = calloc(c.panel_capacity, sizeof(*d->state.panels));
    d->scratch.panels = calloc(c.panel_capacity, sizeof(*d->scratch.panels));
    if (!d->state.nodes || !d->scratch.nodes || !d->state.panels || !d->scratch.panels) {
        cui_dock_destroy(d);
        return NULL;
    }
    d->generation = 1;
    cui_dock_private_init_node(&d->state, 0, 1, SIZE_MAX);
    d->state.root = 0;
    return d;
}
void cui_dock_destroy(cui_dock *d) {
    if (d) {
        free(d->state.nodes);
        free(d->state.panels);
        free(d->scratch.nodes);
        free(d->scratch.panels);
        free(d);
    }
}
cui_dock_node cui_dock_root(const cui_dock *d) {
    return d ? cui_dock_private_id(&d->state, d->state.root) : 0;
}
size_t cui_dock_memory_bytes(const cui_dock *d) {
    return d ? d->memory_bytes : 0;
}
bool cui_dock_node_get(const cui_dock *d, cui_dock_node id, cui_dock_node_info *out) {
    if (!d || !out)
        return false;
    size_t i = cui_dock_private_node_index(d, id);
    if (i == SIZE_MAX)
        return false;
    const cui_dock_private_node *n = &d->state.nodes[i];
    *out = (cui_dock_node_info){id,
                                cui_dock_private_id(&d->state, n->parent),
                                cui_dock_private_id(&d->state, n->a),
                                cui_dock_private_id(&d->state, n->b),
                                n->kind,
                                n->ratio,
                                n->minw,
                                n->minh,
                                n->count,
                                n->active != SIZE_MAX,
                                n->active != SIZE_MAX ? d->state.panels[n->active].key : 0};
    return true;
}
bool cui_dock_panel_at(const cui_dock *d, cui_dock_node id, size_t index, uint64_t *key) {
    if (!d || !key)
        return false;
    size_t n = cui_dock_private_node_index(d, id);
    if (n == SIZE_MAX || d->state.nodes[n].kind != CUI_DOCK_TABS ||
        index >= d->state.nodes[n].count)
        return false;
    size_t p = d->state.nodes[n].first;
    while (index--)
        p = d->state.panels[p].next;
    *key = d->state.panels[p].key;
    return true;
}
bool cui_dock_panel_find(const cui_dock *d, uint64_t key, cui_dock_node *tab, size_t *index) {
    if (!d)
        return false;
    size_t p = cui_dock_private_find_panel(d, &d->state, key);
    if (p == SIZE_MAX)
        return false;
    size_t n = d->state.panels[p].node;
    if (tab)
        *tab = cui_dock_private_id(&d->state, n);
    if (index) {
        *index = 0;
        for (size_t x = d->state.nodes[n].first; x != p; x = d->state.panels[x].next)
            (*index)++;
    }
    return true;
}
static void cui_dock_private_attach(cui_dock_private_state *s, size_t p, size_t n, size_t index) {
    cui_dock_private_node *t = &s->nodes[n];
    size_t after = t->first;
    while (index-- && after != SIZE_MAX)
        after = s->panels[after].next;
    size_t before = after == SIZE_MAX ? t->last : s->panels[after].prev;
    s->panels[p].node = n;
    s->panels[p].prev = before;
    s->panels[p].next = after;
    if (before != SIZE_MAX)
        s->panels[before].next = p;
    else
        t->first = p;
    if (after != SIZE_MAX)
        s->panels[after].prev = p;
    else
        t->last = p;
    if (!t->count)
        t->active = p;
    t->count++;
}
static void cui_dock_private_detach(cui_dock_private_state *s, size_t p) {
    cui_dock_private_panel *x = &s->panels[p];
    cui_dock_private_node *t = &s->nodes[x->node];
    if (x->prev != SIZE_MAX)
        s->panels[x->prev].next = x->next;
    else
        t->first = x->next;
    if (x->next != SIZE_MAX)
        s->panels[x->next].prev = x->prev;
    else
        t->last = x->prev;
    if (t->active == p)
        t->active = x->next != SIZE_MAX ? x->next : x->prev;
    t->count--;
}
static void cui_dock_private_collapse(cui_dock_private_state *s, size_t n) {
    cui_dock_private_node *t = &s->nodes[n];
    if (t->count || n == s->root)
        return;
    size_t parent = t->parent,
           sibling = s->nodes[parent].a == n ? s->nodes[parent].b : s->nodes[parent].a,
           grand = s->nodes[parent].parent;
    s->nodes[sibling].parent = grand;
    if (grand == SIZE_MAX)
        s->root = sibling;
    else if (s->nodes[grand].a == parent)
        s->nodes[grand].a = sibling;
    else
        s->nodes[grand].b = sibling;
    t->used = false;
    s->nodes[parent].used = false;
}
cui_dock_result cui_dock_insert(cui_dock *d, cui_dock_node id, uint64_t key, size_t index) {
    if (!d)
        return CUI_DOCK_INVALID;
    size_t n = cui_dock_private_node_index(d, id);
    if (n == SIZE_MAX)
        return CUI_DOCK_NOT_FOUND;
    if (d->state.nodes[n].kind != CUI_DOCK_TABS)
        return CUI_DOCK_INVALID;
    if (index == SIZE_MAX)
        index = d->state.nodes[n].count;
    if (index > d->state.nodes[n].count)
        return CUI_DOCK_INVALID;
    if (cui_dock_private_find_panel(d, &d->state, key) != SIZE_MAX)
        return CUI_DOCK_DUPLICATE_PANEL;
    size_t p = cui_dock_private_free_panel(d, &d->state);
    if (p == SIZE_MAX)
        return CUI_DOCK_CAPACITY;
    d->state.panels[p] = (cui_dock_private_panel){.used = true, .key = key};
    cui_dock_private_attach(&d->state, p, n, index);
    return CUI_DOCK_OK;
}
cui_dock_result cui_dock_split(cui_dock *d, cui_dock_node id, cui_dock_kind axis, double ratio,
                               uint64_t key, bool before, cui_dock_node *out) {
    if (!d || !cui_dock_private_ratio(ratio) ||
        (axis != CUI_DOCK_SPLIT_HORIZONTAL && axis != CUI_DOCK_SPLIT_VERTICAL))
        return CUI_DOCK_INVALID;
    size_t n = cui_dock_private_node_index(d, id);
    if (n == SIZE_MAX)
        return CUI_DOCK_NOT_FOUND;
    if (d->state.nodes[n].kind != CUI_DOCK_TABS || !d->state.nodes[n].count)
        return CUI_DOCK_INVALID;
    if (cui_dock_private_find_panel(d, &d->state, key) != SIZE_MAX)
        return CUI_DOCK_DUPLICATE_PANEL;
    size_t depth = 1;
    for (size_t p = d->state.nodes[n].parent; p != SIZE_MAX; p = d->state.nodes[p].parent)
        depth++;
    if (depth >= d->config.max_depth || d->generation > UINT32_MAX - 2)
        return CUI_DOCK_CAPACITY;
    size_t a = cui_dock_private_free_node(d, &d->state), b = SIZE_MAX,
           p = cui_dock_private_free_panel(d, &d->state);
    if (a != SIZE_MAX)
        for (size_t i = a + 1; i < d->config.node_capacity; i++)
            if (!d->state.nodes[i].used) {
                b = i;
                break;
            }
    if (a == SIZE_MAX || b == SIZE_MAX || p == SIZE_MAX)
        return CUI_DOCK_CAPACITY;
    size_t parent = d->state.nodes[n].parent;
    cui_dock_private_init_node(&d->state, a, ++d->generation, parent);
    cui_dock_private_init_node(&d->state, b, ++d->generation, a);
    d->state.nodes[a].kind = axis;
    d->state.nodes[a].ratio = ratio;
    d->state.nodes[a].a = before ? b : n;
    d->state.nodes[a].b = before ? n : b;
    d->state.nodes[n].parent = a;
    if (parent == SIZE_MAX)
        d->state.root = a;
    else if (d->state.nodes[parent].a == n)
        d->state.nodes[parent].a = a;
    else
        d->state.nodes[parent].b = a;
    d->state.panels[p] = (cui_dock_private_panel){.used = true, .key = key};
    cui_dock_private_attach(&d->state, p, b, 0);
    if (out)
        *out = cui_dock_private_id(&d->state, b);
    return CUI_DOCK_OK;
}
cui_dock_result cui_dock_move(cui_dock *d, uint64_t key, cui_dock_node id, size_t index) {
    if (!d)
        return CUI_DOCK_INVALID;
    size_t n = cui_dock_private_node_index(d, id),
           p = cui_dock_private_find_panel(d, &d->state, key);
    if (n == SIZE_MAX || p == SIZE_MAX)
        return CUI_DOCK_NOT_FOUND;
    if (d->state.nodes[n].kind != CUI_DOCK_TABS)
        return CUI_DOCK_INVALID;
    size_t old = d->state.panels[p].node, newcount = d->state.nodes[n].count - (old == n ? 1 : 0);
    if (index == SIZE_MAX)
        index = newcount;
    if (index > newcount)
        return CUI_DOCK_INVALID;
    bool was_active = d->state.nodes[old].active == p;
    cui_dock_private_detach(&d->state, p);
    cui_dock_private_attach(&d->state, p, n, index);
    if (old != n || was_active)
        d->state.nodes[n].active = p;
    if (old != n)
        cui_dock_private_collapse(&d->state, old);
    return CUI_DOCK_OK;
}
cui_dock_result cui_dock_move_split(cui_dock *d, uint64_t key, cui_dock_node destination,
                                    cui_dock_kind axis, double ratio, bool before,
                                    cui_dock_node *out) {
    if (!d)
        return CUI_DOCK_INVALID;
    cui_dock_node source;
    cui_dock_node_info source_info;
    if (!cui_dock_panel_find(d, key, &source, NULL) || !cui_dock_node_get(d, source, &source_info))
        return CUI_DOCK_NOT_FOUND;
    cui_dock trial = *d;
    trial.state = d->scratch;
    trial.state.root = d->state.root;
    memcpy(trial.state.nodes, d->state.nodes, d->config.node_capacity * sizeof(*d->state.nodes));
    memcpy(trial.state.panels, d->state.panels,
           d->config.panel_capacity * sizeof(*d->state.panels));
    cui_dock_result result = cui_dock_remove(&trial, key);
    cui_dock_node node;
    if (result == CUI_DOCK_OK)
        result = cui_dock_split(&trial, destination, axis, ratio, key, before, &node);
    if (result != CUI_DOCK_OK)
        return result;
    cui_dock_set_minimum(&trial, node, source_info.min_width, source_info.min_height);
    cui_dock_private_state old = d->state;
    d->state = trial.state;
    d->scratch = old;
    d->generation = trial.generation;
    if (out)
        *out = node;
    return CUI_DOCK_OK;
}
cui_dock_result cui_dock_reorder(cui_dock *d, uint64_t key, size_t index) {
    cui_dock_node n;
    if (!cui_dock_panel_find(d, key, &n, NULL))
        return CUI_DOCK_NOT_FOUND;
    return cui_dock_move(d, key, n, index);
}
cui_dock_result cui_dock_remove(cui_dock *d, uint64_t key) {
    if (!d)
        return CUI_DOCK_INVALID;
    size_t p = cui_dock_private_find_panel(d, &d->state, key);
    if (p == SIZE_MAX)
        return CUI_DOCK_NOT_FOUND;
    size_t n = d->state.panels[p].node;
    cui_dock_private_detach(&d->state, p);
    d->state.panels[p].used = false;
    cui_dock_private_collapse(&d->state, n);
    return CUI_DOCK_OK;
}
cui_dock_result cui_dock_activate(cui_dock *d, uint64_t key) {
    if (!d)
        return CUI_DOCK_INVALID;
    size_t p = cui_dock_private_find_panel(d, &d->state, key);
    if (p == SIZE_MAX)
        return CUI_DOCK_NOT_FOUND;
    d->state.nodes[d->state.panels[p].node].active = p;
    return CUI_DOCK_OK;
}
cui_dock_result cui_dock_set_ratio(cui_dock *d, cui_dock_node id, double ratio) {
    if (!d || !cui_dock_private_ratio(ratio))
        return CUI_DOCK_INVALID;
    size_t n = cui_dock_private_node_index(d, id);
    if (n == SIZE_MAX)
        return CUI_DOCK_NOT_FOUND;
    if (d->state.nodes[n].kind == CUI_DOCK_TABS)
        return CUI_DOCK_INVALID;
    d->state.nodes[n].ratio = ratio;
    return CUI_DOCK_OK;
}
cui_dock_result cui_dock_set_minimum(cui_dock *d, cui_dock_node id, double w, double h) {
    if (!d || !cui_dock_private_dimension(w) || !cui_dock_private_dimension(h))
        return CUI_DOCK_INVALID;
    size_t n = cui_dock_private_node_index(d, id);
    if (n == SIZE_MAX)
        return CUI_DOCK_NOT_FOUND;
    d->state.nodes[n].minw = w == 0 ? 0 : w;
    d->state.nodes[n].minh = h == 0 ? 0 : h;
    return CUI_DOCK_OK;
}
typedef struct {
    double w, h;
    size_t tabs;
} cui_dock_private_minimum;
static cui_dock_private_minimum cui_dock_private_min(const cui_dock_private_state *s, size_t i,
                                                     double gap) {
    const cui_dock_private_node *n = &s->nodes[i];
    if (n->kind == CUI_DOCK_TABS)
        return (cui_dock_private_minimum){n->minw, n->minh, 1};
    cui_dock_private_minimum a = cui_dock_private_min(s, n->a, gap),
                             b = cui_dock_private_min(s, n->b, gap);
    return (cui_dock_private_minimum){
        fmax(n->minw, n->kind == CUI_DOCK_SPLIT_HORIZONTAL ? a.w + b.w + gap : fmax(a.w, b.w)),
        fmax(n->minh, n->kind == CUI_DOCK_SPLIT_VERTICAL ? a.h + b.h + gap : fmax(a.h, b.h)),
        a.tabs + b.tabs};
}
static cui_dock_split_geometry cui_dock_private_split(const cui_dock_private_state *s, size_t i,
                                                      cui_rect r, double gap) {
    const cui_dock_private_node *n = &s->nodes[i];
    cui_dock_private_minimum a = cui_dock_private_min(s, n->a, gap),
                             b = cui_dock_private_min(s, n->b, gap);
    bool horizontal = n->kind == CUI_DOCK_SPLIT_HORIZONTAL;
    double span = horizontal ? r.w : r.h, g = fmin(gap, span), available = fmax(0, span - g),
           ma = horizontal ? a.w : a.h, mb = horizontal ? b.w : b.h, first = available * n->ratio;
    if (ma + mb <= available)
        first = fmin(fmax(first, ma), available - mb);
    else if (ma + mb > 0)
        first = available * ma / (ma + mb);
    cui_rect ra = r, rb = r;
    if (horizontal) {
        ra.w = (float)first;
        rb.x += (float)(first + g);
        rb.w = (float)(available - first);
    } else {
        ra.h = (float)first;
        rb.y += (float)(first + g);
        rb.h = (float)(available - first);
    }
    return (cui_dock_split_geometry){.first = ra,
                                     .second = rb,
                                     .first_minimum = {(float)a.w, (float)a.h},
                                     .second_minimum = {(float)b.w, (float)b.h},
                                     .fit_minimum = ma + mb <= available};
}
static void cui_dock_private_geometry(const cui_dock_private_state *s, size_t i, cui_rect r,
                                      double gap, cui_dock_geometry *out, size_t *at) {
    const cui_dock_private_node *n = &s->nodes[i];
    if (n->kind == CUI_DOCK_TABS) {
        out[(*at)++] =
            (cui_dock_geometry){cui_dock_private_id(s, i), r, n->count, n->active != SIZE_MAX,
                                n->active != SIZE_MAX ? s->panels[n->active].key : 0};
        return;
    }
    cui_dock_split_geometry split = cui_dock_private_split(s, i, r, gap);
    cui_rect ra = split.first, rb = split.second;
    cui_dock_private_geometry(s, n->a, ra, gap, out, at);
    cui_dock_private_geometry(s, n->b, rb, gap, out, at);
}
cui_dock_result cui_dock_split_layout(const cui_dock *d, cui_dock_node id, cui_rect r, float gap,
                                      cui_dock_split_geometry *out) {
    if (!d || !out || !isfinite(r.x) || !isfinite(r.y) || !isfinite(r.w) || !isfinite(r.h) ||
        r.w < 0 || r.h < 0 || !isfinite(gap) || gap < 0 || fabs((double)r.x + r.w) > FLT_MAX ||
        fabs((double)r.y + r.h) > FLT_MAX)
        return CUI_DOCK_INVALID;
    size_t node = cui_dock_private_node_index(d, id);
    if (node == SIZE_MAX)
        return CUI_DOCK_NOT_FOUND;
    if (d->state.nodes[node].kind == CUI_DOCK_TABS)
        return CUI_DOCK_INVALID;
    *out = cui_dock_private_split(&d->state, node, r, gap);
    return CUI_DOCK_OK;
}
cui_dock_result cui_dock_layout(const cui_dock *d, cui_rect r, float gap, cui_dock_geometry *out,
                                size_t capacity, size_t *count, bool *fit) {
    if (!d || !isfinite(r.x) || !isfinite(r.y) || !isfinite(r.w) || !isfinite(r.h) || r.w < 0 ||
        r.h < 0 || !isfinite(gap) || gap < 0 || fabs((double)r.x + r.w) > FLT_MAX ||
        fabs((double)r.y + r.h) > FLT_MAX)
        return CUI_DOCK_INVALID;
    cui_dock_private_minimum min = cui_dock_private_min(&d->state, d->state.root, gap);
    if (count)
        *count = min.tabs;
    if (fit)
        *fit = r.w >= min.w && r.h >= min.h;
    if (!out)
        return CUI_DOCK_OK;
    if (capacity < min.tabs)
        return CUI_DOCK_CAPACITY;
    size_t at = 0;
    cui_dock_private_geometry(&d->state, d->state.root, r, gap, out, &at);
    return CUI_DOCK_OK;
}
typedef struct {
    char *out;
    size_t used;
    bool failed;
} cui_dock_private_writer;
static void cui_dock_private_write(cui_dock_private_writer *w, const char *text) {
    size_t n = strlen(text);
    if (n > SIZE_MAX - w->used) {
        w->failed = true;
        return;
    }
    if (w->out)
        memcpy(w->out + w->used, text, n);
    w->used += n;
}
static void cui_dock_private_uint(cui_dock_private_writer *w, uint64_t value) {
    char text[32];
    (void)snprintf(text, sizeof(text), "%llu ", (unsigned long long)value);
    cui_dock_private_write(w, text);
}
static void cui_dock_private_number(cui_dock_private_writer *w, double value) {
    char text[96];
    (void)snprintf(text, sizeof(text), "%.17g", value);
    const char *point = localeconv()->decimal_point;
    if (point && point[0] && strcmp(point, ".")) {
        char *p = strstr(text, point);
        if (p) {
            size_t n = strlen(point);
            *p = '.';
            memmove(p + 1, p + n, strlen(p + n) + 1);
        }
    }
    cui_dock_private_write(w, text);
    cui_dock_private_write(w, " ");
}
static void cui_dock_private_write_node(const cui_dock_private_state *s, size_t i,
                                        cui_dock_private_writer *w) {
    const cui_dock_private_node *n = &s->nodes[i];
    cui_dock_private_write(w, n->kind == CUI_DOCK_TABS               ? "T "
                              : n->kind == CUI_DOCK_SPLIT_HORIZONTAL ? "H "
                                                                     : "V ");
    if (n->kind != CUI_DOCK_TABS)
        cui_dock_private_number(w, n->ratio);
    cui_dock_private_number(w, n->minw);
    cui_dock_private_number(w, n->minh);
    if (n->kind == CUI_DOCK_TABS) {
        size_t active = 0;
        for (size_t p = n->first; p != SIZE_MAX && p != n->active; p = s->panels[p].next)
            active++;
        cui_dock_private_uint(w, active);
        cui_dock_private_uint(w, n->count);
        for (size_t p = n->first; p != SIZE_MAX; p = s->panels[p].next)
            cui_dock_private_uint(w, s->panels[p].key);
    } else {
        cui_dock_private_write_node(s, n->a, w);
        cui_dock_private_write_node(s, n->b, w);
    }
}
cui_dock_result cui_dock_serialize(const cui_dock *d, char *output, size_t capacity,
                                   size_t *length) {
    if (!d)
        return CUI_DOCK_INVALID;
    cui_dock_private_writer w = {0};
    cui_dock_private_write(&w, "CUIDOCK 1 ");
    cui_dock_private_write_node(&d->state, d->state.root, &w);
    cui_dock_private_write(&w, "\n");
    if (length)
        *length = w.used;
    if (w.failed || w.used > d->config.max_serialized_bytes)
        return CUI_DOCK_CAPACITY;
    if (!output)
        return CUI_DOCK_OK;
    if (capacity <= w.used)
        return CUI_DOCK_CAPACITY;
    w = (cui_dock_private_writer){.out = output};
    cui_dock_private_write(&w, "CUIDOCK 1 ");
    cui_dock_private_write_node(&d->state, d->state.root, &w);
    cui_dock_private_write(&w, "\n");
    output[w.used] = 0;
    return CUI_DOCK_OK;
}
typedef struct {
    cui_dock *dock;
    cui_str text;
    size_t pos;
    uint32_t generation;
    cui_dock_result error;
} cui_dock_private_parser;
static bool cui_dock_private_space(char c) {
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}
static bool cui_dock_private_token(cui_dock_private_parser *p, char token[64]) {
    while (p->pos < p->text.length && cui_dock_private_space(p->text.data[p->pos]))
        p->pos++;
    size_t start = p->pos;
    while (p->pos < p->text.length && !cui_dock_private_space(p->text.data[p->pos]))
        p->pos++;
    size_t n = p->pos - start;
    if (!n || n >= 64 || memchr(p->text.data + start, 0, n)) {
        p->error = CUI_DOCK_INVALID;
        return false;
    }
    memcpy(token, p->text.data + start, n);
    token[n] = 0;
    return true;
}
static bool cui_dock_private_read_uint(cui_dock_private_parser *p, uint64_t *value) {
    char token[64];
    if (!cui_dock_private_token(p, token))
        return false;
    uint64_t n = 0;
    for (size_t i = 0; token[i]; i++) {
        if (token[i] < '0' || token[i] > '9') {
            p->error = CUI_DOCK_INVALID;
            return false;
        }
        unsigned digit = (unsigned)(token[i] - '0');
        if (n > (UINT64_MAX - digit) / 10) {
            p->error = CUI_DOCK_INVALID;
            return false;
        }
        n = n * 10 + digit;
    }
    *value = n;
    return true;
}
static bool cui_dock_private_read_number(cui_dock_private_parser *p, double *out) {
    char token[64];
    if (!cui_dock_private_token(p, token))
        return false;
    size_t i = 0, digits = 0;
    int exponent = 0;
    while (token[i] >= '0' && token[i] <= '9') {
        i++;
        digits++;
    }
    if (token[i] == '.') {
        i++;
        while (token[i] >= '0' && token[i] <= '9') {
            i++;
            digits++;
        }
    }
    if (!digits || digits > 32) {
        p->error = CUI_DOCK_INVALID;
        return false;
    }
    if (token[i] == 'e' || token[i] == 'E') {
        i++;
        if (token[i] == '+' || token[i] == '-')
            i++;
        size_t start = i;
        while (token[i] >= '0' && token[i] <= '9') {
            if (exponent > 400) {
                p->error = CUI_DOCK_INVALID;
                return false;
            }
            exponent = exponent * 10 + (token[i++] - '0');
        }
        if (i == start || exponent > 400) {
            p->error = CUI_DOCK_INVALID;
            return false;
        }
    }
    if (token[i]) {
        p->error = CUI_DOCK_INVALID;
        return false;
    }
    /* Accumulating decimal digits and multiplying by a rounded power of ten
       can round twice when long double is only double (e.g. macOS ARM).
       Delegate conversion once to strtod after validating the ASCII grammar.
       Adapt only its radix to the current locale, without changing locale. */
    char localized[128];
    const char *dot = strchr(token, '.');
    if (dot) {
        const char *point = localeconv()->decimal_point;
        size_t point_length = 0;
        if (point)
            while (point_length < 32 && point[point_length])
                point_length++;
        if (!point_length || point_length == 32) {
            p->error = CUI_DOCK_INVALID;
            return false;
        }
        size_t prefix = (size_t)(dot - token), suffix = strlen(dot + 1);
        memcpy(localized, token, prefix);
        memcpy(localized + prefix, point, point_length);
        memcpy(localized + prefix + point_length, dot + 1, suffix + 1);
    } else {
        memcpy(localized, token, strlen(token) + 1);
    }
    char *end;
    *out = strtod(localized, &end);
    if (*end) {
        p->error = CUI_DOCK_INVALID;
        return false;
    }
    if (!isfinite(*out)) {
        p->error = CUI_DOCK_INVALID;
        return false;
    }
    return true;
}
static size_t cui_dock_private_read_node(cui_dock_private_parser *p, size_t parent, size_t depth) {
    cui_dock *d = p->dock;
    cui_dock_private_state *s = &d->scratch;
    if (depth > d->config.max_depth || p->generation == UINT32_MAX) {
        p->error = CUI_DOCK_CAPACITY;
        return SIZE_MAX;
    }
    size_t n = cui_dock_private_free_node(d, s);
    if (n == SIZE_MAX) {
        p->error = CUI_DOCK_CAPACITY;
        return SIZE_MAX;
    }
    char kind[64];
    if (!cui_dock_private_token(p, kind))
        return SIZE_MAX;
    if (strcmp(kind, "T") && strcmp(kind, "H") && strcmp(kind, "V")) {
        p->error = CUI_DOCK_INVALID;
        return SIZE_MAX;
    }
    cui_dock_private_init_node(s, n, ++p->generation, parent);
    cui_dock_private_node *node = &s->nodes[n];
    node->kind = kind[0] == 'T'   ? CUI_DOCK_TABS
                 : kind[0] == 'H' ? CUI_DOCK_SPLIT_HORIZONTAL
                                  : CUI_DOCK_SPLIT_VERTICAL;
    if (node->kind != CUI_DOCK_TABS &&
        (!cui_dock_private_read_number(p, &node->ratio) || !cui_dock_private_ratio(node->ratio))) {
        p->error = CUI_DOCK_INVALID;
        return SIZE_MAX;
    }
    if (!cui_dock_private_read_number(p, &node->minw) ||
        !cui_dock_private_read_number(p, &node->minh) || !cui_dock_private_dimension(node->minw) ||
        !cui_dock_private_dimension(node->minh)) {
        p->error = CUI_DOCK_INVALID;
        return SIZE_MAX;
    }
    if (node->kind == CUI_DOCK_TABS) {
        uint64_t active, count;
        if (!cui_dock_private_read_uint(p, &active) || !cui_dock_private_read_uint(p, &count))
            return SIZE_MAX;
        if (count > d->config.panel_capacity) {
            p->error = CUI_DOCK_CAPACITY;
            return SIZE_MAX;
        }
        if ((!count && (active || parent != SIZE_MAX)) || (count && active >= count)) {
            p->error = CUI_DOCK_INVALID;
            return SIZE_MAX;
        }
        for (size_t i = 0; i < (size_t)count; i++) {
            uint64_t key;
            if (!cui_dock_private_read_uint(p, &key))
                return SIZE_MAX;
            if (cui_dock_private_find_panel(d, s, key) != SIZE_MAX) {
                p->error = CUI_DOCK_DUPLICATE_PANEL;
                return SIZE_MAX;
            }
            size_t slot = cui_dock_private_free_panel(d, s);
            if (slot == SIZE_MAX) {
                p->error = CUI_DOCK_CAPACITY;
                return SIZE_MAX;
            }
            s->panels[slot] = (cui_dock_private_panel){.used = true, .key = key};
            cui_dock_private_attach(s, slot, n, i);
            if (i == (size_t)active)
                node->active = slot;
        }
    } else {
        node->a = cui_dock_private_read_node(p, n, depth + 1);
        if (node->a == SIZE_MAX)
            return SIZE_MAX;
        node->b = cui_dock_private_read_node(p, n, depth + 1);
        if (node->b == SIZE_MAX)
            return SIZE_MAX;
    }
    return n;
}
cui_dock_result cui_dock_restore(cui_dock *d, cui_str text) {
    if (!d || !text.data || !text.length)
        return CUI_DOCK_INVALID;
    if (text.length > d->config.max_serialized_bytes)
        return CUI_DOCK_CAPACITY;
    memset(d->scratch.nodes, 0, d->config.node_capacity * sizeof(*d->scratch.nodes));
    memset(d->scratch.panels, 0, d->config.panel_capacity * sizeof(*d->scratch.panels));
    cui_dock_private_parser p = {d, text, 0, d->generation, CUI_DOCK_OK};
    char magic[64];
    uint64_t version;
    if (!cui_dock_private_token(&p, magic) || strcmp(magic, "CUIDOCK") ||
        !cui_dock_private_read_uint(&p, &version) || version != 1)
        return CUI_DOCK_INVALID;
    size_t root = cui_dock_private_read_node(&p, SIZE_MAX, 1);
    if (root == SIZE_MAX)
        return p.error == CUI_DOCK_OK ? CUI_DOCK_INVALID : p.error;
    while (p.pos < text.length && cui_dock_private_space(text.data[p.pos]))
        p.pos++;
    if (p.pos != text.length)
        return CUI_DOCK_INVALID;
    d->scratch.root = root;
    cui_dock_private_state old = d->state;
    d->state = d->scratch;
    d->scratch = old;
    d->generation = p.generation;
    return CUI_DOCK_OK;
}
