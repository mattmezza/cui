/* SPDX-License-Identifier: MIT */
#include "cui/cui_tabs_events.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct {
    cui_context *ui;
    cui_id selected;
    cui_choice_item items[3];
    cui_tabs_options options;
    cui_tab_scroll_state scroll;
    cui_tab_drag_state drag;
    cui_tab_overflow overflow;
    cui_menu_entry scratch[3];
    cui_tabs_selection_change copies[16];
    size_t calls, panels, panel_index, calls_at_panel;
    bool inert, layout, omit, consume;
    cui_draw_list draw;
} fixture;
static void changed(void *user,const cui_tabs_selection_change *e) {
    fixture *f=user;assert(f->calls<16);
    assert(e->event.kind==CUI_COMPONENT_EVENT_CHANGE && e->event.target==1);
    assert(e->event.flags==CUI_COMPONENT_EVENT_HAS_FRAME && e->event.frame_sequence>0);
    assert(e->event.source==CUI_COMPONENT_EVENT_SOURCE_UNKNOWN);
    assert(e->value==f->selected && e->previous!=e->value);
    f->copies[f->calls++]=*e;
}
static void panel(cui_context *ui,size_t index,void *user) {
    fixture *f=user;++f->panels;f->panel_index=index;f->calls_at_panel=f->calls;
    cui_text(ui,cui_string("Panel"),14,(cui_color){255,255,255,255});
}
static cui_tabs_event_result frame(fixture *f,bool facade,bool observe) {
    assert(cui_begin_frame(f->ui,640,480,.016f));
    cui_theme theme=cui_theme_light();cui_tabs_event_result result={0};
    if(f->consume) cui_consume_event(f->ui,0);
    cui_begin_box(f->ui,1000,(cui_box){.inert=f->inert,.layout_only=f->layout});
    if(!f->omit) {
        if(facade) result=cui_tabs_events(f->ui,1,&theme,f->items,3,&f->selected,
            f->options,panel,f,(cui_tabs_handler){observe?changed:NULL,f});
        else result.changed=cui_tabs(f->ui,1,&theme,f->items,3,&f->selected,f->options,panel,f);
    }
    cui_end_box(f->ui);f->draw=cui_end_frame(f->ui);assert(cui_last_error(f->ui)==CUI_OK);
    return result;
}
static void init(fixture *f) {
    memset(f,0,sizeof *f);
    f->ui=cui_create((cui_config){.max_elements=128,.max_commands=256});assert(f->ui);
    f->selected=10;
    f->items[0]=(cui_choice_item){10,cui_string("One long tab"),false};
    f->items[1]=(cui_choice_item){20,cui_string("Disabled long tab"),true};
    f->items[2]=(cui_choice_item){30,cui_string("Three long tab"),false};
    frame(f,false,false);cui_focus(f->ui,10);
}
static void finish(fixture *f) {cui_destroy(f->ui);}
static void key(fixture *f,cui_key key) {
    assert(cui_queue_event(f->ui,(cui_event){.type=CUI_EVENT_KEY_DOWN,.key=key}));
}
static void click(fixture *f,cui_id id) {
    cui_rect b=cui_bounds(f->ui,id);assert(b.w>0 && b.h>0);
    cui_event e={.type=CUI_EVENT_POINTER_DOWN,.x=b.x+b.w/2,.y=b.y+b.h/2,
                 .pointer_button=CUI_POINTER_PRIMARY};
    assert(cui_queue_event(f->ui,e));e.type=CUI_EVENT_POINTER_UP;assert(cui_queue_event(f->ui,e));
}
static void equal(fixture *a,fixture *b,cui_tabs_event_result x,cui_tabs_event_result y) {
    assert(x.changed==y.changed && a->selected==b->selected);
    assert(cui_focused(a->ui)==cui_focused(b->ui));
    assert(a->panels==b->panels && a->panel_index==b->panel_index);
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
        sa[i].details.selected==sb[i].details.selected && sa[i].details.controls==sb[i].details.controls);
}
static cui_tabs_event_result paired(fixture *a,fixture *b,bool observe) {
    cui_tabs_event_result x=frame(a,false,false),y=frame(b,true,observe);equal(a,b,x,y);return y;
}
static void selection(void) {
    fixture a,b;init(&a);init(&b);
    key(&a,CUI_KEY_RIGHT);key(&b,CUI_KEY_RIGHT);
    cui_tabs_event_result r=paired(&a,&b,true);
    assert(r.changed && r.change_count==1 && b.selected==30 && b.calls_at_panel==1);
    cui_tabs_selection_change saved=b.copies[0];
    click(&a,30);click(&b,30);r=paired(&a,&b,true);assert(!r.changed && !r.change_count);
    key(&a,CUI_KEY_LEFT);key(&b,CUI_KEY_LEFT);key(&a,CUI_KEY_RIGHT);key(&b,CUI_KEY_RIGHT);
    r=paired(&a,&b,true);assert(!r.changed && !r.change_count); /* Coalesced net-zero. */
    key(&a,CUI_KEY_LEFT);key(&b,CUI_KEY_LEFT);key(&a,CUI_KEY_RIGHT);key(&b,CUI_KEY_RIGHT);
    key(&a,CUI_KEY_LEFT);key(&b,CUI_KEY_LEFT);r=paired(&a,&b,true);
    assert(r.change_count==1 && b.selected==10); /* Several writes, one final change. */
    key(&a,CUI_KEY_RIGHT);key(&b,CUI_KEY_RIGHT);paired(&a,&b,true);
    a.consume=b.consume=true;key(&a,CUI_KEY_LEFT);key(&b,CUI_KEY_LEFT);
    assert(!paired(&a,&b,true).change_count && b.selected==30);
    a.consume=b.consume=false;
    a.options.manual_activation=b.options.manual_activation=true;
    key(&a,CUI_KEY_LEFT);key(&b,CUI_KEY_LEFT);r=paired(&a,&b,true);
    assert(!r.change_count && cui_focused(b.ui)==10 && b.selected==30);
    key(&a,CUI_KEY_ENTER);key(&b,CUI_KEY_ENTER);r=paired(&a,&b,false);
    assert(r.changed && !r.change_count && b.selected==10); /* Null observer still writes. */
    assert(saved.previous==10 && saved.value==30 && saved.event.target==1);
    a.options.disabled=b.options.disabled=true;
    click(&a,30);click(&b,30);r=paired(&a,&b,true);assert(!r.change_count);
    a.options.disabled=b.options.disabled=false;
    a.inert=b.inert=true;key(&a,CUI_KEY_RIGHT);key(&b,CUI_KEY_RIGHT);
    r=paired(&a,&b,true);assert(!r.change_count);
    a.inert=b.inert=false;a.layout=b.layout=true;
    key(&a,CUI_KEY_RIGHT);key(&b,CUI_KEY_RIGHT);r=paired(&a,&b,true);assert(!r.change_count);
    a.layout=b.layout=false;
    a.selected=b.selected=999;r=paired(&a,&b,true);assert(!r.change_count && b.selected==999);
    a.omit=b.omit=true;r=paired(&a,&b,true);assert(!r.change_count);
    size_t n;const cui_semantic *s=cui_semantics(b.ui,&n);
    for(size_t i=0;i<n;++i) assert(s[i].role!=CUI_ROLE_TAB);
    finish(&a);finish(&b);
}
static void overflow(void) {
    fixture a,b;init(&a);init(&b);fixture *fs[2]={&a,&b};
    for(size_t i=0;i<2;++i) {
        fixture *f=fs[i];cui_tab_overflow_init(&f->overflow,3100,3101,f->scratch,3);
        f->overflow.options.popup.viewport=(cui_rect){0,0,640,480};
        f->overflow.options.popup.width=220;
        f->options=(cui_tabs_options){.scroll=&f->scroll,.bar_extent=110,.max_width=100,
                                     .overflow=&f->overflow,.drag=&f->drag};
    }
    paired(&a,&b,true);assert(b.overflow.visible);
    click(&a,3101);click(&b,3101);assert(!paired(&a,&b,true).change_count);
    assert(b.overflow.menu.overlay.open);
    key(&a,CUI_KEY_DOWN);key(&b,CUI_KEY_DOWN);key(&a,CUI_KEY_ENTER);key(&b,CUI_KEY_ENTER);
    cui_tabs_event_result r=paired(&a,&b,true);
    assert(r.change_count==1 && b.selected==30 && !b.overflow.menu.overlay.open);
    a.options.disabled=b.options.disabled=true;
    a.drag.active=b.drag.active=true;a.drag.candidate=b.drag.candidate=30;
    assert(!paired(&a,&b,true).change_count && !a.drag.active && !b.drag.active);
    finish(&a);finish(&b);
}
static void drag_only(void) {
    fixture a,b;init(&a);init(&b);
    a.options.drag=&a.drag;b.options.drag=&b.drag;paired(&a,&b,true);
    cui_rect first=cui_bounds(b.ui,10),last=cui_bounds(b.ui,30);
    cui_event down={.type=CUI_EVENT_POINTER_DOWN,.x=first.x+2,.y=first.y+2};
    assert(cui_queue_event(a.ui,down) && cui_queue_event(b.ui,down));
    assert(!paired(&a,&b,true).change_count);
    cui_event move={.type=CUI_EVENT_POINTER_MOVE,.x=last.x+last.w-2,.y=last.y+2};
    assert(cui_queue_event(a.ui,move) && cui_queue_event(b.ui,move));
    assert(!paired(&a,&b,true).change_count && b.drag.active);
    move.type=CUI_EVENT_POINTER_UP;
    assert(cui_queue_event(a.ui,move) && cui_queue_event(b.ui,move));
    assert(!paired(&a,&b,true).change_count && b.drag.requested && b.drag.moved==10);
    assert(b.selected==10 && !b.calls);finish(&a);finish(&b);
}
static void base_maintenance(void) {
    fixture f;init(&f);cui_tab_drag_state drag={.active=true,.candidate=10,.requested=true};
    cui_tabs_options o={.disabled=true,.drag=&drag};
    assert(cui_begin_frame(f.ui,640,480,.016f));
    cui_tabs_event_result r=cui_base_tabs_events(f.ui,1,f.items,3,&f.selected,o,
                                               (cui_tabs_handler){changed,&f});
    assert(!r.change_count && !drag.active && !drag.requested);
    cui_end_frame(f.ui);
    /* Empty-model maintenance remains live even without selected changes. */
    cui_tab_overflow_init(&f.overflow,3100,3101,f.scratch,3);f.overflow.visible=true;
    assert(cui_begin_frame(f.ui,640,480,.016f));
    r=cui_base_tabs_events(f.ui,1,NULL,0,&f.selected,(cui_tabs_options){.overflow=&f.overflow},
                          (cui_tabs_handler){NULL,(void *)1});
    assert(!r.changed && !r.change_count && !f.overflow.visible);
    r=cui_base_tabs_events(f.ui,0,f.items,3,&f.selected,(cui_tabs_options){0},
                          (cui_tabs_handler){changed,&f});assert(!r.changed && !r.change_count);
    cui_end_frame(f.ui);finish(&f);
}
int main(void) {selection();overflow();drag_only();base_maintenance();puts("PASS aggregate Tabs selection and legacy maintenance");return 0;}
