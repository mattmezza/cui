/* SPDX-License-Identifier: MIT */
#include "app.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    ce_app a;
    ce_init(&a);
    cui_component_check_request request = {.current = CUI_CHECK_UNCHECKED,
                                           .requested = CUI_CHECK_CHECKED};
    assert(cui_component_event_init(&request.event, CUI_COMPONENT_EVENT_REQUEST, CE_SETTING,
                                    CUI_COMPONENT_EVENT_SOURCE_KEYBOARD));
    ce_setting_request(&a, &request);
    assert(a.requests == 1 && a.rejected == 1 && a.setting == CUI_CHECK_UNCHECKED);
    ce_allow_request(&a, &request);
    ce_setting_request(&a, &request);
    assert(a.requests == 2 && a.rejected == 1 && a.setting == CUI_CHECK_CHECKED);
    request.requested = CUI_CHECK_UNCHECKED;
    assert(a.last_request.requested == CUI_CHECK_CHECKED); /* Owned snapshot copy. */
    ce_setting_request(&a, &request);
    assert(a.setting == CUI_CHECK_UNCHECKED);
    cui_component_event_metadata activation;
    assert(cui_component_event_init(&activation, CUI_COMPONENT_EVENT_ACTIVATE, CE_APPLY,
                                    CUI_COMPONENT_EVENT_SOURCE_POINTER));
    ce_apply(&a, &activation);
    assert(a.actions == 1);
    ce_close(&a, &activation);
    assert(a.visible && a.close_requested && a.closes == 1);
    assert(ce_finish_frame(&a) && !a.visible && !a.close_requested);
    assert(!ce_finish_frame(&a) && a.closes == 1);
    ce_reopen(&a, &activation);
    assert(!a.visible && ce_finish_frame(&a) && a.visible);
    puts("PASS owner accept/reject, copied request, passive counts and deferred omission");
    return 0;
}
