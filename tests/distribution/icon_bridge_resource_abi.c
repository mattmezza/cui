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
#undef OFFSET
#undef CHECK
int main(void) {
    return 0;
}
