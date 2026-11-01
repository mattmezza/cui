/* SPDX-License-Identifier: MIT */
#if defined(CUI_TIME_TEXT_NEW_FIRST)
#include "cui/cui_picker.h"
#include "cui/cui_time_text.h"
#else
#include "cui/cui_picker.h"
#include "cui/cui_time_text.h"
#endif
#include <stddef.h>
#include <stdint.h>
#if UINTPTR_MAX == UINT64_MAX
#if defined(__cplusplus)
static_assert(sizeof(cui_time_locale_v1) == 240, "LP64 locale receipt");
static_assert(sizeof(cui_time_text_options_v1) == 64, "LP64 options receipt");
#else
_Static_assert(sizeof(cui_time_locale_v1) == 240, "LP64 locale receipt");
_Static_assert(sizeof(cui_time_text_options_v1) == 64, "LP64 options receipt");
#endif
#elif defined(__wasm32__)
_Static_assert(sizeof(cui_time_locale_v1) == 128, "wasm32 locale receipt");
_Static_assert(sizeof(cui_time_text_options_v1) == 60, "wasm32 options receipt");
#endif
int main(void) {
    cui_time t = {1, 2, 3};
    return t.hour != 1;
}
