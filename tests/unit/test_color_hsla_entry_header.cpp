/* SPDX-License-Identifier: MIT */
#include "cui/cui_color_hsla_entry.h"
#include <type_traits>
static_assert(std::is_trivially_copyable<cui_color_hsla_entry_result>::value,
              "copyable observer payload");
int main() {
    cui_color_hsla_entry_state state{};
    if (cui_color_hsla_entry_init(&state, {}) != CUI_COLOR_HSLA_ENTRY_OK)
        return 1;
    auto result = cui_base_color_hsla_entry(nullptr, 0, {}, &state, {}, {});
    cui_color_hsla_entry_destroy(&state);
    return result.input.error == CUI_EDIT_INVALID_ARGUMENT ? 0 : 1;
}
