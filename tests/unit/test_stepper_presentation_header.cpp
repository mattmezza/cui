/* SPDX-License-Identifier: MIT */
#include "cui/cui_stepper_presentation.h"
#include <type_traits>
static_assert(std::is_trivially_copyable<cui_stepper_presentation_options>::value,
              "portable descriptor");
int main() {
    return cui_stepper_present(nullptr, 0, nullptr, nullptr, 0, 0, {}, {}, {}).valid ? 1 : 0;
}
