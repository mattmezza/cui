/* SPDX-License-Identifier: MIT */
#include "cui/cui.h"
#include <cstddef>
#include <type_traits>
static_assert(sizeof(cui_scroll_body_layout_v1) == 80, "V1 unchanged");
static_assert(sizeof(cui_scroll_region_sizing_v1) == 24, "region record");
static_assert(sizeof(cui_scroll_body_layout_v2) == 144, "V2 record");
static_assert(offsetof(cui_scroll_body_layout_v2, content_region) == 80, "Content ID");
static_assert(offsetof(cui_scroll_body_layout_v2, footer) == 88, "Footer ID");
static_assert(offsetof(cui_scroll_body_layout_v2, content_sizing) == 96, "Content sizing");
static_assert(offsetof(cui_scroll_body_layout_v2, body_sizing) == 120, "Body sizing");
using layout_v2 = bool (*)(cui_context *, const cui_scroll_body_layout_v2 *, float *, float *);
static_assert(std::is_same<decltype(&cui_layout_scroll_body_v2), layout_v2>::value,
              "public C declaration authority");
int main() {
    return 0;
}
