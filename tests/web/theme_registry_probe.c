/* SPDX-License-Identifier: MIT */
#include "cui/cui_theme_registry.h"
#include <string.h>
static cui_theme_registry_entry_v2 entries[40], scratch[40];
static cui_theme_registry_v2 registry;
static cui_theme_host_state_v2 host;
int theme_probe_init(void) {
    static const char legacy[] =
        "{\"themes\":[{\"name\":\"Custom Light\",\"colors\":{"
        "\"skeleton.background\":\"#6d285a\",\"secondary.background\":\"#606162\"}},"
        "{\"name\":\"Custom Dark\",\"mode\":\"dark\","
        "\"colors\":{\"secondary.background\":\"#4a248a\"}}]}";
    if (!cui_theme_registry_init_v2(&registry, entries, 40, true) ||
        cui_theme_registry_import_v2(&registry, (cui_str){legacy, sizeof legacy - 1},
                                     (cui_theme_import_options){0}, scratch, 40, NULL))
        return 1;
    const char *names[] = {"Custom Light", "Custom Dark"};
    const char *sources[] = {"{\"tokens\":{\"colors\":{\"background\":\"#124578\",\"foreground\":"
                             "\"#ffffff\"},\"typography\":{\"md\":{\"size\":23}}}}",
                             "{\"tokens\":{\"colors\":{\"background\":\"#223311\",\"foreground\":"
                             "\"#ffffff\"},\"typography\":{\"md\":{\"size\":20}}}}"};
    for (unsigned i = 0; i < 2; i++) {
        const cui_theme_registry_entry_v2 *imported =
            cui_theme_registry_find_v2(&registry, (cui_str){names[i], strlen(names[i])});
        if (!imported)
            return 2;
        cui_theme_registry_entry_v2 e = *imported;
        if (cui_theme_semantic_import((cui_str){sources[i], strlen(sources[i])},
                                      &imported->base.theme, &e.base.semantic, &e.base.theme, NULL))
            return 2;
        if (cui_theme_registry_put_v2(&registry, &e, true))
            return 3;
    }
    return 0;
}
int theme_probe_update(int dark, int reduced, int contrast, unsigned revision) {
    return cui_theme_host_update_v2(
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
        return host.base.theme.dark;
    case 2:
        return host.base.preferences.reduced_motion;
    case 3:
        return host.base.preferences.high_contrast;
    case 4:
        return host.base.theme.background.r;
    case 5:
        return host.base.theme.background.g;
    case 6:
        return host.base.theme.background.b;
    case 7:
        return host.base.theme.font_size;
    case 8:
        return host.base.motion.duration_fast;
    case 9:
        return host.base.motion.distance_medium;
    case 10:
        return host.base.changed;
    case 11:
        return host.base.metrics_changed;
    case 12:
        return host.base.motion_changed;
    case 13:
        return (double)host.base.preferences.revision;
    case 14:
        return host.skeleton.background.r;
    case 15:
        return host.skeleton.background.g;
    case 16:
        return host.skeleton.background.b;
    case 17:
        return host.skeleton.background.a;
    default:
        return -1;
    }
}
