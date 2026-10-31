/* SPDX-License-Identifier: MIT */
#include "cui/cui.h"
#include "cui/cui_scroll.h"
#include <cstddef>
#include <type_traits>
static_assert(sizeof(cui_scrollbar_track_layout_v1) == 48, "portable V1 record");
static_assert(offsetof(cui_scrollbar_track_layout_v1, viewport) == 16, "viewport");
static_assert(offsetof(cui_scrollbar_track_layout_v1, bar) == 24, "bar");
static_assert(offsetof(cui_scrollbar_track_layout_v1, thumb) == 32, "thumb");
static_assert(offsetof(cui_scrollbar_track_layout_v1, thickness) == 40, "dimensions");
static_assert(std::is_same<decltype(&cui_layout_scrollbar_track_v1),
                           bool (*)(cui_context *, const cui_scrollbar_track_layout_v1 *)>::value,
              "C API signature");
int main() {
    return 0;
}
