/* SPDX-License-Identifier: MIT */
#include "cui/cui_disclosure_events.h"
#include <assert.h>
#include <string.h>
typedef struct {
    cui_context *ui;
    bool *open;
    cui_disclosure_change changes[8];
    size_t count;
    bool semantic_before;
} observer;
static void observe(void *user, const cui_disclosure_change *change) {
    observer *o = user;
    assert(o->count < 8 && *o->open == change->expanded);
    assert(change->previous != change->expanded);
    assert(change->item == 10 && change->item_index == 0);
    assert(change->event.target == 10);
    assert(change->event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION);
    assert(change->event.activation.target == 10);
    assert(change->event.activation.event_index == change->event.event_index);
    size_t count;
    const cui_semantic *sem = cui_semantics(o->ui, &count);
    for (size_t i = 0; i < count; ++i)
        if (sem[i].id == 10)
            assert(sem[i].details.expanded == o->semantic_before);
    o->changes[o->count++] = *change;
}
static cui_disclosure_event_result scene(cui_context *ui, bool *open, observer *o,
                                          bool legacy, bool disabled, bool passive) {
    assert(cui_begin_frame(ui, 300, 200, .016f));
    cui_begin_box(ui, 1, (cui_box){.layout_only = passive});
    cui_theme theme = cui_theme_light();
    cui_disclosure_event_result result = {0};
    if (legacy)
        result.changed = cui_collapsible_ex(ui, 10, &theme, cui_string("Title"), open,
                        (cui_collapsible_options){.disabled = disabled}, NULL, NULL);
    else
        result = cui_collapsible_events(ui, 10, &theme, cui_string("Title"), open,
                        (cui_collapsible_options){.disabled = disabled}, NULL, NULL,
                        (cui_disclosure_handler){o ? observe : NULL, o});
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return result;
}
static void click(cui_context *ui) {
    cui_rect r = cui_bounds(ui, 10);
    assert(r.w > 0 && r.h > 0);
    assert(cui_queue_event(ui, (cui_event){.type=CUI_EVENT_POINTER_DOWN,.x=r.x+2,.y=r.y+2}));
    assert(cui_queue_event(ui, (cui_event){.type=CUI_EVENT_POINTER_UP,.x=r.x+2,.y=r.y+2}));
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.max_elements=128,.max_commands=256});
    cui_context *old = cui_create((cui_config){.max_elements=128,.max_commands=256});
    assert(ui && old);
    bool open=false, previous=false;
    observer o={.ui=ui,.open=&open};
    scene(ui,&open,&o,false,false,false);
    scene(old,&previous,NULL,true,false,false);
    click(ui); click(ui); click(old); click(old);
    cui_disclosure_event_result result=scene(ui,&open,&o,false,false,false);
    cui_disclosure_event_result legacy=scene(old,&previous,NULL,true,false,false);
    assert(result.changed == legacy.changed && result.changed && !open && open == previous);
    assert(result.change_count == 2 && o.count == 2 && !result.count_overflow);
    assert(!o.changes[0].previous && o.changes[0].expanded);
    assert(o.changes[1].previous && !o.changes[1].expanded);
    assert(o.changes[0].event.event_index < o.changes[1].event.event_index);
    assert(o.changes[0].event.source == CUI_COMPONENT_EVENT_SOURCE_POINTER);
    click(ui);
    result=scene(ui,&open,NULL,false,false,false);
    assert(result.changed && open && result.change_count == 0);
    o.semantic_before=true;
    click(ui);
    result=scene(ui,&open,&o,false,true,false);
    assert(!result.changed && open && o.count == 2);
    scene(ui,&open,&o,false,false,true);
    assert(o.count == 2);
    /* Keyboard and accessibility are genuine accepted evidence, not inferred clicks. */
    scene(ui,&open,NULL,false,false,false);
    cui_focus(ui,10);
    assert(cui_queue_event(ui,(cui_event){.type=CUI_EVENT_KEY_DOWN,.key=CUI_KEY_ENTER}));
    result=scene(ui,&open,&o,false,false,false);
    assert(result.change_count == 1 && !open && o.count == 3);
    assert(o.changes[2].event.source == CUI_COMPONENT_EVENT_SOURCE_KEYBOARD);
    o.semantic_before=false;
    assert(cui_queue_event(ui,(cui_event){.type=CUI_EVENT_ACCESSIBILITY_ACTION,
        .accessibility_target=10,.accessibility_action=CUI_ACCESSIBILITY_ACTIVATE}));
    result=scene(ui,&open,&o,false,false,false);
    assert(result.change_count == 1 && open && o.count == 4);
    assert(o.changes[3].event.source == CUI_COMPONENT_EVENT_SOURCE_ACCESSIBILITY);
    assert(cui_base_collapsible_events(ui,0,cui_string(""),&open,false,
           (cui_disclosure_handler){observe,&o}).change_count == 0);
    /* Omit on the next frame; copied pointer-free evidence survives teardown. */
    assert(cui_begin_frame(ui,300,200,.016f)); cui_end_frame(ui);
    cui_destroy(ui); cui_destroy(old);
    assert(o.changes[0].event.activation.target == 10);
    return 0;
}
