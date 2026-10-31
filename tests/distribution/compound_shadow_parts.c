/* SPDX-License-Identifier: MIT */
/* Match generated selective/umbrella declarations to the same rebuilt library. */
#ifdef CUI_COMPOUND_SHADOW_UMBRELLA
#include "cui_all.h"
#else
#include "cui_content.h"
#include "cui_media.h"
#include "cui_message.h"
#include "cui_status.h"
#endif
#include <assert.h>
int main(void) {
    assert(cui_bubble_part_id(1, CUI_BUBBLE_PART_SURFACE));
    assert(cui_message_part_id(1, CUI_MESSAGE_PART_HEADER));
    assert(cui_message_body_id(1, 255));
    assert(cui_message_scroller_part_id(1, CUI_MESSAGE_SCROLLER_PART_VIEWPORT));
    cui_message_scroller_state *s =
        cui_message_scroller_create((cui_message_scroller_config){2, 1, 20});
    assert(s && cui_message_scroller_row_id(1, s, 0));
    assert(!cui_message_scroller_row_id(1, s, 1));
    cui_message_scroller_destroy(s);
    assert(cui_notification_part_id(1, (cui_notification_id){1, 1}, CUI_NOTIFICATION_PART_CARD));
    assert(cui_attachment_part_id(1, CUI_ATTACHMENT_PART_MEDIA));
    assert(cui_attachment_child_part_id(1, 127, CUI_ATTACHMENT_CHILD_PART_RETRY));
    return 0;
}
