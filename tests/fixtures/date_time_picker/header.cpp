#include "cui/cui_date_time_picker.h"
#include <type_traits>
static_assert(std::is_standard_layout<cui_date_time_picker_state>::value, "C11 layout");
static_assert(std::is_trivially_copyable<cui_date_time_value>::value, "value ownership");
auto datetime_base = &cui_base_date_time_picker;
auto datetime_submit = &cui_date_time_picker_submit_time;
