/* SPDX-License-Identifier: MIT */
/* Private harness instrumentation, absent from production builds. */
#ifndef CUI_ACCESSIBILITY_TEST_H
#define CUI_ACCESSIBILITY_TEST_H
#include <stdint.h>
uint64_t cui_accessibility_test_live(void);
uint64_t cui_accessibility_test_released(void);
#endif
