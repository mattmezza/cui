/* SPDX-License-Identifier: MIT */
#include "cui/cui_alert_events.h"
#include <type_traits>
static_assert(std::is_trivially_copyable<cui_alert_dismissal>::value,
              "Alert completion payload must remain copyable");
int main() {
    cui_alert_options options{};
    cui_alert_dismiss_handler handler{};
    cui_alert_dismiss_result result = cui_alert_events(nullptr, 1, nullptr, options, handler);
    return result.valid || result.submitted || result.dismissal_count || result.legacy_dismissed;
}
