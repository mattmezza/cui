/* SPDX-License-Identifier: MIT */
#include "cui/cui_stepper_navigation.h"
#include <type_traits>
static_assert(std::is_trivially_copyable<cui_stepper_navigation_request>::value, "copied payload");
int main() {
    const auto result = cui_base_stepper_navigation(nullptr, 0, nullptr, 0, 0, false, {});
    return result.valid ? 1 : 0;
}
