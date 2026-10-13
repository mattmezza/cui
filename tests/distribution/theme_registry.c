/* SPDX-License-Identifier: MIT */
#define CUI_THEME_REGISTRY_IMPLEMENTATION
#include "cui_theme_registry.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
int main(void) {
    cui_theme_registry_entry entries[2];
    cui_theme_registry registry;
    assert(cui_theme_registry_init(&registry, entries, 2, false));
    cui_theme_registry_entry entry = {0};
    strcpy(entry.name, "Brand");
    entry.theme = cui_theme_light();
    assert(cui_theme_semantic_snapshot(&entry.theme, &entry.semantic));
    entry.motion = cui_theme_motion_default();
    assert(cui_theme_registry_put(&registry, &entry, false) == CUI_THEME_REGISTRY_OK);
    memset(&entry, 0, sizeof entry);
    const cui_theme_registry_entry *found =
        cui_theme_registry_find(&registry, (cui_str){"Brand", 5});
    assert(found && found->semantic.typography.sans[0]);
    cui_theme_host_state host = {0};
    assert(cui_theme_host_update(
               &host, &registry,
               (cui_theme_selection){.mode = CUI_THEME_FORCE_LIGHT, .light_name = {"Brand", 5}},
               (cui_theme_host_preferences){.reduced_motion = true}, 16) == CUI_THEME_REGISTRY_OK);
    assert(host.ready && host.changed && host.motion.duration_fast == 0);
    cui_color color = {0};
    assert(cui_theme_color_parse((cui_str){"blue-600/80", 11}, &color) && color.a == 204);
    assert(cui_theme_registry_reset(&registry) && registry.count == 0);
    return 0;
}
