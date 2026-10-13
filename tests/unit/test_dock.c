#include "cui/cui_dock.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static cui_str dock_test_string(const char *s) {
    return (cui_str){s, strlen(s)};
}
static cui_dock *make(void) {
    cui_dock *d = cui_dock_create((cui_dock_config){64, 128, 16, 65536});
    assert(d);
    return d;
}
static void near(float a, float b) {
    assert(fabsf(a - b) < 0.001f);
}
static void serialize(cui_dock *d, char *text, size_t cap) {
    size_t len;
    assert(cui_dock_serialize(d, text, cap, &len) == CUI_DOCK_OK);
    assert(strlen(text) == len);
}
static void test_nested_geometry_move(void) {
    cui_dock *d = make();
    cui_dock_node left = cui_dock_root(d), right, bottom;
    assert(cui_dock_insert(d, left, 0, 0) == CUI_DOCK_OK);
    assert(cui_dock_insert(d, left, 1, SIZE_MAX) == CUI_DOCK_OK);
    assert(cui_dock_activate(d, 1) == CUI_DOCK_OK);
    assert(cui_dock_reorder(d, 1, 0) == CUI_DOCK_OK);
    uint64_t key;
    assert(cui_dock_panel_at(d, left, 0, &key) && key == 1);
    cui_dock_node_info info;
    assert(cui_dock_node_get(d, left, &info) && info.active_panel == 1);
    assert(cui_dock_split(d, left, CUI_DOCK_SPLIT_HORIZONTAL, 0.25, 2, false, &right) ==
           CUI_DOCK_OK);
    assert(cui_dock_split(d, right, CUI_DOCK_SPLIT_VERTICAL, 0.5, 3, false, &bottom) ==
           CUI_DOCK_OK);
    assert(cui_dock_set_minimum(d, left, 100, 20) == CUI_DOCK_OK);
    assert(cui_dock_set_minimum(d, right, 50, 80) == CUI_DOCK_OK);
    assert(cui_dock_set_minimum(d, bottom, 50, 50) == CUI_DOCK_OK);
    cui_dock_geometry geo[8];
    size_t count;
    bool fit;
    assert(cui_dock_layout(d, (cui_rect){0, 0, 400, 200}, 4, geo, 8, &count, &fit) == CUI_DOCK_OK);
    assert(count == 3 && fit);
    near(geo[0].bounds.w, 100);
    near(geo[1].bounds.x, 104);
    near(geo[1].bounds.h, 98);
    near(geo[2].bounds.y, 102);
    near(geo[2].bounds.w, 296);
    assert(cui_dock_layout(d, (cui_rect){0, 0, 120, 100}, 4, geo, 8, &count, &fit) == CUI_DOCK_OK);
    assert(!fit);
    for (size_t i = 0; i < count; i++) {
        assert(geo[i].bounds.w >= 0 && geo[i].bounds.h >= 0);
        assert(geo[i].bounds.x + geo[i].bounds.w <= 120.001f);
        assert(geo[i].bounds.y + geo[i].bounds.h <= 100.001f);
    }
    cui_dock_node stale_root = cui_dock_root(d);
    assert(cui_dock_move(d, 3, right, SIZE_MAX) == CUI_DOCK_OK);
    assert(!cui_dock_node_get(d, bottom, &info));
    assert(cui_dock_remove(d, 2) == CUI_DOCK_OK);
    assert(cui_dock_move(d, 3, left, SIZE_MAX) == CUI_DOCK_OK);
    assert(cui_dock_root(d) == left);
    assert(!cui_dock_node_get(d, stale_root, &info));
    assert(cui_dock_node_get(d, left, &info) && info.panel_count == 3 && info.active_panel == 3);
    assert(cui_dock_insert(d, left, 1, 0) == CUI_DOCK_DUPLICATE_PANEL);
    assert(cui_dock_remove(d, 0) == CUI_DOCK_OK);
    assert(cui_dock_remove(d, 1) == CUI_DOCK_OK);
    assert(cui_dock_remove(d, 3) == CUI_DOCK_OK);
    assert(cui_dock_node_get(d, left, &info) && info.panel_count == 0 && !info.has_active);
    cui_dock_destroy(d);
}
static void test_serialization_rejection(void) {
    cui_dock *d = make();
    cui_dock_node root = cui_dock_root(d), second;
    assert(cui_dock_insert(d, root, UINT64_MAX, 0) == CUI_DOCK_OK);
    assert(cui_dock_split(d, root, CUI_DOCK_SPLIT_HORIZONTAL, 1.0 / 3.0, 0, false, &second) ==
           CUI_DOCK_OK);
    assert(cui_dock_set_minimum(d, second, 0.1, 12.5) == CUI_DOCK_OK);
    assert(cui_dock_set_minimum(d, root, -0.0, 0.0) == CUI_DOCK_OK);
    char before[8192], after[8192];
    serialize(d, before, sizeof(before));
    size_t len;
    char untouched[] = "untouched";
    assert(cui_dock_serialize(d, untouched, sizeof(untouched), &len) == CUI_DOCK_CAPACITY);
    assert(!strcmp(untouched, "untouched"));
    assert(cui_dock_restore(d, dock_test_string(before)) == CUI_DOCK_OK);
    assert(!cui_dock_node_get(d, root, &(cui_dock_node_info){0}));
    serialize(d, after, sizeof(after));
    assert(!strcmp(before, after));
    const char *bad[] = {"",
                         "CUIDOCK 3 T 0 0 0 0",
                         "CUIDOCK 1 T 0 0 0 2 7 7",
                         "CUIDOCK 1 T 0 0 2 1 7",
                         "CUIDOCK 1 T 0 0 0 18446744073709551616",
                         "CUIDOCK 1 H 0.5 0 0 T 0 0 0 1 1",
                         "CUIDOCK 1 H nan 0 0 T 0 0 0 1 1 T 0 0 0 1 2",
                         "CUIDOCK 1 H 1e999999999 0 0 T 0 0 0 1 1 T 0 0 0 1 2",
                         "CUIDOCK 1 T -1 0 0 1 7",
                         "CUIDOCK 1 T 0 0 0 1 7 garbage",
                         "CUIDOCK 1 H 0.5 0 0 T 0 0 0 0 T 0 0 0 1 2",
                         "CUIDOCK 1 T 0 0 0 18446744073709551615"};
    for (size_t i = 0; i < sizeof(bad) / sizeof(*bad); i++) {
        assert(cui_dock_restore(d, dock_test_string(bad[i])) != CUI_DOCK_OK);
        serialize(d, after, sizeof(after));
        assert(!strcmp(before, after));
    }
    assert(cui_dock_restore(d, (cui_str){"bad", SIZE_MAX}) == CUI_DOCK_CAPACITY);
    char nul[] = "CUIDOCK 1 T 0 0 0 0\0junk";
    assert(cui_dock_restore(d, (cui_str){nul, sizeof(nul) - 1}) != CUI_DOCK_OK);
    char deep[8192] = "CUIDOCK 1 ";
    for (int i = 0; i < 70; i++)
        strcat(deep, "H 0.5 0 0 ");
    strcat(deep, "T 0 0 0 1 1 ");
    for (int i = 0; i < 70; i++) {
        char temp[40];
        snprintf(temp, sizeof(temp), "T 0 0 0 1 %d ", i + 2);
        strcat(deep, temp);
    }
    assert(cui_dock_restore(d, dock_test_string(deep)) != CUI_DOCK_OK);
    serialize(d, after, sizeof(after));
    assert(!strcmp(before, after));
    cui_dock_destroy(d);
    assert(!cui_dock_create((cui_dock_config){.node_capacity = SIZE_MAX}));
    assert(!cui_dock_create((cui_dock_config){.panel_capacity = SIZE_MAX}));
    assert(!cui_dock_create((cui_dock_config){.max_depth = 65}));
    assert(!cui_dock_create((cui_dock_config){.node_capacity = CUI_DOCK_NODE_LIMIT + 1}));
    assert(!cui_dock_create((cui_dock_config){.panel_capacity = CUI_DOCK_PANEL_LIMIT + 1}));
    assert(!cui_dock_create((cui_dock_config){.max_serialized_bytes = SIZE_MAX}));
    d = cui_dock_create((cui_dock_config){1, 1, 1, 64});
    assert(d);
    root = cui_dock_root(d);
    assert(cui_dock_insert(d, root, 1, 0) == CUI_DOCK_OK);
    assert(cui_dock_insert(d, root, 2, 0) == CUI_DOCK_CAPACITY);
    assert(cui_dock_split(d, root, CUI_DOCK_SPLIT_VERTICAL, 0.5, 2, false, NULL) ==
           CUI_DOCK_CAPACITY);
    assert(cui_dock_restore(d, dock_test_string("CUIDOCK 1 H 0.5 0 0 T 0 0 0 1 1 T 0 0 0 1 2")) !=
           CUI_DOCK_OK);
    cui_dock_destroy(d);
}
static void test_decimal_precision_roundtrip(void) {
    const double ratios[] = {0.1,
                             0.2,
                             0.3,
                             1.0 / 3.0,
                             0.12345678901234567,
                             1e-200,
                             DBL_MIN,
                             DBL_TRUE_MIN,
                             nextafter(0.1, 0.0),
                             nextafter(0.1, 1.0),
                             nextafter(1.0, 0.0)};
    const double minima[] = {0.0,     0.1,          123.45678901234567, 1e-300,
                             DBL_MIN, DBL_TRUE_MIN, nextafter(1e9, 0.0)};
    cui_dock *d = make();
    cui_dock_node original = cui_dock_root(d), second;
    assert(cui_dock_insert(d, original, 1, 0) == CUI_DOCK_OK);
    assert(cui_dock_split(d, original, CUI_DOCK_SPLIT_HORIZONTAL, 0.5, 2, false, &second) ==
           CUI_DOCK_OK);
    for (size_t i = 0; i < sizeof(ratios) / sizeof(*ratios); i++) {
        assert(cui_dock_set_ratio(d, cui_dock_root(d), ratios[i]) == CUI_DOCK_OK);
        assert(cui_dock_panel_find(d, 2, &second, NULL));
        double minimum = minima[i % (sizeof(minima) / sizeof(*minima))];
        assert(cui_dock_set_minimum(d, second, minimum, minimum) == CUI_DOCK_OK);
        char before[8192], after[8192];
        serialize(d, before, sizeof(before));
        assert(cui_dock_restore(d, dock_test_string(before)) == CUI_DOCK_OK);
        serialize(d, after, sizeof(after));
        assert(!strcmp(before, after));
        cui_dock_node_info info;
        assert(cui_dock_node_get(d, cui_dock_root(d), &info));
        assert(info.ratio == ratios[i]);
        assert(cui_dock_panel_find(d, 2, &second, NULL));
        assert(cui_dock_node_get(d, second, &info));
        assert(info.min_width == minimum && info.min_height == minimum);
    }
    cui_dock_destroy(d);
}
static unsigned rng = 11;
static unsigned random_number(void) {
    rng = rng * 1664525u + 1013904223u;
    return rng;
}
static size_t audit(cui_dock *d, cui_dock_node id, cui_dock_node parent, bool seen[64],
                    size_t depth) {
    assert(depth <= 16);
    cui_dock_node_info n;
    assert(cui_dock_node_get(d, id, &n));
    assert(n.parent == parent);
    if (n.kind == CUI_DOCK_TABS) {
        assert(!n.first && !n.second);
        if (parent)
            assert(n.panel_count);
        bool active = false;
        for (size_t i = 0; i < n.panel_count; i++) {
            uint64_t key;
            assert(cui_dock_panel_at(d, id, i, &key));
            assert(key < 64 && !seen[key]);
            seen[key] = true;
            active |= key == n.active_panel;
            cui_dock_node found;
            size_t index;
            assert(cui_dock_panel_find(d, key, &found, &index) && found == id && index == i);
        }
        assert(n.has_active == (n.panel_count > 0));
        if (n.has_active)
            assert(active);
        return 1;
    }
    assert(n.first && n.second && n.first != n.second && n.ratio > 0 && n.ratio < 1);
    size_t count = 0;
    for (size_t i = 0; i < n.child_count; i++) {
        cui_dock_node child;
        cui_dock_slot_size size;
        assert(cui_dock_child_at(d, id, i, &child, &size));
        if (size.has_size)
            assert(isfinite(size.size) && size.size >= 0 && size.size <= 1e9);
        count += audit(d, child, id, seen, depth + 1);
    }
    return count;
}
static void test_random_mutations(bool nary) {
    cui_dock *d = nary ? cui_dock_create_nary((cui_dock_config){64, 128, 16, 65536}) : make();
    assert(d);
    bool present[64] = {0};
    char before[32768], after[32768];
    for (size_t step = 0; step < 7000; step++) {
        cui_dock_geometry geo[64];
        size_t count;
        assert(cui_dock_layout(d, (cui_rect){0, 0, 1000, 700}, 3, geo, 64, &count, NULL) ==
               CUI_DOCK_OK);
        size_t pick = random_number() % count;
        uint64_t key = random_number() % 64;
        cui_dock_result result;
        switch (random_number() % 7) {
        case 0:
            result = cui_dock_insert(d, geo[pick].node, key, SIZE_MAX);
            if (result == CUI_DOCK_OK)
                present[key] = true;
            else
                assert(result == CUI_DOCK_DUPLICATE_PANEL || result == CUI_DOCK_CAPACITY);
            break;
        case 1:
            result = cui_dock_remove(d, key);
            if (result == CUI_DOCK_OK)
                present[key] = false;
            else
                assert(!present[key] && result == CUI_DOCK_NOT_FOUND);
            break;
        case 2:
            result = cui_dock_move(d, key, geo[pick].node, SIZE_MAX);
            assert(result == (present[key] ? CUI_DOCK_OK : CUI_DOCK_NOT_FOUND));
            break;
        case 3:
            if (geo[pick].panel_count) {
                result = cui_dock_split(d, geo[pick].node,
                                        (random_number() & 1) ? CUI_DOCK_SPLIT_HORIZONTAL
                                                              : CUI_DOCK_SPLIT_VERTICAL,
                                        0.3, key, (random_number() & 1) != 0, NULL);
                if (result == CUI_DOCK_OK)
                    present[key] = true;
                else
                    assert(result == CUI_DOCK_DUPLICATE_PANEL || result == CUI_DOCK_CAPACITY);
            }
            break;
        case 4:
            result = cui_dock_activate(d, key);
            assert(result == (present[key] ? CUI_DOCK_OK : CUI_DOCK_NOT_FOUND));
            break;
        case 5:
            if (present[key]) {
                cui_dock_node tab;
                assert(cui_dock_panel_find(d, key, &tab, NULL));
                cui_dock_node_info n;
                assert(cui_dock_node_get(d, tab, &n));
                assert(cui_dock_reorder(d, key, random_number() % n.panel_count) == CUI_DOCK_OK);
            }
            break;
        default:
            assert(cui_dock_set_minimum(d, geo[pick].node, random_number() % 150,
                                        random_number() % 150) == CUI_DOCK_OK);
            break;
        }
        bool seen[64] = {0};
        size_t tabs = audit(d, cui_dock_root(d), 0, seen, 1);
        assert(tabs <= 64);
        for (size_t i = 0; i < 64; i++)
            assert(seen[i] == present[i]);
        if (step % 100 == 0) {
            serialize(d, before, sizeof(before));
            assert(cui_dock_restore(d, dock_test_string(before)) == CUI_DOCK_OK);
            serialize(d, after, sizeof(after));
            assert(!strcmp(before, after));
        }
    }
    serialize(d, before, sizeof(before));
    for (size_t i = 0; i < 2000; i++) {
        char garbage[256];
        size_t n = random_number() % sizeof(garbage);
        for (size_t j = 0; j < n; j++)
            garbage[j] = (char)(random_number() % 128);
        cui_dock_result r = cui_dock_restore(d, (cui_str){garbage, n});
        assert(r != CUI_DOCK_OK);
        serialize(d, after, sizeof(after));
        assert(!strcmp(before, after));
    }
    cui_dock_destroy(d);
}
static void test_split_layout(void) {
    cui_dock *d = make();
    cui_dock_node first = cui_dock_root(d), second;
    assert(cui_dock_insert(d, first, 10, 0) == CUI_DOCK_OK);
    assert(cui_dock_split(d, first, CUI_DOCK_SPLIT_HORIZONTAL, .25, 20, false, &second) ==
           CUI_DOCK_OK);
    assert(cui_dock_set_minimum(d, first, 100, 30) == CUI_DOCK_OK);
    assert(cui_dock_set_minimum(d, second, 50, 20) == CUI_DOCK_OK);
    cui_dock_split_geometry split;
    assert(cui_dock_split_layout(d, cui_dock_root(d), (cui_rect){7, 9, 400, 200}, 4, &split) ==
           CUI_DOCK_OK);
    cui_dock_geometry leaves[2];
    size_t count;
    bool fit;
    assert(cui_dock_layout(d, (cui_rect){7, 9, 400, 200}, 4, leaves, 2, &count, &fit) ==
           CUI_DOCK_OK);
    near(split.first.x, leaves[0].bounds.x);
    near(split.first.w, leaves[0].bounds.w);
    near(split.second.x, leaves[1].bounds.x);
    near(split.second.w, leaves[1].bounds.w);
    assert(split.fit_minimum && fit && count == 2);
    near(split.first_minimum.width, 100);
    near(split.second_minimum.width, 50);
    assert(cui_dock_split_layout(d, cui_dock_root(d), (cui_rect){0, 0, 120, 100}, 4, &split) ==
           CUI_DOCK_OK);
    assert(!split.fit_minimum);
    near(split.first.w + split.second.w + 4, 120);
    cui_dock_split_geometry before = split;
    assert(cui_dock_split_layout(d, first, (cui_rect){0, 0, 10, 10}, 0, &split) ==
           CUI_DOCK_INVALID);
    assert(!memcmp(&before, &split, sizeof split));
    assert(cui_dock_split_layout(d, cui_dock_root(d), (cui_rect){0, 0, NAN, 10}, 0, &split) ==
           CUI_DOCK_INVALID);
    assert(!memcmp(&before, &split, sizeof split));
    cui_dock_destroy(d);
}
static void test_move_split_transaction(void) {
    cui_dock *d = make();
    cui_dock_node left = cui_dock_root(d), right, moved = 999;
    assert(cui_dock_insert(d, left, 0, 0) == CUI_DOCK_OK);
    assert(cui_dock_insert(d, left, 1, 1) == CUI_DOCK_OK);
    assert(cui_dock_split(d, left, CUI_DOCK_SPLIT_HORIZONTAL, .5, 2, false, &right) == CUI_DOCK_OK);
    assert(cui_dock_set_minimum(d, left, 123, 45) == CUI_DOCK_OK);
    char before[8192], after[8192];
    serialize(d, before, sizeof before);
    assert(cui_dock_move_split(d, 1, right, CUI_DOCK_TABS, .5, false, &moved) == CUI_DOCK_INVALID);
    serialize(d, after, sizeof after);
    assert(!strcmp(before, after) && moved == 999);
    assert(cui_dock_move_split(d, 1, right, CUI_DOCK_SPLIT_VERTICAL, .5, true, &moved) ==
           CUI_DOCK_OK);
    cui_dock_node_info info;
    assert(cui_dock_node_get(d, moved, &info) && info.active_panel == 1);
    near((float)info.min_width, 123);
    near((float)info.min_height, 45);
    serialize(d, before, sizeof before);
    assert(cui_dock_move_split(d, 1, moved, CUI_DOCK_SPLIT_VERTICAL, .5, false, NULL) !=
           CUI_DOCK_OK);
    serialize(d, after, sizeof after);
    assert(!strcmp(before, after));
    assert(cui_dock_node_get(d, moved, &info)); /* Rejected transaction preserves generations. */
    cui_dock_destroy(d);
}
static void test_two_model_transfer(void) {
    cui_dock *a = make(),
             *b = cui_dock_create((cui_dock_config){.node_capacity = 3, .panel_capacity = 2});
    assert(b);
    cui_dock_node ar = cui_dock_root(a), br = cui_dock_root(b), second, out = 999;
    assert(cui_dock_insert(a, ar, 0, 0) == CUI_DOCK_OK);
    assert(cui_dock_insert(a, ar, 1, 1) == CUI_DOCK_OK);
    assert(cui_dock_insert(b, br, 2, 0) == CUI_DOCK_OK);
    assert(cui_dock_split(b, br, CUI_DOCK_SPLIT_HORIZONTAL, .5, 3, false, &second) == CUI_DOCK_OK);
    char sa[8192], sb[8192], after[8192];
    serialize(a, sa, sizeof sa);
    serialize(b, sb, sizeof sb);
    assert(cui_dock_transfer(a, 1, b, br, SIZE_MAX, CUI_DOCK_TABS, .5, false, &out) ==
           CUI_DOCK_CAPACITY);
    serialize(a, after, sizeof after);
    assert(!strcmp(sa, after));
    serialize(b, after, sizeof after);
    assert(!strcmp(sb, after) && out == 999);
    assert(cui_dock_node_get(a, ar, &(cui_dock_node_info){0}));
    assert(cui_dock_remove(b, 3) == CUI_DOCK_OK);
    assert(cui_dock_transfer(a, 1, b, br, SIZE_MAX, CUI_DOCK_SPLIT_VERTICAL, .5, true, &out) ==
           CUI_DOCK_OK);
    assert(!cui_dock_panel_find(a, 1, NULL, NULL) && cui_dock_panel_find(b, 1, NULL, NULL));
    assert(cui_dock_insert(a, ar, 1, SIZE_MAX) == CUI_DOCK_OK);
    serialize(a, sa, sizeof sa);
    serialize(b, sb, sizeof sb);
    assert(cui_dock_transfer(a, 1, b, out, SIZE_MAX, CUI_DOCK_TABS, .5, false, NULL) ==
           CUI_DOCK_DUPLICATE_PANEL);
    serialize(a, after, sizeof after);
    assert(!strcmp(sa, after));
    serialize(b, after, sizeof after);
    assert(!strcmp(sb, after));
    assert(cui_dock_transfer(a, 0, a, ar, SIZE_MAX, CUI_DOCK_TABS, .5, false, NULL) ==
           CUI_DOCK_INVALID);
    cui_dock_destroy(a);
    cui_dock_destroy(b);
}
static void test_bulk_remove(void) {
    cui_dock *d = make();
    cui_dock_node root = cui_dock_root(d);
    for (unsigned i = 0; i < 3; ++i)
        assert(cui_dock_insert(d, root, i, SIZE_MAX) == CUI_DOCK_OK);
    char before[4096], after[4096];
    serialize(d, before, sizeof before);
    uint64_t duplicate[] = {0, 0};
    assert(cui_dock_remove_many(d, duplicate, 2) == CUI_DOCK_NOT_FOUND);
    serialize(d, after, sizeof after);
    assert(!strcmp(before, after));
    uint64_t missing[] = {0, 99};
    assert(cui_dock_remove_many(d, missing, 2) == CUI_DOCK_NOT_FOUND);
    serialize(d, after, sizeof after);
    assert(!strcmp(before, after));
    uint64_t keys[] = {0, 2};
    assert(cui_dock_remove_many(d, keys, 2) == CUI_DOCK_OK);
    assert(!cui_dock_panel_find(d, 0, NULL, NULL) && !cui_dock_panel_find(d, 2, NULL, NULL) &&
           cui_dock_panel_find(d, 1, NULL, NULL));
    cui_dock_destroy(d);
}
static bool show_key(void *user, const cui_dock *model, uint64_t key) {
    assert(model);
    return key != *(const uint64_t *)user;
}
static bool show_none(void *user, const cui_dock *model, uint64_t key) {
    (void)user;
    (void)model;
    (void)key;
    return false;
}
static void test_visibility_queries(void) {
    cui_dock *model = make();
    cui_dock_node node = cui_dock_root(model);
    assert(cui_dock_insert(model, node, 0, 0) == CUI_DOCK_OK);
    assert(cui_dock_insert(model, node, UINT64_MAX, 1) == CUI_DOCK_OK);
    char before[4096], after[4096];
    serialize(model, before, sizeof before);
    uint64_t hidden = 0, displayed = 77;
    assert(cui_dock_visible_count(model, node, NULL, NULL) == 2);
    assert(cui_dock_visible_count(model, node, show_key, &hidden) == 1);
    assert(cui_dock_displayed_panel(model, node, show_key, &hidden, &displayed) &&
           displayed == UINT64_MAX);
    assert(!cui_dock_displayed_panel(model, node, show_none, NULL, &displayed) &&
           displayed == UINT64_MAX);
    assert(cui_dock_visible_count(model, node, show_none, NULL) == 0);
    serialize(model, after, sizeof after);
    assert(!strcmp(before, after));
    hidden = UINT64_MAX;
    assert(cui_dock_displayed_panel(model, node, show_key, &hidden, &displayed) && displayed == 0);
    assert(!cui_dock_displayed_panel(model, 0, show_key, &hidden, &displayed) && displayed == 0);
    cui_dock_destroy(model);
}
static void test_slot_sizing(void) {
    cui_dock *d = make();
    cui_dock_node a = cui_dock_root(d), b, c, bottom;
    assert(cui_dock_insert(d, a, 0, 0) == CUI_DOCK_OK);
    assert(cui_dock_split(d, a, CUI_DOCK_SPLIT_HORIZONTAL, .5, 1, false, &b) == CUI_DOCK_OK);
    assert(cui_dock_split(d, b, CUI_DOCK_SPLIT_HORIZONTAL, .5, 2, false, &c) == CUI_DOCK_OK);
    assert(cui_dock_split(d, c, CUI_DOCK_SPLIT_VERTICAL, .5, 3, false, &bottom) == CUI_DOCK_OK);
    cui_dock_node root = cui_dock_root(d), slots[3] = {99, 98, 97};
    size_t count = 0;
    assert(cui_dock_split_slots(d, root, NULL, 0, &count) == CUI_DOCK_OK && count == 3);
    assert(cui_dock_split_slots(d, root, slots, 2, &count) == CUI_DOCK_CAPACITY);
    assert(slots[0] == 99 && slots[2] == 97);
    assert(cui_dock_split_slots(d, root, slots, 3, &count) == CUI_DOCK_OK);
    assert(slots[0] == a && slots[1] == b);
    cui_dock_node_info info;
    assert(cui_dock_node_get(d, slots[2], &info) && info.kind == CUI_DOCK_SPLIT_VERTICAL);
    const double sizes[] = {100, 200, 300};
    assert(cui_dock_set_slot_sizes(d, root, sizes, 3, 4) == CUI_DOCK_OK);
    cui_dock_geometry geo[4];
    bool fit;
    assert(cui_dock_layout(d, (cui_rect){0, 0, 608, 100}, 4, geo, 4, &count, &fit) == CUI_DOCK_OK);
    assert(count == 4 && fit);
    near(geo[0].bounds.w, 100);
    near(geo[1].bounds.w, 200);
    near(geo[2].bounds.w, 300);
    near(geo[3].bounds.w, 300);
    near(geo[1].bounds.x, 104);
    near(geo[2].bounds.x, 308);
    char before[65536], after[65536];
    serialize(d, before, sizeof before);
    const double bad[] = {100, NAN, 300};
    assert(cui_dock_set_slot_sizes(d, root, bad, 3, 4) == CUI_DOCK_INVALID);
    assert(cui_dock_set_slot_sizes(d, root, sizes, 2, 4) == CUI_DOCK_INVALID);
    assert(cui_dock_set_slot_sizes(d, root, sizes, 3, -1) == CUI_DOCK_INVALID);
    const double extreme[] = {nextafter(0.0, 1.0), 1e9, 1e9};
    assert(cui_dock_set_slot_sizes(d, root, extreme, 3, 4) == CUI_DOCK_INVALID);
    assert(cui_dock_set_slot_sizes(d, a, sizes, 3, 4) == CUI_DOCK_INVALID);
    serialize(d, after, sizeof after);
    assert(!strcmp(before, after));
    assert(cui_dock_restore(d, dock_test_string(before)) == CUI_DOCK_OK);
    root = cui_dock_root(d);
    serialize(d, after, sizeof after);
    assert(!strcmp(before, after));
    assert(cui_dock_layout(d, (cui_rect){0, 0, 608, 100}, 4, geo, 4, &count, &fit) == CUI_DOCK_OK);
    near(geo[0].bounds.w, 100);
    near(geo[1].bounds.w, 200);
    near(geo[2].bounds.w, 300);
    assert(cui_dock_set_minimum(d, geo[0].node, 150, 0) == CUI_DOCK_OK);
    assert(cui_dock_layout(d, (cui_rect){0, 0, 608, 100}, 4, geo, 4, &count, &fit) == CUI_DOCK_OK);
    near(geo[0].bounds.w, 150);
    cui_dock_destroy(d);
}
static bool visible_mask(void *user, const cui_dock *model, uint64_t key) {
    (void)model;
    return key < 64 && !(*(uint64_t *)user & (UINT64_C(1) << key));
}
static void test_hidden_geometry(void) {
    cui_dock *d = make();
    cui_dock_node a = cui_dock_root(d), b, c, extra;
    assert(cui_dock_insert(d, a, 0, 0) == CUI_DOCK_OK);
    assert(cui_dock_split(d, a, CUI_DOCK_SPLIT_HORIZONTAL, .25, 1, false, &b) == CUI_DOCK_OK);
    assert(cui_dock_split(d, b, CUI_DOCK_SPLIT_VERTICAL, .3, 2, false, &c) == CUI_DOCK_OK);
    assert(cui_dock_split(d, a, CUI_DOCK_SPLIT_HORIZONTAL, .6, 3, false, &extra) == CUI_DOCK_OK);
    assert(cui_dock_set_minimum(d, a, 1000, 1000) == CUI_DOCK_OK);
    assert(cui_dock_set_minimum(d, extra, 1000, 1000) == CUI_DOCK_OK);
    char before[65536], after[65536];
    serialize(d, before, sizeof before);
    cui_rect bounds = {10, 20, 400, 200};
    cui_dock_geometry geo[4];
    size_t count;
    bool fit;
    uint64_t hidden = 9;
    assert(cui_dock_layout_visible(d, bounds, 4, visible_mask, &hidden, geo, 4, &count, &fit) ==
           CUI_DOCK_OK);
    assert(count == 2 && fit && geo[0].node == b && geo[1].node == c);
    near(geo[0].bounds.x, 10);
    near(geo[0].bounds.w, 400);
    near(geo[0].bounds.h, 58.8f);
    near(geo[1].bounds.y, 82.8f);
    cui_dock_visible_split_geometry split;
    assert(cui_dock_split_layout_visible(d, cui_dock_root(d), bounds, 4, visible_mask, &hidden,
                                         &split) == CUI_DOCK_OK);
    assert(!split.first_visible && split.second_visible && split.geometry.fit_minimum);
    near(split.geometry.second.w, 400);
    hidden = 11;
    assert(cui_dock_layout_visible(d, bounds, 4, visible_mask, &hidden, geo, 4, &count, &fit) ==
           CUI_DOCK_OK);
    assert(count == 1 && fit && geo[0].node == c && geo[0].active_panel == 2 &&
           geo[0].panel_count == 1);
    near(geo[0].bounds.x, 10);
    near(geo[0].bounds.y, 20);
    near(geo[0].bounds.w, 400);
    near(geo[0].bounds.h, 200);
    hidden = 15;
    geo[0].node = 777;
    assert(cui_dock_layout_visible(d, bounds, 4, visible_mask, &hidden, geo, 4, &count, &fit) ==
           CUI_DOCK_OK);
    assert(count == 0 && fit && geo[0].node == 777);
    hidden = 0;
    assert(cui_dock_layout_visible(d, bounds, 4, visible_mask, &hidden, NULL, 0, &count, &fit) ==
           CUI_DOCK_OK);
    assert(count == 4 && !fit);
    assert(cui_dock_layout_visible(d, bounds, 4, visible_mask, &hidden, geo, 3, &count, &fit) ==
           CUI_DOCK_CAPACITY);
    assert(geo[0].node == 777);
    cui_dock_geometry legacy[4];
    size_t legacy_count;
    assert(cui_dock_layout(d, bounds, 4, legacy, 4, &legacy_count, NULL) == CUI_DOCK_OK);
    assert(cui_dock_layout_visible(d, bounds, 4, NULL, NULL, geo, 4, &count, &fit) == CUI_DOCK_OK);
    for (size_t i = 0; i < 4; i++) {
        assert(geo[i].node == legacy[i].node);
        near(geo[i].bounds.x, legacy[i].bounds.x);
        near(geo[i].bounds.y, legacy[i].bounds.y);
        near(geo[i].bounds.w, legacy[i].bounds.w);
        near(geo[i].bounds.h, legacy[i].bounds.h);
    }
    serialize(d, after, sizeof after);
    assert(!strcmp(before, after));
    cui_dock_destroy(d);
}
static void test_nary_model(void) {
    cui_dock *d = cui_dock_create_nary((cui_dock_config){0});
    cui_dock_node a = cui_dock_root(d), b, c, bottom;
    assert(cui_dock_insert(d, a, 0, 0) == CUI_DOCK_OK);
    assert(cui_dock_split(d, a, CUI_DOCK_SPLIT_HORIZONTAL, .25, 1, false, &b) == CUI_DOCK_OK);
    cui_dock_node root = cui_dock_root(d);
    assert(cui_dock_split(d, b, CUI_DOCK_SPLIT_HORIZONTAL, 1.0 / 3, 2, false, &c) == CUI_DOCK_OK);
    assert(cui_dock_root(d) == root);
    cui_dock_node_info info;
    assert(cui_dock_node_get(d, root, &info) && info.child_count == 3 && info.nary);
    assert(info.first == a && info.second == b);
    cui_dock_node child;
    cui_dock_slot_size size;
    assert(cui_dock_child_at(d, root, 2, &child, &size) && child == c && size.has_size);
    assert(!cui_dock_child_at(d, root, 3, &child, &size) && child == c);
    cui_dock_slot_size sizes[3] = {{true, 100}, {false, 0}, {true, 200}};
    assert(cui_dock_set_sizes(d, root, sizes, 3) == CUI_DOCK_OK);
    cui_dock_child_geometry geo[3] = {{.node = 777}};
    size_t count;
    bool fit;
    assert(cui_dock_children_layout(d, root, (cui_rect){0, 0, 400, 200}, 4, NULL, NULL, geo, 2,
                                    &count, &fit) == CUI_DOCK_CAPACITY);
    assert(count == 3 && geo[0].node == 777);
    assert(cui_dock_children_layout(d, root, (cui_rect){0, 0, 400, 200}, 4, NULL, NULL, geo, 3,
                                    &count, &fit) == CUI_DOCK_OK);
    near(geo[0].bounds.w, 100);
    near(geo[1].bounds.w, 92);
    near(geo[2].bounds.w, 200);
    near(geo[2].bounds.x, 200);
    assert(fit);
    cui_dock_slot_size zeros[3] = {{true, 0}, {true, 0}, {true, 0}};
    assert(cui_dock_set_sizes(d, root, zeros, 3) == CUI_DOCK_OK);
    assert(cui_dock_children_layout(d, root, (cui_rect){0, 0, 300, 100}, 0, NULL, NULL, geo, 3,
                                    &count, &fit) == CUI_DOCK_OK);
    for (size_t i = 0; i < 3; i++)
        near(geo[i].bounds.w, 100);
    assert(cui_dock_set_minimum(d, a, 100, 0) == CUI_DOCK_OK);
    assert(cui_dock_set_minimum(d, b, 100, 0) == CUI_DOCK_OK);
    assert(cui_dock_set_minimum(d, c, 100, 0) == CUI_DOCK_OK);
    assert(cui_dock_children_layout(d, root, (cui_rect){0, 0, 150, 100}, 0, NULL, NULL, geo, 3,
                                    &count, &fit) == CUI_DOCK_OK &&
           !fit);
    for (size_t i = 0; i < 3; i++)
        near(geo[i].bounds.w, 50);
    assert(cui_dock_set_minimum(d, a, 0, 0) == CUI_DOCK_OK);
    assert(cui_dock_set_minimum(d, b, 0, 0) == CUI_DOCK_OK);
    assert(cui_dock_set_minimum(d, c, 0, 0) == CUI_DOCK_OK);
    assert(cui_dock_set_sizes(d, root, sizes, 3) == CUI_DOCK_OK);
    char before[65536], after[65536];
    serialize(d, before, sizeof before);
    assert(!strncmp(before, "CUIDOCK 2 ", 10));
    assert(cui_dock_set_ratio(d, root, .7) == CUI_DOCK_INVALID);
    cui_dock_split_geometry untouched = {.first = {.w = 777}};
    assert(cui_dock_split_layout(d, root, (cui_rect){0, 0, 400, 200}, 4, &untouched) ==
               CUI_DOCK_INVALID &&
           untouched.first.w == 777);
    sizes[1] = (cui_dock_slot_size){true, NAN};
    assert(cui_dock_set_sizes(d, root, sizes, 3) == CUI_DOCK_INVALID);
    assert(cui_dock_set_sizes(d, root, sizes, 2) == CUI_DOCK_INVALID);
    serialize(d, after, sizeof after);
    assert(!strcmp(before, after));
    assert(cui_dock_restore(d, dock_test_string(before)) == CUI_DOCK_OK);
    serialize(d, after, sizeof after);
    assert(!strcmp(before, after));
    root = cui_dock_root(d);
    assert(cui_dock_child_at(d, root, 2, &c, NULL));
    assert(cui_dock_split(d, c, CUI_DOCK_SPLIT_VERTICAL, .5, 3, false, &bottom) == CUI_DOCK_OK);
    assert(cui_dock_node_get(d, root, &info) && info.child_count == 3 && cui_dock_root(d) == root);
    assert(cui_dock_panel_find(d, 0, &a, NULL));
    assert(cui_dock_move(d, 3, a, 1) == CUI_DOCK_OK);
    assert(cui_dock_child_at(d, root, 2, &child, &size) && child == c && size.has_size &&
           size.size == 200);
    assert(cui_dock_remove(d, 1) == CUI_DOCK_OK);
    assert(cui_dock_node_get(d, root, &info) && info.child_count == 2);
    assert(cui_dock_remove(d, 2) == CUI_DOCK_OK && cui_dock_root(d) == a);
    cui_dock_destroy(d);
}
static void test_nary_normalization_capacity(void) {
    cui_dock *d = cui_dock_create_nary((cui_dock_config){4, 3, 2, 65536});
    cui_dock_node a = cui_dock_root(d), b, c;
    assert(cui_dock_insert(d, a, 0, 0) == CUI_DOCK_OK);
    assert(cui_dock_split(d, a, CUI_DOCK_SPLIT_HORIZONTAL, .5, 1, false, &b) == CUI_DOCK_OK);
    cui_dock_node root = cui_dock_root(d);
    assert(cui_dock_split(d, b, CUI_DOCK_SPLIT_HORIZONTAL, .5, 2, false, &c) == CUI_DOCK_OK);
    char before[65536], after[65536];
    serialize(d, before, sizeof before);
    cui_dock_node untouched = 777;
    assert(cui_dock_split(d, c, CUI_DOCK_SPLIT_HORIZONTAL, .5, 3, false, &untouched) ==
               CUI_DOCK_CAPACITY &&
           untouched == 777);
    serialize(d, after, sizeof after);
    assert(!strcmp(before, after));
    cui_dock *source = make();
    assert(cui_dock_insert(source, cui_dock_root(source), 42, 0) == CUI_DOCK_OK);
    char source_before[65536], source_after[65536];
    serialize(source, source_before, sizeof source_before);
    assert(cui_dock_transfer(source, 42, d, a, 0, CUI_DOCK_TABS, .5, false, &untouched) ==
               CUI_DOCK_CAPACITY &&
           untouched == 777);
    serialize(source, source_after, sizeof source_after);
    serialize(d, after, sizeof after);
    assert(!strcmp(before, after) && !strcmp(source_before, source_after));
    assert(cui_dock_balance(d, root) == CUI_DOCK_OK);
    cui_dock_geometry geo[3];
    size_t count;
    assert(cui_dock_layout(d, (cui_rect){0, 0, 300, 100}, 0, geo, 3, &count, NULL) == CUI_DOCK_OK &&
           count == 3);
    for (size_t i = 0; i < 3; i++)
        near(geo[i].bounds.w, 100);
    cui_dock_destroy(source);
    cui_dock_destroy(d);
    d = make();
    const char *nested =
        "CUIDOCK 2 X 0 0 2 1 300 X 0 0 2 1 100 T 0 0 0 1 0 1 200 T 0 0 0 1 1 0 T 0 0 0 1 2 \n";
    assert(cui_dock_restore(d, dock_test_string(nested)) == CUI_DOCK_OK);
    root = cui_dock_root(d);
    assert(cui_dock_child_at(d, root, 0, &a, NULL));
    cui_dock_slot_size sizes[2] = {{true, 600}, {false, 0}};
    assert(cui_dock_set_sizes(d, root, sizes, 2) == CUI_DOCK_OK);
    cui_dock_node_info info;
    assert(cui_dock_node_get(d, root, &info) && info.child_count == 3);
    assert(!cui_dock_node_get(d, a, &info));
    cui_dock_slot_size size;
    assert(cui_dock_child_at(d, root, 0, &a, &size) && size.has_size && size.size == 200);
    assert(cui_dock_child_at(d, root, 1, &b, &size) && size.has_size && size.size == 400);
    assert(cui_dock_child_at(d, root, 2, &c, &size) && !size.has_size);
    const char *partial =
        "CUIDOCK 2 X 0 0 2 1 300 X 0 0 2 1 100 T 0 0 0 1 0 0 T 0 0 0 1 1 0 T 0 0 0 1 2 \n";
    assert(cui_dock_restore(d, dock_test_string(partial)) == CUI_DOCK_OK);
    root = cui_dock_root(d);
    assert(cui_dock_set_sizes(d, root, sizes, 2) == CUI_DOCK_OK);
    assert(cui_dock_child_at(d, root, 0, &a, &size) && size.has_size && size.size == 100);
    assert(cui_dock_child_at(d, root, 1, &b, &size) && !size.has_size);
    serialize(d, before, sizeof before);
    const char *bad[] = {"CUIDOCK 2 X 0 0 1 0 T 0 0 0 1 0",
                         "CUIDOCK 2 X 0 0 2 2 T 0 0 0 1 0 0 T 0 0 0 1 1",
                         "CUIDOCK 2 X 0 0 2 1 -1 T 0 0 0 1 0 0 T 0 0 0 1 1",
                         "CUIDOCK 1 X 0 0 2 0 T 0 0 0 1 0 0 T 0 0 0 1 1",
                         "CUIDOCK 2 X 0 0 2 0 T 0 0 0 1 0 0 T 0 0 0 1 0"};
    for (size_t i = 0; i < sizeof bad / sizeof *bad; i++) {
        assert(cui_dock_restore(d, dock_test_string(bad[i])) != CUI_DOCK_OK);
        serialize(d, after, sizeof after);
        assert(!strcmp(before, after));
    }
    const char *legacy = "CUIDOCK 1 H 0.25 0 0 T 0 0 0 1 0 T 0 0 0 1 1 \n";
    assert(cui_dock_restore(d, dock_test_string(legacy)) == CUI_DOCK_OK);
    serialize(d, after, sizeof after);
    assert(!strcmp(legacy, after));
    cui_dock_destroy(d);
}
int main(void) {
    test_nary_normalization_capacity();
    test_nary_model();
    test_hidden_geometry();
    test_slot_sizing();
    test_visibility_queries();
    cui_dock_group_constraints sealed = cui_dock_group_sealed();
    assert(sealed.locked && sealed.unclosable && sealed.preserve_last && !sealed.collapsed);
    cui_dock_group_constraints split = cui_dock_group_in_split(false);
    assert(!split.locked && !split.unclosable && !split.preserve_last && !split.collapsed);
    assert(cui_dock_group_in_split(true).preserve_last);
    test_bulk_remove();
    test_two_model_transfer();
    test_move_split_transaction();
    test_split_layout();
    test_nested_geometry_move();
    test_serialization_rejection();
    test_decimal_precision_roundtrip();
    test_random_mutations(false);
    test_random_mutations(true);
    puts("dock tests passed");
    return 0;
}
