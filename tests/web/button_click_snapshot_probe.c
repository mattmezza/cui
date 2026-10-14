/* SPDX-License-Identifier: MIT */
#include "button_click_event_probe.c"
double click_evidence(unsigned index, unsigned field) {
    if (index >= callbacks || index >= 32)
        return -1;
    const cui_button_click_event *click = &events_copied[index];
    const cui_activation_snapshot *s = &click->evidence;
    switch (field) {
    case 0:
        return click->has_evidence;
    case 1:
        return s->has_press;
    case 2:
        return s->has_release;
    case 3:
        return s->has_key_bounds;
    case 4:
        return s->press.x;
    case 5:
        return s->press.y;
    case 6:
        return s->release.x;
    case 7:
        return s->release.y;
    case 8:
        return s->press.modifiers;
    case 9:
        return s->release.modifiers;
    case 10:
        return s->press.click_count;
    case 11:
        return s->release.click_count;
    case 12:
        return s->press.type;
    case 13:
        return s->press.contact;
    case 14:
        return s->release.contact;
    case 15:
        return s->key_bounds.x;
    case 16:
        return s->key_bounds.y;
    case 17:
        return s->key_bounds.w;
    case 18:
        return s->key_bounds.h;
    case 19:
        return s->target;
    case 20:
        return s->event_index;
    case 21:
        return s->frame_sequence;
    default:
        return -1;
    }
}
