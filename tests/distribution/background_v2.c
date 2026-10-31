/* SPDX-License-Identifier: MIT */
/* C-only synthetic transport consumer; deliberately NOT an adapter capability. */
#include "cui_draw_v2.h"
#include "cui_renderer.h"
#include <assert.h>
_Static_assert(_Generic((cui_draw_list_v2 *)0, cui_draw_list *: 0, default: 1),
               "V2 list must not alias V1 consumer input");
_Static_assert(_Generic((cui_draw_command_v2 *)0, cui_draw_command *: 0, default: 1),
               "V2 command arrays must not alias V1 storage");
int main(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 32, .max_commands = 64});
    assert(ui);
    cui_frame_v2_options options = {
        .size = sizeof(options),
        .support = {.size = sizeof(cui_draw_v2_support),
                    .abi_version = CUI_DRAW_ABI_V2,
                    .background_output = CUI_BACKGROUND_OUTPUT_SRGBA8_CENTER_V1,
                    .features = CUI_DRAW_V2_FEATURE_BACKGROUNDS,
                    .max_commands = 64,
                    .max_backgrounds = 32,
                    .max_pixels = 4096,
                    .max_pixels_per_background = 4096,
                    .cpu_bytes = 16384,
                    .texture_bytes = 16384,
                    .transfer_bytes = 16384,
                    .metadata_bytes = 4096},
        .required_features = CUI_DRAW_V2_FEATURE_BACKGROUNDS};
    /* Test-only mirror of this checkpoint's private owned record, to exercise
     * exact byte admission through the public API rather than an allocator guess. */
    struct owner_probe {
        cui_background brush;
        bool active;
    };
    size_t map_bytes = 32 * sizeof(uint32_t);
    size_t alignment = _Alignof(struct owner_probe);
    map_bytes += (alignment - map_bytes % alignment) % alignment;
    options.background_bytes = map_bytes + 32 * sizeof(struct owner_probe) - 1;
    assert(!cui_begin_frame_v2(ui, 20, 10, 0, &options));
    assert(cui_last_error(ui) == CUI_ERROR_CAPACITY && !cui_frame_active(ui));
    cui_clear_error(ui);
    options.background_bytes++;
    assert(cui_begin_frame_v2(ui, 20, 10, 0, &options));
    cui_draw_list_v2 empty;
    assert(cui_end_frame_v2(ui, &empty));
    options.background_bytes = 0;
    assert(cui_begin_frame_v2(ui, 20, 10, 0, &options));
    cui_begin_box(ui, 1,
                  (cui_box){.width = 8, .height = 8, .fixed_width = true, .fixed_height = true});
    cui_background brush;
    assert(cui_background_make_linear(CUI_BACKGROUND_CSS, CUI_BACKGROUND_SRGB, 90,
                                      (cui_background_stop){{1, 0, 0, 1}, 0},
                                      (cui_background_stop){{0, 0, 1, 1}, 1}, &brush));
    assert(cui_set_background(ui, &brush));
    brush.stops[0].color.r = 0;
    cui_end_box(ui);
    cui_draw_list_v2 list;
    assert(cui_end_frame_v2(ui, &list));
    cui_draw_command_v2 copied = {0};
    bool found = false;
    for (size_t i = 0; i < list.count; i++)
        if (list.commands[i].kind == CUI_DRAW_V2_BACKGROUND) {
            copied = list.commands[i];
            found = true;
        }
    assert(found);
    cui_destroy(ui);
    /* Only the by-value semantic brush survives: no full-list deep-copy claim. */
    assert(copied.payload.background.brush.stops[0].color.r == 1);
    assert(cui_background_valid(&copied.payload.background.brush));
    cui_background_pixel pixel;
    assert(cui_background_sample(&copied.payload.background.brush, copied.payload.background.bounds,
                                 4, 4, &pixel));
    assert(pixel.r > 0 && pixel.b > 0 && pixel.a == 1);
    return 0;
}
