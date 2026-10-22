/* SPDX-License-Identifier: MIT */
#include "cui/cui_stepper_visual.h"
#include <type_traits>
static_assert(std::is_trivially_copyable<cui_stepper_visual_options>::value, "portable descriptor");
int main() {
    return cui_stepper_visual_navigation(nullptr, 0, nullptr, nullptr, 0, 0, {}, {}).valid ? 1 : 0;
}
