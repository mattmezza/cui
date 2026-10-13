/* SPDX-License-Identifier: MIT */
#include "cui/cui_theme_registry.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Optional primary-source fixture audit. No copied theme/highlighter dependency:
 * caller supplies the pinned reference tree; missing fixture is an explicit skip. */
int main(int argc, char **argv) {
    const char *root = argc > 1 ? argv[1] : "/tmp/cui-gpui-reference";
    const char *files[] = {"themes/ayu.json", "themes/aurora.json"};
    cui_theme_registry_entry *entries = calloc(16, sizeof *entries),
                             *scratch = calloc(16, sizeof *scratch);
    char *source = malloc(CUI_THEME_JSON_MAX_BYTES + 1);
    assert(entries && scratch && source);
    for (size_t i = 0; i < 2; i++) {
        char path[1024];
        int length = snprintf(path, sizeof path, "%s/%s", root, files[i]);
        assert(length > 0 && (size_t)length < sizeof path);
        FILE *file = fopen(path, "rb");
        if (!file) {
            free(entries);
            free(scratch);
            free(source);
            puts("SKIP: pinned source fixture unavailable");
            return 77;
        }
        size_t bytes = fread(source, 1, CUI_THEME_JSON_MAX_BYTES + 1, file);
        assert(!ferror(file));
        fclose(file);
        assert(bytes <= CUI_THEME_JSON_MAX_BYTES);
        cui_theme_registry registry;
        assert(cui_theme_registry_init(&registry, entries, 16, false));
        cui_theme_import_result report;
        assert(cui_theme_registry_import(&registry, (cui_str){source, bytes},
                                         (cui_theme_import_options){0}, scratch, 16,
                                         &report) == CUI_THEME_REGISTRY_UNSUPPORTED);
        assert(registry.count == 0 && !report.partial);
        assert(cui_theme_registry_import(&registry, (cui_str){source, bytes},
                                         (cui_theme_import_options){.allow_unsupported = true},
                                         scratch, 16, &report) == CUI_THEME_REGISTRY_OK);
        assert(report.partial && report.unsupported_fields > 0 && report.added > 0 &&
               registry.count == report.added);
        memset(source, '!', bytes);
        memset(scratch, 0xa5, 16 * sizeof *scratch);
        const cui_theme_registry_entry *first = cui_theme_registry_sorted(&registry, 0);
        assert(first && first->name[0] && first->semantic.typography.sans[0]);
        if (i == 0) {
            const cui_theme_registry_entry *light =
                cui_theme_registry_find(&registry, (cui_str){"Ayu Light", 9});
            assert(light && light->theme.background.r == 252 && light->theme.background.g == 252 &&
                   light->theme.primary.r == 85);
        }
    }
    free(entries);
    free(scratch);
    free(source);
    return 0;
}
