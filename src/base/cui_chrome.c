/* SPDX-License-Identifier: MIT */
#include "cui/cui_chrome.h"
cui_align cui_status_bar_center_alignment(bool has_left, bool has_right) {
    return has_left ? has_right ? CUI_ALIGN_CENTER : CUI_ALIGN_END : CUI_ALIGN_START;
}
