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
                         "CUIDOCK 2 T 0 0 0 0",
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
    return audit(d, n.first, id, seen, depth + 1) + audit(d, n.second, id, seen, depth + 1);
}
static void test_random_mutations(void) {
    cui_dock *d = make();
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
int main(void) {
    test_move_split_transaction();
    test_split_layout();
    test_nested_geometry_move();
    test_serialization_rejection();
    test_decimal_precision_roundtrip();
    test_random_mutations();
    puts("dock tests passed");
    return 0;
}
