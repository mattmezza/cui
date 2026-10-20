/* SPDX-License-Identifier: MIT */
#include "cui/cui_date_picker_calendar.h"
#include <type_traits>
static_assert(std::is_trivially_copyable<cui_calendar_presentation_options>::value,
              "value options");
static_assert(std::is_trivially_copyable<cui_calendar_cell_info>::value, "copied occurrence");
static_assert(std::is_trivially_copyable<cui_calendar_behavior_options>::value, "value policies");
static_assert(std::is_trivially_copyable<cui_date_picker_calendar_options>::value,
              "value forwarding");
int main() {
    if (cui_date_picker_calendar_id(0) || !cui_date_picker_calendar_id(31))
        return 1;
    if (cui_date_picker_ex(nullptr, 31, nullptr, {}, nullptr, nullptr, {}, {}))
        return 1;
    cui_calendar_cell_info info = {};
    if (cui_calendar_cell_resolve(0, {2024, 2, 1}, 1, 0, 0, &info) || info.id)
        return 1;
    if (!cui_calendar_cell_resolve(10, {2024, 2, 1}, 1, 0, 0, &info) || !info.neighboring)
        return 1;
    const auto result = cui_base_calendar_with_behavior(nullptr, 0, nullptr, nullptr, {}, {}, {});
    return result.changed || result.activated || result.navigated ? 1 : 0;
}
