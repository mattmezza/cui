/* SPDX-License-Identifier: MIT */
#include "cui/cui_hover_card_extended.h"
#include <type_traits>
static_assert(std::is_trivially_copyable<cui_hover_card_contact>::value, "caller workspace");
static_assert(std::is_trivially_copyable<cui_hover_lifecycle_event>::value, "copied event");
int main() {
    const auto result = cui_begin_hover_card_extended(nullptr, 0, nullptr, nullptr, {}, {},
                                                     nullptr, nullptr, {});
    return result.lifecycle.valid ? 1 : 0;
}
