/* SPDX-License-Identifier: MIT */
#include "cui/cui_date_picker_actions.h"
static_assert(CUI_DATE_PICKER_PRESET_LIMIT == 8u, "bounded presets");
static_assert(sizeof(cui_date_picker_actions_state) > sizeof(cui_date_picker_state),
              "independent child");
