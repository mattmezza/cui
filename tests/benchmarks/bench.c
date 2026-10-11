/* SPDX-License-Identifier: MIT */
#include "cui/cui.h"
#include <stdio.h>
#include <time.h>
int main(void) {
    const size_t frames = 500, items = 100;
    cui_context *ui = cui_create((cui_config){.width = 800, .height = 600});
    if (!ui)
        return 1;
    clock_t start = clock();
    size_t total = 0;
    for (size_t f = 0; f < frames; f++) {
        if (!cui_begin_frame(ui, 800, 600, 1.0f / 60))
            return 2;
        cui_begin_box(ui, cui_id_from(ui, "root"), (cui_box){.width = -1, .height = -1, .gap = 2});
        for (size_t i = 0; i < items; i++) {
            char key[32];
            snprintf(key, sizeof(key), "item-%zu", i);
            cui_begin_box(ui, cui_id_from(ui, key),
                          (cui_box){.width = 300, .height = 20, .background = {36, 44, 48, 255}});
            cui_text(ui, cui_string("Measured layout workload"), 14,
                     (cui_color){240, 240, 240, 255});
            cui_end_box(ui);
        }
        cui_end_box(ui);
        total += cui_end_frame(ui).count;
        if (cui_last_error(ui) != CUI_OK)
            return 3;
    }
    double ms = 1000.0 * (clock() - start) / CLOCKS_PER_SEC;
    printf("CUI %s; CPU clock; %zu frames x %zu boxes; %.3f ms total; %.3f ms/frame; %zu commands "
           "total\n",
           CUI_VERSION, frames, items, ms, ms / frames, total);
    cui_destroy(ui);
    return 0;
}
