/* SPDX-License-Identifier: MIT */
#include "cui/cui_bridge.h"
#include <stddef.h>
#ifdef __cplusplus
#define CHECK static_assert
#else
#define CHECK _Static_assert
#endif
CHECK(sizeof(cui_bridge_icon_handle_v1) == 8, "handle");
CHECK(sizeof(cui_bridge_icon_limits_v1) == 48, "limits");
CHECK(sizeof(cui_bridge_icon_info_v1) == 80, "info");
CHECK(sizeof(cui_bridge_icon_bitmap_v1) == 64, "bitmap");
CHECK(sizeof(cui_bridge_icon_style_v1) == 128, "style");
CHECK(sizeof(cui_bridge_icon_constraints_v1) == 32, "constraints");
CHECK(sizeof(cui_bridge_icon_options_v1) == 256, "options");
CHECK(sizeof(cui_bridge_icon_result_v1) == 40, "result");
#define OFFSET(t, f, n) CHECK(offsetof(t, f) == n, #f)
OFFSET(cui_bridge_icon_handle_v1, slot, 0);
OFFSET(cui_bridge_icon_handle_v1, generation, 4);
OFFSET(cui_bridge_icon_limits_v1, struct_size, 0);
OFFSET(cui_bridge_icon_limits_v1, schema_version, 4);
OFFSET(cui_bridge_icon_limits_v1, reserved, 8);
OFFSET(cui_bridge_icon_limits_v1, max_svg_bytes, 16);
OFFSET(cui_bridge_icon_limits_v1, max_pixels, 24);
OFFSET(cui_bridge_icon_limits_v1, max_cache_bytes, 32);
OFFSET(cui_bridge_icon_limits_v1, max_versions, 40);
OFFSET(cui_bridge_icon_info_v1, struct_size, 0);
OFFSET(cui_bridge_icon_info_v1, schema_version, 4);
OFFSET(cui_bridge_icon_info_v1, icon_error, 8);
OFFSET(cui_bridge_icon_info_v1, reference_count, 12);
OFFSET(cui_bridge_icon_info_v1, parse_count, 16);
OFFSET(cui_bridge_icon_info_v1, raster_count, 24);
OFFSET(cui_bridge_icon_info_v1, cache_bytes, 32);
OFFSET(cui_bridge_icon_info_v1, cache_versions, 40);
OFFSET(cui_bridge_icon_info_v1, source_revision, 48);
OFFSET(cui_bridge_icon_info_v1, source_kind, 56);
OFFSET(cui_bridge_icon_info_v1, flags, 60);
OFFSET(cui_bridge_icon_info_v1, handle, 64);
OFFSET(cui_bridge_icon_info_v1, reserved, 72);
OFFSET(cui_bridge_icon_bitmap_v1, struct_size, 0);
OFFSET(cui_bridge_icon_bitmap_v1, schema_version, 4);
OFFSET(cui_bridge_icon_bitmap_v1, format, 8);
OFFSET(cui_bridge_icon_bitmap_v1, reserved, 12);
OFFSET(cui_bridge_icon_bitmap_v1, width, 16);
OFFSET(cui_bridge_icon_bitmap_v1, height, 20);
OFFSET(cui_bridge_icon_bitmap_v1, stride, 24);
OFFSET(cui_bridge_icon_bitmap_v1, reserved2, 28);
OFFSET(cui_bridge_icon_bitmap_v1, required_bytes, 32);
OFFSET(cui_bridge_icon_bitmap_v1, revision, 40);
OFFSET(cui_bridge_icon_bitmap_v1, paint_x, 48);
OFFSET(cui_bridge_icon_bitmap_v1, paint_y, 52);
OFFSET(cui_bridge_icon_bitmap_v1, paint_width, 56);
OFFSET(cui_bridge_icon_bitmap_v1, paint_height, 60);
OFFSET(cui_bridge_icon_style_v1, struct_size, 0);
OFFSET(cui_bridge_icon_style_v1, schema_version, 4);
OFFSET(cui_bridge_icon_style_v1, flags, 8);
OFFSET(cui_bridge_icon_style_v1, width, 16);
OFFSET(cui_bridge_icon_style_v1, padding, 24);
OFFSET(cui_bridge_icon_style_v1, background_rgba, 36);
OFFSET(cui_bridge_icon_style_v1, border_width, 44);
OFFSET(cui_bridge_icon_style_v1, align_x, 48);
OFFSET(cui_bridge_icon_style_v1, offset_x, 56);
OFFSET(cui_bridge_icon_style_v1, corners, 64);
OFFSET(cui_bridge_icon_style_v1, border_edges, 80);
OFFSET(cui_bridge_icon_style_v1, max_width, 96);
OFFSET(cui_bridge_icon_style_v1, opacity, 104);
OFFSET(cui_bridge_icon_style_v1, padding_edges, 108);
OFFSET(cui_bridge_icon_style_v1, reserved_tail, 124);
OFFSET(cui_bridge_icon_constraints_v1, flags, 8);
OFFSET(cui_bridge_icon_constraints_v1, min_width, 16);
OFFSET(cui_bridge_icon_constraints_v1, max_height, 28);
OFFSET(cui_bridge_icon_options_v1, flags, 8);
OFFSET(cui_bridge_icon_options_v1, size, 12);
OFFSET(cui_bridge_icon_options_v1, design_size, 16);
OFFSET(cui_bridge_icon_options_v1, custom_size, 32);
OFFSET(cui_bridge_icon_options_v1, inherited_text_size, 36);
OFFSET(cui_bridge_icon_options_v1, width, 40);
OFFSET(cui_bridge_icon_options_v1, opacity, 48);
OFFSET(cui_bridge_icon_options_v1, rem_size, 52);
OFFSET(cui_bridge_icon_options_v1, color_rgba, 56);
OFFSET(cui_bridge_icon_options_v1, transform, 64);
OFFSET(cui_bridge_icon_options_v1, style, 88);
OFFSET(cui_bridge_icon_options_v1, constraints, 216);
OFFSET(cui_bridge_icon_options_v1, reserved_tail, 248);
OFFSET(cui_bridge_icon_result_v1, icon_error, 8);
OFFSET(cui_bridge_icon_result_v1, frame_status, 12);
OFFSET(cui_bridge_icon_result_v1, node, 16);
OFFSET(cui_bridge_icon_result_v1, frame_sequence, 24);
OFFSET(cui_bridge_icon_result_v1, reserved, 32);
#undef OFFSET
#undef CHECK
int main(void) {
    return 0;
}
