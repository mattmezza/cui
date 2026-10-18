/* SPDX-License-Identifier: MIT */
#include "cui/cui_picker_events.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct {
    cui_context *ui;
    cui_calendar_state state;
    cui_date_selection selection;
    cui_calendar_options options;
    cui_date_selection_change copies[16];
    size_t calls, policy_calls, policy_at_callback, semantic_at_callback;
    unsigned mode; /* Styled, Base, reduce-only with caller-owned submission. */
    bool inert, layout, omit, deny_weekends, request_omit;
    cui_draw_list draw;
} fixture;
static bool policy(void *user,cui_date date) {
    fixture *f=user;++f->policy_calls;
    return f->deny_weekends && (cui_date_weekday(date)==0 || cui_date_weekday(date)==6);
}
static void observe(void *user,const cui_date_selection_change *e) {
    fixture *f=user;assert(f->calls<16);
    assert(e->event.kind==CUI_COMPONENT_EVENT_CHANGE && e->event.target==10);
    assert(e->event.flags==CUI_COMPONENT_EVENT_HAS_FRAME && e->event.frame_sequence>0);
    assert(e->event.source==CUI_COMPONENT_EVENT_SOURCE_UNKNOWN);
    assert(!cui_date_compare(e->value.start,f->selection.start));
    assert(!cui_date_compare(e->value.end,f->selection.end));
    f->copies[f->calls++]=*e;f->policy_at_callback=f->policy_calls;
    f->request_omit=true; /* Unrelated flag only; apply after outer return. */
    (void)cui_semantics(f->ui,&f->semantic_at_callback);
}
static cui_calendar_event_result frame(fixture *f,bool facade,bool observer) {
    assert(cui_begin_frame(f->ui,1000,700,.016f));
    cui_begin_box(f->ui,1000,(cui_box){.inert=f->inert,.layout_only=f->layout});
    cui_calendar_event_result r={0};cui_theme theme=cui_theme_light();
    cui_date_selection_handler h={observer?observe:NULL,f};
    if(!f->omit) {
        if(f->mode==0) {
            if(facade) r=cui_calendar_events(f->ui,10,&theme,&f->state,&f->selection,f->options,h);
            else r.control=cui_calendar(f->ui,10,&theme,&f->state,&f->selection,f->options);
        } else if(f->mode==1) {
            if(facade) r=cui_base_calendar_events(f->ui,10,&f->state,&f->selection,f->options,h);
            else r.control=cui_base_calendar(f->ui,10,&f->state,&f->selection,f->options);
        } else {
            if(facade) r=cui_calendar_reduce_events(f->ui,10,&f->state,&f->selection,f->options,h);
            else r.control=cui_calendar_reduce(f->ui,10,&f->state,&f->selection,f->options);
            if(r.change_count) assert(!f->semantic_at_callback);
            cui_calendar_submit(f->ui,10,&f->state,f->selection,f->options);
        }
        if(r.change_count && f->mode!=2) assert(f->policy_at_callback==f->policy_calls);
        if(f->mode) {
            size_t n;const cui_semantic *s=cui_semantics(f->ui,&n);
            for(size_t i=0;i<n;++i) {
                cui_begin_box(f->ui,s[i].id,(cui_box){.width=32,.height=24});cui_end_box(f->ui);
            }
        }
    }
    cui_end_box(f->ui);f->draw=cui_end_frame(f->ui);assert(cui_last_error(f->ui)==CUI_OK);
    return r;
}
static void init(fixture *f,unsigned mode) {
    memset(f,0,sizeof *f);f->mode=mode;
    f->ui=cui_create((cui_config){.max_elements=256,.max_commands=512});assert(f->ui);
    f->selection.start=(cui_date){2024,2,15};cui_calendar_init(&f->state,f->selection.start);
    f->options=(cui_calendar_options){.is_disabled=policy,.disabled_user=f};
    frame(f,false,false);cui_focus(f->ui,cui_calendar_day_id(10,f->state.cursor));
}
static void finish(fixture *f) {cui_destroy(f->ui);}
static void key(fixture *f,cui_key k) {
    assert(cui_queue_event(f->ui,(cui_event){.type=CUI_EVENT_KEY_DOWN,.key=k}));
}
static void click(fixture *f,cui_id id) {
    cui_rect b=cui_bounds(f->ui,id);assert(b.w>0 && b.h>0);
    cui_event e={.type=CUI_EVENT_POINTER_DOWN,.x=b.x+b.w/2,.y=b.y+b.h/2};
    assert(cui_queue_event(f->ui,e));e.type=CUI_EVENT_POINTER_UP;assert(cui_queue_event(f->ui,e));
}
static void equal(fixture *a,fixture *b,cui_calendar_result x,cui_calendar_result y) {
    assert(x.changed==y.changed && x.activated==y.activated && x.navigated==y.navigated);
    assert(!cui_date_compare(a->selection.start,b->selection.start));
    assert(!cui_date_compare(a->selection.end,b->selection.end));
    assert(!memcmp(&a->state,&b->state,sizeof a->state));
    assert(a->policy_calls==b->policy_calls && cui_focused(a->ui)==cui_focused(b->ui));
    assert(a->draw.count==b->draw.count);
    for(size_t i=0;i<a->draw.count;++i) {
        const cui_draw_command *p=a->draw.commands+i,*q=b->draw.commands+i;
        assert(p->type==q->type && !memcmp(&p->bounds,&q->bounds,sizeof p->bounds));
        assert(!memcmp(&p->color,&q->color,sizeof p->color) && p->text.length==q->text.length);
        if(p->text.length) assert(!memcmp(p->text.data,q->text.data,p->text.length));
    }
    size_t na,nb;const cui_semantic *sa=cui_semantics(a->ui,&na),*sb=cui_semantics(b->ui,&nb);
    assert(na==nb);
    for(size_t i=0;i<na;++i) assert(sa[i].id==sb[i].id && sa[i].role==sb[i].role &&
        sa[i].details.selected==sb[i].details.selected);
}
static cui_calendar_event_result paired(fixture *a,fixture *b,bool observer) {
    cui_calendar_event_result x=frame(a,false,false),y=frame(b,true,observer);
    equal(a,b,x.control,y.control);return y;
}
static void keyboard(unsigned mode) {
    fixture a,b;init(&a,mode);init(&b,mode);
    key(&a,CUI_KEY_RIGHT);key(&b,CUI_KEY_RIGHT);
    cui_calendar_event_result r=paired(&a,&b,true);
    assert(r.control.navigated && !r.change_count && b.selection.start.day==15);
    key(&a,CUI_KEY_ENTER);key(&b,CUI_KEY_ENTER);r=paired(&a,&b,true);
    assert(r.control.changed && r.change_count==1 && b.selection.start.day==16);
    if(mode!=2) assert(b.semantic_at_callback>0);
    assert(b.request_omit && !b.omit);
    cui_date_selection_change saved=b.copies[0];
    key(&a,CUI_KEY_ENTER);key(&b,CUI_KEY_ENTER);r=paired(&a,&b,true);
    assert(r.control.activated && !r.change_count);
    key(&a,CUI_KEY_RIGHT);key(&b,CUI_KEY_RIGHT);key(&a,CUI_KEY_ENTER);key(&b,CUI_KEY_ENTER);
    key(&a,CUI_KEY_RIGHT);key(&b,CUI_KEY_RIGHT);key(&a,CUI_KEY_ENTER);key(&b,CUI_KEY_ENTER);
    r=paired(&a,&b,true);assert(r.change_count==1 && b.selection.start.day==18);
    key(&a,CUI_KEY_LEFT);key(&b,CUI_KEY_LEFT);key(&a,CUI_KEY_ENTER);key(&b,CUI_KEY_ENTER);
    key(&a,CUI_KEY_RIGHT);key(&b,CUI_KEY_RIGHT);key(&a,CUI_KEY_ENTER);key(&b,CUI_KEY_ENTER);
    r=paired(&a,&b,true);assert(!r.change_count); /* Net-zero selection batch. */
    key(&a,CUI_KEY_RIGHT);key(&b,CUI_KEY_RIGHT);key(&a,CUI_KEY_ENTER);key(&b,CUI_KEY_ENTER);
    r=paired(&a,&b,false);assert(r.control.changed && !r.change_count);
    assert(saved.previous.start.day==15 && saved.value.start.day==16);
    a.options.disabled=b.options.disabled=true;
    key(&a,CUI_KEY_RIGHT);key(&b,CUI_KEY_RIGHT);r=paired(&a,&b,true);assert(!r.change_count);
    a.options.disabled=b.options.disabled=false;a.inert=b.inert=true;
    key(&a,CUI_KEY_ENTER);key(&b,CUI_KEY_ENTER);assert(!paired(&a,&b,true).change_count);
    a.inert=b.inert=false;a.layout=b.layout=true;
    key(&a,CUI_KEY_ENTER);key(&b,CUI_KEY_ENTER);assert(!paired(&a,&b,true).change_count);
    a.layout=b.layout=false;
    /* Passive presentation repair, including disabled maintenance, is not selection. */
    a.state.month=b.state.month=(cui_date){0};a.state.cursor=b.state.cursor=(cui_date){0};
    a.options.disabled=b.options.disabled=true;
    assert(!paired(&a,&b,true).change_count && cui_date_valid(b.state.month));
    a.omit=b.omit=b.request_omit;assert(!paired(&a,&b,true).change_count);
    finish(&a);finish(&b);
    assert(saved.previous.start.day==15 && saved.value.start.day==16);
}
static void range_and_chooser(void) {
    fixture a,b;init(&a,0);init(&b,0);a.options.range=b.options.range=true;
    click(&a,cui_calendar_day_id(10,(cui_date){2024,2,20}));
    click(&b,cui_calendar_day_id(10,(cui_date){2024,2,20}));
    assert(paired(&a,&b,true).change_count==1 && b.state.selecting_end);
    click(&a,cui_calendar_day_id(10,(cui_date){2024,2,10}));
    click(&b,cui_calendar_day_id(10,(cui_date){2024,2,10}));
    assert(paired(&a,&b,true).change_count==1 && !b.state.selecting_end);
    assert(b.selection.start.day==10 && b.selection.end.day==20);
    click(&a,cui_calendar_navigation_id(10,5));click(&b,cui_calendar_navigation_id(10,5));
    assert(!paired(&a,&b,true).change_count && b.state.view==CUI_CALENDAR_YEARS);
    click(&a,cui_calendar_year_id(10,2024));click(&b,cui_calendar_year_id(10,2024));
    assert(!paired(&a,&b,true).change_count && b.state.view==CUI_CALENDAR_MONTHS);
    click(&a,cui_calendar_month_id(10,3));click(&b,cui_calendar_month_id(10,3));
    assert(!paired(&a,&b,true).change_count && b.state.month.month==3);
    a.deny_weekends=b.deny_weekends=true;
    a.options.minimum=b.options.minimum=(cui_date){2024,3,1};
    a.options.maximum=b.options.maximum=(cui_date){2024,3,20};
    paired(&a,&b,true);
    click(&a,cui_calendar_day_id(10,(cui_date){2024,3,2}));
    click(&b,cui_calendar_day_id(10,(cui_date){2024,3,2}));
    assert(!paired(&a,&b,true).change_count && b.calls==2);
    finish(&a);finish(&b);
}
static void admission(void) {
    fixture f;init(&f,0);cui_calendar_state state=f.state;cui_date_selection value=f.selection;
    assert(cui_begin_frame(f.ui,1000,700,.016f));
    cui_date_selection_handler h={observe,&f};
    assert(!cui_calendar_reduce_events(f.ui,0,&f.state,&f.selection,f.options,h).change_count);
    assert(!cui_base_calendar_events(f.ui,10,NULL,&f.selection,f.options,h).change_count);
    assert(!cui_calendar_events(f.ui,10,NULL,&f.state,&f.selection,f.options,h).change_count);
    assert(!memcmp(&state,&f.state,sizeof state) && !memcmp(&value,&f.selection,sizeof value));
    assert(!f.calls);cui_end_frame(f.ui);finish(&f);
}
int main(void) {
    for(unsigned mode=0;mode<3;++mode) keyboard(mode);
    range_and_chooser();admission();puts("PASS Calendar selection observers, legacy equivalence and navigation silence");return 0;
}
