/* SPDX-License-Identifier: MIT */
#if defined(CUI_TIME_FIELD_NEW_FIRST)
#include "cui/cui_picker.h"
#include "cui/cui_time_field_v1.h"
#else
#include "cui/cui_picker.h"
#include "cui/cui_time_field_v1.h"
#endif
#if defined(__cplusplus)
#define CUI_TFW_ASSERT static_assert
#else
#define CUI_TFW_ASSERT _Static_assert
#endif
#if UINTPTR_MAX == UINT64_MAX
CUI_TFW_ASSERT(sizeof(cui_time_field_options_v1) == 152, "native64 options");
CUI_TFW_ASSERT(sizeof(cui_time_field_state_v1) == 96, "native64 state");
CUI_TFW_ASSERT(sizeof(cui_time_field_result_v1) == 32, "native64 result");
#elif defined(__wasm32__)
CUI_TFW_ASSERT(sizeof(cui_time_field_options_v1) == 120, "wasm32 options");
CUI_TFW_ASSERT(sizeof(cui_time_field_state_v1) == 96, "wasm32 state");
CUI_TFW_ASSERT(sizeof(cui_time_field_result_v1) == 20, "wasm32 result");
#endif
int main(void) {
    return CUI_TIME_FIELD_VERSION_V1 != 1;
}
