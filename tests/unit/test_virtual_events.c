/* SPDX-License-Identifier: MIT */
/* Reuse real deferred-child/model fixtures and run their legacy assertions too. */
#define main virtual_legacy_main
#include "test_virtual_styled.c"
#undef main
#include "cui/cui_virtual_events.h"
typedef struct { size_t count; row_probe *rows; cui_scroll_change saved; } observer;
static void observed(void *user, const cui_scroll_change *change) {
    observer *p = user;
    if (p->rows) assert(p->rows->calls > 0);
    ++p->count;
    p->saved = *change;
    assert(change->event.kind == CUI_COMPONENT_EVENT_CHANGE);
    assert(change->event.target == 10);
    assert(change->event.flags == CUI_COMPONENT_EVENT_HAS_FRAME);
}
static void compare(bool styled) {
    cui_context *a = cui_create((cui_config){.width=400,.height=300});
    cui_context *b = cui_create((cui_config){.width=400,.height=300});
    cui_virtual_list *v = model();
    cui_theme t = cui_theme_light();
    cui_virtual_widget_state sa={0}, sb={0};
    row_probe pa={.child=true}, pb={.child=true};
    observer p={.rows=styled ? &pb : NULL};
    cui_scroll_handler h={observed,&p};
    cui_virtual_widget_options o={.scroll={.width=100,.height=60,.hide_scrollbars=true}};
    for (unsigned iteration=0;iteration<9;iteration++) {
        if (iteration==1) {
            cui_event e={.type=CUI_EVENT_SCROLL,.x=5,.y=5,.dy=-.5f,.scroll_precise=true};
            assert(cui_queue_event(a,e) && cui_queue_event(b,e));
            pa.defer=pb.defer=styled;
        }
        if (iteration==2) pa.defer=pb.defer=false;
        if (iteration==3) sa.scroll.y=sb.scroll.y=NAN;
        if (iteration==4) { o.scroll.disabled=true; sa.scroll.y=sb.scroll.y=999; }
        if (iteration==5) { o.scroll.disabled=false; cui_focus(a,10); cui_focus(b,10); key(a,CUI_KEY_HOME);key(b,CUI_KEY_HOME); }
        if (iteration==6) { key(a,CUI_KEY_END);key(b,CUI_KEY_END);key(a,CUI_KEY_HOME);key(b,CUI_KEY_HOME); }
        if (iteration==7) sa.scroll.y=sb.scroll.y=3;
        assert(cui_begin_frame(a,400,300,.016f));
        assert(cui_begin_frame(b,400,300,.016f));
        pa.calls=pb.calls=0;
        size_t old_count=p.count;
        cui_virtual_widget_result legacy=styled ? cui_virtual_list_draw(a,10,&t,v,&sa,o,row,&pa) : cui_base_virtual_list(a,10,v,&sa,o);
        cui_virtual_event_result r=styled ? cui_virtual_list_draw_events(b,10,&t,v,&sb,o,row,&pb,iteration==7 ? (cui_scroll_handler){0}:h) : cui_base_virtual_list_events(b,10,v,&sb,o,iteration==7 ? (cui_scroll_handler){0}:h);
        assert(r.valid && !r.count_overflow);
        assert(r.control_result.error==legacy.error && r.control_result.scroll_changed==legacy.scroll_changed && r.control_result.window_pending==legacy.window_pending);
        assert(r.control_result.view.begin==legacy.view.begin && r.control_result.view.end==legacy.view.end && r.control_result.view.scroll==legacy.view.scroll);
        assert(sa.scroll.x==sb.scroll.x && sa.scroll.y==sb.scroll.y && pa.calls==pb.calls);
        assert(r.change_count==p.count-old_count && r.change_count<=1);
        size_t ac=0,bc=0;
        (void)cui_frame_events(a,&ac); (void)cui_frame_events(b,&bc);
        assert(ac==bc);
        for(size_t event_index=0;event_index<ac;event_index++)
            assert(cui_event_consumed(a,event_index)==cui_event_consumed(b,event_index));
        size_t asc=0,bsc=0;
        const cui_semantic *asem=cui_semantics(a,&asc), *bsem=cui_semantics(b,&bsc);
        assert(asc==bsc);
        for(size_t semantic_index=0;semantic_index<asc;semantic_index++) {
            assert(asem[semantic_index].id==bsem[semantic_index].id);
            assert(asem[semantic_index].details.scroll_y==bsem[semantic_index].details.scroll_y);
        }
        if (iteration==3) assert(r.changed && !p.saved.has_previous && p.saved.previous.x==0 && p.saved.previous.y==0);
        if (iteration==2 || iteration==6 || iteration==8) assert(!r.changed && !r.change_count);
        if (iteration==1 && styled) assert(r.control_result.window_pending && p.saved.value.y==.5f && r.control_result.view.scroll==0);
        if (iteration==7) assert(!r.change_count);
        cui_end_frame(a); cui_end_frame(b);
        assert(cui_last_error(a)==CUI_OK && cui_last_error(b)==CUI_OK);
        assert(cui_bounds(a,10).h==cui_bounds(b,10).h);
    }
    /* Rejected work cannot normalize state or call the observer. */
    sb.scroll.y=999;
    assert(!cui_base_virtual_list_events(b,10,v,&sb,o,h).valid && sb.scroll.y==999);
    assert(cui_begin_frame(b,400,300,.016f));
    assert(!cui_base_virtual_list_events(b,0,v,&sb,o,h).valid && sb.scroll.y==999);
    cui_end_frame(b); /* omission retains no handler */
    cui_scroll_change copy=p.saved;
    cui_destroy(a);cui_destroy(b);cui_virtual_destroy(v);
    assert(copy.event.target==10 && copy.event.frame_sequence>0);
}
static void passive(void) {
    cui_context *ui=cui_create((cui_config){.width=400,.height=300});
    cui_virtual_list *v=model(); cui_theme t=cui_theme_light(); observer p={0};
    for(unsigned mode=1;mode<=2;mode++) for(unsigned styled=0;styled<2;styled++) {
        cui_virtual_widget_state state={0}; state.scroll.y=999;
        cui_virtual_widget_options o={.scroll={.width=100,.height=60,.hide_scrollbars=true}};
        row_probe rows={0};
        assert(cui_begin_frame(ui,400,300,.016f));
        cui_begin_box(ui,1,(cui_box){.width=200,.height=100,.inert=mode==1,.layout_only=mode==2});
        cui_virtual_event_result r=styled ? cui_virtual_list_draw_events(ui,10,&t,v,&state,o,row,&rows,(cui_scroll_handler){observed,&p}) : cui_base_virtual_list_events(ui,10,v,&state,o,(cui_scroll_handler){observed,&p});
        assert(r.valid && r.changed && !r.change_count && !p.count && state.scroll.y<999);
        cui_end_box(ui); cui_end_frame(ui); assert(cui_last_error(ui)==CUI_OK);
    }
    cui_destroy(ui); cui_virtual_destroy(v);
}
typedef struct { cui_virtual_widget_state *state; size_t calls; bool accept; } measure_mutation;
static bool mutating_measure(void *user, size_t index, uint64_t key_value,
                             float cross, cui_size *out) {
    measure_mutation *p=user;
    (void)index; (void)key_value; (void)cross;
    ++p->calls; p->state->scroll.y=7;
    *out=(cui_size){50,20}; return p->accept;
}
static void measurement_compatibility(void) {
    for(unsigned accept=0;accept<2;accept++) {
        cui_context *a=cui_create((cui_config){.width=400,.height=300});
        cui_context *b=cui_create((cui_config){.width=400,.height=300});
        cui_virtual_list *v=model(); cui_theme t=cui_theme_light();
        cui_virtual_widget_state sa={0},sb={0};
        measure_mutation ma={&sa,0,accept!=0},mb={&sb,0,accept!=0};
        cui_virtual_widget_options o={.scroll={.width=100,.height=60,.hide_scrollbars=true},.measure=mutating_measure,.measure_user=&ma};
        row_probe pa={0},pb={0}; observer p={0};
        assert(cui_begin_frame(a,400,300,.016f));assert(cui_begin_frame(b,400,300,.016f));
        cui_virtual_widget_result legacy=cui_virtual_list_draw(a,10,&t,v,&sa,o,row,&pa);
        o.measure_user=&mb;
        cui_virtual_event_result r=cui_virtual_list_draw_events(b,10,&t,v,&sb,o,row,&pb,(cui_scroll_handler){observed,&p});
        assert(ma.calls==1 && mb.calls==1 && sa.scroll.y==7 && sb.scroll.y==7);
        assert(legacy.error==r.control_result.error && pa.calls==pb.calls);
        assert(r.valid==(accept!=0) && p.count==accept);
        if(accept) assert(p.saved.previous.y==0 && p.saved.value.y==7);
        cui_end_frame(a);cui_end_frame(b);cui_destroy(a);cui_destroy(b);cui_virtual_destroy(v);
    }
}
int main(void) { measurement_compatibility(); passive(); assert(virtual_legacy_main()==0); compare(false); compare(true); puts("virtual events ok"); return 0; }
