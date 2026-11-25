/* SPDX-License-Identifier: MIT */
#ifndef CUI_TOUCH_PAGE_HOST_H
#define CUI_TOUCH_PAGE_HOST_H
#include "cui/cui.h"
#include <string.h>

/* Browser host policy, not hit testing: Core supplies clipped/modal-aware hits,
   logical ancestry, dispatch targets and the reducers' final consumption. */
typedef struct {
    uint64_t id, device;
    cui_id target;
    bool active, blocked, move, refused, started;
} cui_touch_page_contact;
typedef struct {
    cui_touch_page_contact contacts[16];
    cui_id yielding_viewport;
} cui_touch_page_host;

static inline bool cui_touch_page_protected(const cui_touch_page_host *host,
                                            const cui_semantic *s) {
    return s->details.touch_drag_priority || s->role == CUI_ROLE_SLIDER ||
           s->role == CUI_ROLE_SCROLLBAR || s->role == CUI_ROLE_SEPARATOR ||
           (s->id != host->yielding_viewport &&
            (s->details.gesture_pan || s->details.gesture_zoom));
}

static inline void cui_touch_page_dispatch(cui_touch_page_host *host, cui_context *ui) {
    size_t count, semantic_count;
    const cui_event *events = cui_frame_events(ui, &count);
    const cui_semantic *semantics = cui_semantics(ui, &semantic_count);
    bool modal = !cui_pointer_scope_allowed(ui, 0);
    for (size_t n = 0; n < 16; n++)
        host->contacts[n].move = host->contacts[n].refused = false;
    for (size_t e = 0; e < count; e++) {
        if (events[e].pointer_type != CUI_POINTER_TOUCH)
            continue;
        size_t slot = 16, free_slot = 16;
        for (size_t n = 0; n < 16; n++) {
            if (host->contacts[n].active && host->contacts[n].id == events[e].pointer_id &&
                host->contacts[n].device == events[e].pointer_device)
                slot = n;
            if (!host->contacts[n].active)
                free_slot = n;
        }
        if (events[e].type == CUI_EVENT_POINTER_DOWN) {
            if (slot == 16)
                slot = free_slot;
            if (slot == 16)
                continue;
            bool blocked = modal || cui_event_consumed(ui, e);
            for (size_t n = 0; n < 16; n++)
                if (host->contacts[n].active && n != slot) {
                    host->contacts[n].blocked = true;
                    blocked = true;
                }
            for (size_t n = 0; n < semantic_count; n++) {
                const cui_semantic *s = &semantics[n];
                if (cui_touch_page_protected(host, s) &&
                    (cui_event_target(ui, e) == s->id || cui_event_hit_within_scope(ui, e, s->id) ||
                     ((s->details.gesture_pan || s->details.gesture_zoom) &&
                      cui_hovered(ui, s->id))))
                    blocked = true;
            }
            host->contacts[slot] = (cui_touch_page_contact){.id = events[e].pointer_id,
                                                            .device = events[e].pointer_device,
                                                            .target = cui_event_target(ui, e),
                                                            .active = true,
                                                            .blocked = blocked,
                                                            .started = true};
        } else if (slot != 16) {
            cui_touch_page_contact *contact = &host->contacts[slot];
            contact->blocked |= modal;
            if (events[e].type == CUI_EVENT_POINTER_MOVE || events[e].type == CUI_EVENT_SCROLL) {
                contact->move = true;
                /* Persistent ownership can retain a boundary target after its
                   reducer refuses the delta; final consumption gets first refusal. */
                contact->refused |= cui_event_consumed(ui, e);
                if (events[e].type == CUI_EVENT_SCROLL &&
                    (events[e].scroll_phase == CUI_SCROLL_PHASE_ENDED ||
                     events[e].scroll_phase == CUI_SCROLL_PHASE_CANCELLED))
                    contact->active = false;
            } else if (events[e].type == CUI_EVENT_GESTURE) {
                if (events[e].scroll_phase == CUI_SCROLL_PHASE_ENDED ||
                    events[e].scroll_phase == CUI_SCROLL_PHASE_CANCELLED) {
                    contact->active = false;
                } else if (host->yielding_viewport &&
                           cui_event_target(ui, e) == host->yielding_viewport &&
                           cui_event_gesture_contacts(ui, e) == 1) {
                    contact->move = true;
                    contact->refused |= cui_event_consumed(ui, e);
                } else {
                    contact->blocked = true;
                }
            } else if (events[e].type == CUI_EVENT_POINTER_UP) {
                contact->active = false;
            }
            /* Synthetic capture handoffs also use CANCEL: the input host handles
               physical cancellation explicitly, so these must not end a pan. */
        }
    }
}
static inline void cui_touch_page_finish(cui_touch_page_host *host, cui_context *ui) {
    size_t count;
    const cui_semantic *semantics = cui_semantics(ui, &count);
    for (size_t n = 0; n < 16; n++) {
        cui_touch_page_contact *contact = &host->contacts[n];
        if (!contact->active || !contact->started)
            continue;
        contact->started = false;
        cui_id target = contact->target;
        for (size_t step = 0; target && step < count; step++) {
            for (size_t i = 0; i < count; i++)
                if (semantics[i].id == target && cui_touch_page_protected(host, &semantics[i]))
                    contact->blocked = true;
            target = cui_semantic_parent(ui, target);
        }
    }
}
static inline bool cui_touch_page_available(const cui_touch_page_host *host, uint64_t id) {
    for (size_t n = 0; n < 16; n++) {
        const cui_touch_page_contact *c = &host->contacts[n];
        if (c->active && c->id == id && !c->blocked && c->move && !c->refused)
            return true;
    }
    return false;
}
static inline void cui_touch_page_end(cui_touch_page_host *host, uint64_t id) {
    for (size_t n = 0; n < 16; n++)
        if (host->contacts[n].id == id)
            memset(&host->contacts[n], 0, sizeof host->contacts[n]);
}
#endif
