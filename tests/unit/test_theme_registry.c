/* SPDX-License-Identifier: MIT */
#include "cui/cui_theme_registry.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static bool color(cui_color a, cui_color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}
static cui_str string(const char *s) {
    return (cui_str){s, strlen(s)};
}
static void colors(void) {
    const char *families[] = {"slate",  "gray",    "zinc",   "neutral", "stone",  "red",
                              "orange", "amber",   "yellow", "lime",    "green",  "emerald",
                              "teal",   "cyan",    "sky",    "blue",    "indigo", "violet",
                              "purple", "fuchsia", "pink",   "rose"};
    const unsigned scales[] = {50, 100, 200, 300, 400, 500, 600, 700, 800, 900, 950};
    for (size_t i = 0; i < 22; i++)
        for (size_t k = 0; k < 11; k++) {
            char input[64];
            cui_color result;
            snprintf(input, sizeof input, "%s-%u/25", families[i], scales[k]);
            assert(cui_theme_color_parse(string(input), &result) && result.a == 64);
        }

    cui_color c = {1, 2, 3, 4}, old = c;
    assert(cui_theme_color_parse(string("#123"), &c) && color(c, (cui_color){17, 34, 51, 255}));
    assert(cui_theme_color_parse(string("#1234"), &c) && c.a == 68);
    assert(cui_theme_color_parse(string("#12aBfF80"), &c) &&
           color(c, (cui_color){18, 171, 255, 128}));
    assert(cui_theme_color_parse(string("neutral-200/60"), &c) &&
           color(c, (cui_color){229, 229, 229, 153}));
    assert(cui_theme_color_parse(string("blue"), &c) && color(c, (cui_color){59, 130, 246, 255}));
    assert(cui_theme_color_parse(string("#123\xc2\xa0"), &c) && c.r == 17);
    assert(!cui_theme_color_parse((cui_str){"white\0suffix", 12}, &c));
    const char *bad[] = {"#12",     "#GGG",    "#123456789", "blue-42", "blue/101",
                         "blue/-1", "red/NaN", "rgb(1,2,3)", "unknown", "neutral-200/50/50"};
    c = old;
    for (size_t i = 0; i < sizeof bad / sizeof *bad; i++) {
        assert(!cui_theme_color_parse(string(bad[i]), &c));
        assert(color(c, old));
    }
}
static void semantic(void) {
    cui_theme base = cui_theme_light(), theme = base;
    cui_theme_semantic_tokens tokens;
    assert(cui_theme_semantic_snapshot(&base, &tokens));
    cui_theme_import_result r;
    const char *json = "{\"tokens\":{\"colors\":{\"primary\":\"#1234\",\"surface\":\"blue-600/"
                       "50\",\"muted\":\"#fff\"},\"radius\":{\"md\":9,\"lg\":12,\"full\":9999},"
                       "\"spacing\":{\"xxs\":1,\"xxl\":40},\"typography\":{\"sans\":\"Portable "
                       "\\u03b1\",\"mono\":\"Mono\",\"md\":{\"size\":20,\"line_height\":30,"
                       "\"weight\":550},\"mono_md\":{\"size\":15}},\"shadow\":{\"sm\":[{\"color\":{"
                       "\"h\":0,\"s\":1,\"l\":0.5,\"a\":0.5},\"offset\":{\"x\":-2,\"y\":3},\"blur_"
                       "radius\":4,\"spread_radius\":-1,\"inset\":true}],\"lg\":[]}}}";
    char *owned = malloc(strlen(json) + 1);
    assert(owned);
    strcpy(owned, json);
    assert(cui_theme_semantic_import(string(owned), &base, &tokens, &theme, &r) ==
           CUI_THEME_REGISTRY_OK);
    memset(owned, '!', strlen(owned));
    free(owned);
    assert(color(theme.primary, (cui_color){17, 34, 51, 68}) && theme.font_size == 20 &&
           theme.radius == 9 && theme.radius_lg == 12);
    assert(!strcmp(tokens.typography.sans, "Portable \xce\xb1") &&
           tokens.typography.mono_md.size == 15);
    assert(tokens.shadow.sm.count == 1 && tokens.shadow.sm.values[0].color.r == 255 &&
           tokens.shadow.sm.values[0].color.a == 128 && tokens.shadow.sm.values[0].inset);
    assert(!memcmp(theme.buttons, base.buttons, sizeof base.buttons) &&
           color(theme.selection, base.selection));
    assert(tokens.radius.full == 9999 && tokens.spacing.xxl == 40);
    cui_theme saved = theme;
    cui_theme_semantic_tokens snapshot = tokens;
    const char *bad[] = {"{\"tokens\":{\"colors\":{\"primary\":\"nope\"}}}",
                         "{\"tokens\":{\"colors\":{\"selection\":\"#fff\"}}}",
                         "{\"tokens\":{\"typography\":{\"md\":{\"weight\":1001}}}}",
                         "{\"tokens\":{\"radius\":{\"md\":-1}}}",
                         "{\"tokens\":{\"colors\":null}}",
                         "{\"tokens\":{\"shadow\":false}}",
                         "{\"tokens\":{},\"themes\":[]}",
                         "{\"tokens\":{},\"tokens\":{}}",
                         "{\"tokens\":{\"typography\":{\"sans\":\"\\ud800\"}}}",
                         "{\"tokens\":{\"radius\":{\"md\":1e999}}}",
                         "{\"tokens\":{\"radius\":{\"md\":01}}}"};
    for (size_t i = 0; i < sizeof bad / sizeof *bad; i++) {
        assert(cui_theme_semantic_import(string(bad[i]), &base, &tokens, &theme, &r) !=
               CUI_THEME_REGISTRY_OK);
        assert(!memcmp(&theme, &saved, sizeof theme) && !memcmp(&tokens, &snapshot, sizeof tokens));
    }
    char long_font[400];
    memset(long_font, 'x', sizeof long_font);
    long_font[sizeof long_font - 1] = 0;
    char input[600];
    snprintf(input, sizeof input, "{\"tokens\":{\"typography\":{\"sans\":\"%s\"}}}", long_font);
    assert(cui_theme_semantic_import(string(input), &base, &tokens, &theme, &r) ==
           CUI_THEME_REGISTRY_CAPACITY_ERROR);
    assert(!memcmp(&theme, &saved, sizeof theme));
    assert(cui_theme_semantic_import(string("{}"), &base, &tokens, &theme, &r) ==
           CUI_THEME_REGISTRY_OK);
    assert(color(theme.primary, base.primary));
}
static void bounds(void) {
    cui_theme base = cui_theme_light(), theme = base;
    cui_theme_semantic_tokens tokens;
    assert(cui_theme_semantic_snapshot(&base, &tokens));
    assert(tokens.shadow.sm.count == 1 && tokens.shadow.md.values[0].blur_radius == 8 &&
           tokens.shadow.lg.values[0].spread_radius == -4);
    cui_theme_semantic_tokens saved = tokens;
    cui_theme_import_result report;
    char input[4096];
    strcpy(input, "{\"tokens\":{\"shadow\":{\"sm\":[");
    const char *shadow = "{\"color\":{\"h\":0,\"s\":0,\"l\":0,\"a\":1},\"offset\":{\"x\":0,\"y\":1}"
                         ",\"blur_radius\":2,\"spread_radius\":0,\"inset\":false}";
    for (unsigned i = 0; i < 9; i++) {
        if (i)
            strcat(input, ",");
        strcat(input, shadow);
    }
    strcat(input, "]}}}");
    assert(cui_theme_semantic_import(string(input), &base, &tokens, &theme, &report) ==
           CUI_THEME_REGISTRY_CAPACITY_ERROR);
    assert(!memcmp(&saved, &tokens, sizeof saved));
    char depth[128];
    size_t n = 0;
    for (unsigned i = 0; i < 18; i++)
        depth[n++] = '[';
    for (unsigned i = 0; i < 18; i++)
        depth[n++] = ']';
    depth[n] = 0;
    assert(cui_theme_semantic_import(string(depth), &base, &tokens, &theme, &report) ==
           CUI_THEME_REGISTRY_CAPACITY_ERROR);
    /* Maximum permitted nesting must still validate escapes. The parser's
     * workspace is shared rather than multiplied by each recursive frame. */
    n = 0;
    for (unsigned i = 0; i < 16; i++)
        depth[n++] = '[';
    memcpy(depth + n, "\"\\uD83D\\uDE00\"", 14);
    n += 14;
    for (unsigned i = 0; i < 16; i++)
        depth[n++] = ']';
    depth[n] = 0;
    assert(cui_theme_semantic_import(string(depth), &base, &tokens, &theme, &report) ==
           CUI_THEME_REGISTRY_INVALID); /* Valid JSON, unsupported root shape. */
    assert(!memcmp(&saved, &tokens, sizeof saved));
    memcpy(depth + 16, "\"\\uD83D\\uDE0X\"", 14);
    assert(cui_theme_semantic_import(string(depth), &base, &tokens, &theme, &report) ==
               CUI_THEME_REGISTRY_INVALID &&
           report.byte_offset > 16);
    assert(!memcmp(&saved, &tokens, sizeof saved));
    char *huge = malloc(CUI_THEME_JSON_MAX_BYTES + 1);
    assert(huge);
    memset(huge, ' ', CUI_THEME_JSON_MAX_BYTES + 1);
    assert(cui_theme_semantic_import((cui_str){huge, CUI_THEME_JSON_MAX_BYTES + 1}, &base, &tokens,
                                     &theme, &report) == CUI_THEME_REGISTRY_CAPACITY_ERROR);
    free(huge);
    huge = malloc(20000);
    assert(huge);
    strcpy(huge, "{\"tokens\":{\"shadow\":{\"sm\":[");
    for (unsigned i = 0; i < 4100; i++) {
        if (i)
            strcat(huge, ",");
        strcat(huge, "{}");
    }
    strcat(huge, "]}}}");
    assert(cui_theme_semantic_import(string(huge), &base, &tokens, &theme, &report) ==
           CUI_THEME_REGISTRY_CAPACITY_ERROR);
    assert(!memcmp(&saved, &tokens, sizeof saved));
    free(huge);
    const char *original = "{\"tokens\":{\"colors\":{\"primary\":\"#123456\"},\"typography\":{"
                           "\"sans\":\"\\uD83D\\uDE00\"}}}";
    assert(cui_theme_semantic_import(string(original), &base, &tokens, &theme, &report) == 0 &&
           !strcmp(tokens.typography.sans, "\xf0\x9f\x98\x80"));
    assert(cui_theme_semantic_import(string("{\"tokens\":{\"radius\":{\"md\":1,\"\\u006dd\":2}}}"),
                                     &base, &tokens, &theme,
                                     &report) == CUI_THEME_REGISTRY_INVALID);
    /* Structured byte mutations exercise escaping, object grammar and UTF8;
     * every rejected mutation preserves both caller output objects. */
    for (size_t at = 0; at < strlen(original); at++) {
        strcpy(input, original);
        input[at] = (char)(at % 3 == 0 ? 0xff : at % 3 == 1 ? '\\' : ':');
        cui_theme old = theme;
        saved = tokens;
        cui_theme_registry_error error =
            cui_theme_semantic_import(string(input), &base, &tokens, &theme, &report);
        if (error) {
            assert(!memcmp(&old, &theme, sizeof old) && !memcmp(&saved, &tokens, sizeof saved));
        }
    }
}
static cui_theme_registry_entry entry(const char *name, bool dark) {
    cui_theme_registry_entry e = {0};
    strcpy(e.name, name);
    e.theme = dark ? cui_theme_dark() : cui_theme_light();
    assert(cui_theme_semantic_snapshot(&e.theme, &e.semantic));
    e.motion = cui_theme_motion_default();
    return e;
}
static void registry(void) {
    cui_theme_registry_entry *storage = calloc(64, sizeof *storage),
                             *scratch = calloc(64, sizeof *scratch),
                             *saved = calloc(64, sizeof *saved);
    assert(storage && scratch && saved);
    cui_theme_registry r = {0};
    assert(cui_theme_registry_init(&r, storage, 64, true));
    assert(r.count == 38);
    for (size_t i = 0; i < 38; i++) {
        cui_theme expected;
        assert(cui_theme_preset_get(i, &expected));
        const cui_theme_registry_entry *e = cui_theme_registry_find(&r, cui_theme_preset_name(i));
        assert(e);
        assert(color(e->theme.background, expected.background) &&
               color(e->theme.primary, expected.primary) &&
               e->theme.font_size == expected.font_size);
        assert(!memcmp(e->theme.buttons, expected.buttons, sizeof expected.buttons));
    }
    assert(cui_theme_registry_sorted(&r, 0)->is_default &&
           cui_theme_registry_sorted(&r, 1)->is_default);
    cui_theme_registry_entry custom = entry("custom", false);
    assert(cui_theme_registry_put(&r, &custom, false) == 0);
    cui_theme_import_result empty_report;
    assert(cui_theme_registry_import(&r, string("{\"name\":\"Empty\"}"),
                                     (cui_theme_import_options){0}, scratch, 64,
                                     &empty_report) == 0 &&
           empty_report.added == 0);
    uint64_t revision = r.revision;
    custom.theme.primary = (cui_color){1, 2, 3, 255};
    assert(cui_theme_registry_put(&r, &custom, false) == 0 && r.revision == revision);
    assert(cui_theme_registry_put(&r, &custom, true) == 0 && r.revision == revision + 1);
    const cui_theme_registry_entry *normalized = cui_theme_registry_find(&r, string("custom"));
    assert(color(normalized->semantic.colors.primary, custom.theme.primary));
    custom.semantic.spacing.xl = 37;
    strcpy(custom.semantic.typography.sans, "Retained");
    custom.semantic.colors.selection = (cui_color){7, 8, 9, 255};
    assert(cui_theme_registry_put(&r, &custom, true) == 0);
    normalized = cui_theme_registry_find(&r, string("custom"));
    assert(normalized->semantic.spacing.xl == 37 &&
           !strcmp(normalized->semantic.typography.sans, "Retained") &&
           color(normalized->semantic.colors.selection, custom.semantic.colors.selection));
    cui_theme_host_state resolved = {0};
    cui_theme_selection choice = {CUI_THEME_FORCE_LIGHT, {"custom", 6}, {"custom", 6}};
    assert(cui_theme_host_update(&resolved, &r, choice, (cui_theme_host_preferences){.revision = 1},
                                 16) == 0 &&
           color(resolved.semantic.colors.primary, custom.theme.primary));
    cui_theme_registry_entry before = *normalized;
    revision = r.revision;
    custom.motion.duration_fast = NAN;
    assert(cui_theme_registry_put(&r, &custom, true) == CUI_THEME_REGISTRY_INVALID &&
           r.revision == revision && !memcmp(&before, normalized, sizeof before));
    custom.motion = cui_theme_motion_default();
    custom.theme.font_size = NAN;
    assert(cui_theme_registry_put(&r, &custom, true) == CUI_THEME_REGISTRY_INVALID &&
           r.revision == revision && !memcmp(&before, normalized, sizeof before));
    custom = entry("Custom", true);
    assert(cui_theme_registry_put(&r, &custom, false) == 0 && r.count == 40);
    assert(cui_theme_registry_find(&r, string("custom")) !=
           cui_theme_registry_find(&r, string("Custom")));
    cui_theme_import_result report;
    const char *json =
        "{\"name\":\"Set\",\"author\":null,\"themes\":[{\"name\":\"Brand\",\"mode\":\"dark\","
        "\"font.family\":\"Owned\",\"font.size\":19,\"radius\":7,\"colors\":{\"primary."
        "background\":\"#123\",\"button.background\":\"#fed\"}}]}";
    assert(cui_theme_registry_import(&r, string(json), (cui_theme_import_options){0}, scratch, 64,
                                     &report) == 0 &&
           report.added == 1);
    const cui_theme_registry_entry *brand = cui_theme_registry_find(&r, string("Brand"));
    assert(brand && brand->theme.dark && brand->theme.font_size == 19 &&
           !strcmp(brand->semantic.typography.sans, "Owned"));
    assert(color(brand->theme.buttons[0].normal, (cui_color){255, 238, 221, 255}));
    memset(scratch, 0xa5, 64 * sizeof *scratch);
    assert(!strcmp(brand->name, "Brand") && !strcmp(brand->semantic.typography.sans, "Owned") &&
           color(brand->theme.primary, (cui_color){17, 34, 51, 255}));
    revision = r.revision;
    assert(cui_theme_registry_import(&r, string(json), (cui_theme_import_options){0}, scratch, 64,
                                     &report) == 0 &&
           report.skipped == 1 && r.revision == revision);
    assert(cui_theme_registry_import(&r, string(json),
                                     (cui_theme_import_options){.replace_existing = true}, scratch,
                                     64, &report) == 0 &&
           report.replaced == 1);
    assert(cui_theme_registry_import(&r, string("{\"extra\":0,\"author\":true,\"themes\":[]}"),
                                     (cui_theme_import_options){.allow_unsupported = true}, scratch,
                                     64, &report) == CUI_THEME_REGISTRY_INVALID);
    memcpy(saved, storage, 64 * sizeof *saved);
    size_t count = r.count;
    revision = r.revision;
    const char *bad = "{\"themes\":[{\"name\":\"First\"},{\"name\":\"Last\",\"colors\":{\"primary."
                      "background\":\"bad\"}}]}";
    assert(cui_theme_registry_import(&r, string(bad), (cui_theme_import_options){0}, scratch, 64,
                                     &report) != 0);
    assert(r.count == count && r.revision == revision &&
           !memcmp(storage, saved, 64 * sizeof *saved));
    assert(cui_theme_registry_import(&r, string(json), (cui_theme_import_options){0}, storage, 64,
                                     &report) == CUI_THEME_REGISTRY_INVALID);
    assert(cui_theme_registry_import(&r, string(json), (cui_theme_import_options){0}, scratch, 1,
                                     &report) == CUI_THEME_REGISTRY_CAPACITY_ERROR);
    char *scratch_json = (char *)scratch;
    strcpy(scratch_json, json);
    assert(cui_theme_registry_import(&r, string(scratch_json), (cui_theme_import_options){0},
                                     scratch, 64, &report) == CUI_THEME_REGISTRY_INVALID);
    assert(!memcmp(storage, saved, 64 * sizeof *saved));
    assert(cui_theme_registry_import(&r, (cui_str){(const char *)storage, 8},
                                     (cui_theme_import_options){0}, scratch, 64,
                                     &report) == CUI_THEME_REGISTRY_INVALID);
    assert(!memcmp(storage, saved, 64 * sizeof *saved));
    const char *partial =
        "{\"themes\":[{\"name\":\"Partial\",\"highlight\":{},\"colors\":{\"unmapped\":\"#fff\"}}]}";
    assert(cui_theme_registry_import(&r, string(partial), (cui_theme_import_options){0}, scratch,
                                     64, &report) == CUI_THEME_REGISTRY_UNSUPPORTED);
    assert(r.count == count && r.revision == revision &&
           !memcmp(storage, saved, 64 * sizeof *saved));
    assert(cui_theme_registry_import(&r, string(partial),
                                     (cui_theme_import_options){.allow_unsupported = true}, scratch,
                                     64, &report) == 0 &&
           report.partial && report.unsupported_fields == 2);
    assert(cui_theme_registry_reset(&r) && r.count == 38 &&
           !cui_theme_registry_find(&r, string("Brand")));
    r.revision = UINT64_MAX;
    memcpy(saved, storage, 64 * sizeof *saved);
    assert(!cui_theme_registry_reset(&r) && !memcmp(storage, saved, 64 * sizeof *saved));
    free(storage);
    free(scratch);
    free(saved);
    cui_theme_registry_entry one[1], temp[2];
    assert(cui_theme_registry_init(&r, one, 1, false));
    assert(cui_theme_registry_import(&r,
                                     string("{\"themes\":[{\"name\":\"one\"},{\"name\":\"two\"}]}"),
                                     (cui_theme_import_options){0}, temp, 2,
                                     &report) == CUI_THEME_REGISTRY_CAPACITY_ERROR &&
           r.count == 0);
    cui_theme_registry unchanged = r;
    assert(!cui_theme_registry_init(&r, one, 1, true) && !memcmp(&r, &unchanged, sizeof r));
}
static void motion_host(void) {
    cui_theme_motion_tokens m = cui_theme_motion_default(), p = m;
    assert(m.duration_instant == 0 && m.duration_fast == .12f && m.duration_normal == .18f &&
           m.duration_slow == .28f);
    assert(m.spring_control.epsilon == .001f && m.spring_move.epsilon == .1f);
    assert(cui_theme_motion_project(&m, 16, false, &p) && p.distance_short == 4 &&
           p.distance_medium == 8);
    assert(cui_theme_motion_project(&m, 16, true, &p) && !p.spring_move.travel &&
           p.spring_move.response == 0 && p.duration_normal == 0 && p.distance_medium == 0);
    cui_theme_motion_tokens saved = p;
    m.easing_enter.x1 = -1;
    assert(!cui_theme_motion_project(&m, 16, false, &p) && !memcmp(&p, &saved, sizeof p));
    m = cui_theme_motion_default();
    float v = -1;
    assert(cui_theme_bezier_sample(m.easing_move, .5f, &v) && fabsf(v - .8778336f) < .00001f);
    assert(!cui_theme_bezier_sample(m.easing_move, NAN, &v));
    cui_theme_registry_entry *entries = calloc(38, sizeof *entries);
    assert(entries);
    cui_theme_registry r;
    assert(cui_theme_registry_init(&r, entries, 38, true));
    cui_theme_host_state state = {0};
    cui_theme_selection select = {
        CUI_THEME_FOLLOW_SYSTEM, {"Default Light", 13}, {"Default Dark", 12}};
    cui_theme_host_preferences prefs = {.revision = 1};
    assert(cui_theme_host_update(&state, &r, select, prefs, 16) == 0 && state.changed &&
           state.metrics_changed && state.motion_changed && !state.theme.dark);
    assert(cui_theme_host_update(&state, &r, select, prefs, 16) == 0 && !state.changed &&
           !state.metrics_changed && !state.motion_changed);
    prefs.dark = true;
    prefs.revision++;
    assert(cui_theme_host_update(&state, &r, select, prefs, 16) == 0 && state.changed &&
           state.theme.dark);
    prefs.reduced_motion = true;
    prefs.auto_hide_scrollbars = true;
    prefs.revision++;
    assert(cui_theme_host_update(&state, &r, select, prefs, 16) == 0 && state.motion_changed &&
           state.theme.scrollbar.reduced_motion &&
           state.theme.scrollbar.visibility == CUI_SCROLLBAR_AUTO_HIDE);
    prefs.high_contrast = true;
    prefs.revision++;
    assert(cui_theme_host_update(&state, &r, select, prefs, 16) == 0 && state.theme.radius == 0 &&
           state.theme.background.r == 0 && state.theme.text.r == 255);
    assert(cui_theme_host_update(&state, &r, select, prefs, 16) == 0 && !state.changed &&
           !state.metrics_changed && !state.motion_changed);
    cui_theme_host_state old = state;
    prefs.revision--;
    assert(cui_theme_host_update(&state, &r, select, prefs, 16) == CUI_THEME_REGISTRY_INVALID &&
           !memcmp(&old, &state, sizeof old));
    prefs.revision++;
    select.dark_name = string("missing");
    assert(cui_theme_host_update(&state, &r, select, prefs, 16) == CUI_THEME_REGISTRY_MISSING &&
           !memcmp(&old, &state, sizeof old));
    free(entries);
    assert(!strcmp(state.semantic.typography.sans, ".SystemUIFont") && state.theme.dark &&
           state.motion.duration_fast == 0);
}
int main(void) {
    colors();
    semantic();
    bounds();
    registry();
    motion_host();
    return 0;
}
