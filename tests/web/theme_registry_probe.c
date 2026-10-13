/* SPDX-License-Identifier: MIT */
#include "cui/cui_theme_registry.h"
#include <string.h>
static cui_theme_registry_entry entries[40];
static cui_theme_registry registry;
static cui_theme_host_state host;
int theme_probe_init(void) {
    if (!cui_theme_registry_init(&registry, entries, 40, true))
        return 1;
    const char *names[] = {"Custom Light", "Custom Dark"};
    const char *sources[] = {"{\"tokens\":{\"colors\":{\"background\":\"#124578\",\"foreground\":"
                             "\"#ffffff\"},\"typography\":{\"md\":{\"size\":23}}}}",
                             "{\"tokens\":{\"colors\":{\"background\":\"#223311\",\"foreground\":"
                             "\"#ffffff\"},\"typography\":{\"md\":{\"size\":20}}}}"};
    for (unsigned i = 0; i < 2; i++) {
        cui_theme_registry_entry e = {.motion = cui_theme_motion_default()};
        cui_theme base = i ? cui_theme_dark() : cui_theme_light();
        strcpy(e.name, names[i]);
        if (cui_theme_semantic_import((cui_str){sources[i], strlen(sources[i])}, &base, &e.semantic,
                                      &e.theme, NULL))
            return 2;
        if (cui_theme_registry_put(&registry, &e, false))
            return 3;
    }
    return 0;
}
int theme_probe_update(int dark, int reduced, int contrast, unsigned revision) {
    return cui_theme_host_update(
        &host, &registry,
        (cui_theme_selection){CUI_THEME_FOLLOW_SYSTEM, {"Custom Light", 12}, {"Custom Dark", 11}},
        (cui_theme_host_preferences){.dark = dark != 0,
                                     .reduced_motion = reduced != 0,
                                     .high_contrast = contrast != 0,
                                     .revision = revision},
        16);
}
double theme_probe_stat(unsigned field) {
    switch (field) {
    case 0:
        return (double)registry.count;
    case 1:
        return host.theme.dark;
    case 2:
        return host.preferences.reduced_motion;
    case 3:
        return host.preferences.high_contrast;
    case 4:
        return host.theme.background.r;
    case 5:
        return host.theme.background.g;
    case 6:
        return host.theme.background.b;
    case 7:
        return host.theme.font_size;
    case 8:
        return host.motion.duration_fast;
    case 9:
        return host.motion.distance_medium;
    case 10:
        return host.changed;
    case 11:
        return host.metrics_changed;
    case 12:
        return host.motion_changed;
    case 13:
        return (double)host.preferences.revision;
    default:
        return -1;
    }
}
