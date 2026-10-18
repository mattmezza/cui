/* SPDX-License-Identifier: MIT */
#include "cui/cui_list_events.h"
#include "cui/cui_components.h"
#include <assert.h>
#include <string.h>
static const cui_choice_item items[]={{10,{"Alpha",5},false},{20,{"Disabled",8},true},{30,{"Gamma",5},false}};
typedef struct {
    cui_context *ui;
    cui_id selected;
    cui_list_state state;
    cui_selection_change copies[32];
    cui_choice_item_change item_copies[32];
    bool multi, values[3], activation_value;
    size_t calls, trace_length;
    char trace[128];
    bool redirect, consume, cancel_seen, deferred_remove;
    cui_draw_list draw;
} fixture;
static void init(fixture *f) {
    memset(f,0,sizeof *f);f->selected=10;
    f->ui=cui_create((cui_config){.width=600,.height=300});assert(f->ui);
}
static void trace(fixture *f,char c) {assert(f->trace_length+1<sizeof f->trace);f->trace[f->trace_length++]=c;}
static void activated(cui_id item,void *user) {
    fixture *f=user;
    if(f->multi) f->activation_value=f->values[item==10?0:2];
    else assert(f->selected==item);
    trace(f,'A');
}
static void confirmed(cui_id item,bool secondary,void *user) {
    fixture *f=user;assert(item==10||item==30);(void)secondary;trace(f,'C');
    if(f->multi) {
        assert(f->values[item==10?0:2]==f->activation_value);
        if(f->redirect) f->values[item==10?0:2]=false;
    } else if(f->redirect) f->selected=10; /* Existing legacy owner-update timing. */
}
static void cancelled(void *user) {fixture *f=user;trace(f,'X');f->cancel_seen=true;}
static void changed(void *user,const cui_selection_change *event) {
    fixture *f=user;assert(f->calls<32 && f->trace_length>=2 && f->trace[f->trace_length-1]=='C');
    assert(event->event.target==90 && event->event.kind==CUI_COMPONENT_EVENT_CHANGE);
    assert(event->event.flags&CUI_COMPONENT_EVENT_HAS_FRAME);
    assert(event->event.flags&CUI_COMPONENT_EVENT_HAS_INDEX);
    assert(event->event.frame_sequence==cui_frame_sequence(f->ui));
    assert(event->value==f->selected && event->previous!=event->value);
    f->copies[f->calls++]=*event;trace(f,'T');f->deferred_remove=true;
    if(f->consume) {
        size_t count;cui_frame_events(f->ui,&count);
        for(size_t i=event->event.event_index+1;i<count;++i) cui_consume_event(f->ui,i);
    }
}
static cui_list_event_result frame(fixture *f,bool styled,bool legacy,bool handler,unsigned passive,cui_list_options o) {
    assert(cui_begin_frame(f->ui,600,300,.016f));
    cui_begin_box(f->ui,900,(cui_box){.width=500,.height=250,.inert=passive==1,.layout_only=passive==2});
    o.on_activate=activated;o.on_confirm=confirmed;o.on_cancel=cancelled;o.activation_user=f;
    cui_theme t=cui_theme_light();cui_list_event_result r={0};
    if(legacy) r.legacy_activated=styled?cui_list(f->ui,90,&t,items,3,&f->selected,&f->state,o)
        :cui_base_list(f->ui,items,3,&f->selected,&f->state,o);
    else {cui_selection_handler h={handler?changed:NULL,f};
        r=styled?cui_list_events(f->ui,90,&t,items,3,&f->selected,&f->state,o,h)
            :cui_base_list_events(f->ui,90,items,3,&f->selected,&f->state,o,h);}
    cui_end_box(f->ui);f->draw=cui_end_frame(f->ui);assert(cui_last_error(f->ui)==CUI_OK);return r;
}
static void key(fixture *f,cui_key code) {assert(cui_queue_event(f->ui,(cui_event){.type=CUI_EVENT_KEY_DOWN,.key=code}));}
static void click(fixture *f,cui_id item) {
    cui_rect b=cui_bounds(f->ui,item);assert(b.w>0&&b.h>0);
    assert(cui_queue_event(f->ui,(cui_event){.type=CUI_EVENT_POINTER_DOWN,.x=b.x+2,.y=b.y+2}));
    assert(cui_queue_event(f->ui,(cui_event){.type=CUI_EVENT_POINTER_UP,.x=b.x+2,.y=b.y+2}));
}
static void compare(fixture *a,fixture *b) {
    assert(!memcmp(a->values,b->values,sizeof a->values));
    assert(a->selected==b->selected && a->state.active==b->state.active && a->state.first_visible==b->state.first_visible &&
        a->state.search_length==b->state.search_length && !memcmp(a->state.search,b->state.search,sizeof a->state.search) &&
        a->state.secondary_active==b->state.secondary_active && cui_focused(a->ui)==cui_focused(b->ui));
    char legacy_trace[128]={0};size_t trace_count=0;
    for(size_t i=0;i<b->trace_length;++i) if(b->trace[i]!='T') legacy_trace[trace_count++]=b->trace[i];
    assert(!strcmp(a->trace,legacy_trace));
    size_t an,bn;const cui_semantic *as=cui_semantics(a->ui,&an),*bs=cui_semantics(b->ui,&bn);
    assert(an==bn && a->draw.count==b->draw.count);
    for(size_t i=0;i<an;++i) {
        assert(as[i].id==bs[i].id && as[i].role==bs[i].role && as[i].disabled==bs[i].disabled &&
            as[i].value==bs[i].value && as[i].details.selected==bs[i].details.selected);
        cui_rect ar=cui_bounds(a->ui,as[i].id),br=cui_bounds(b->ui,bs[i].id);
        assert(ar.x==br.x && ar.y==br.y && ar.w==br.w && ar.h==br.h);
    }
    for(size_t i=0;i<a->draw.count;++i) {
        const cui_draw_command *ac=&a->draw.commands[i],*bc=&b->draw.commands[i];
        assert(ac->type==bc->type && ac->bounds.x==bc->bounds.x && ac->bounds.y==bc->bounds.y &&
            ac->bounds.w==bc->bounds.w && ac->bounds.h==bc->bounds.h && ac->text.length==bc->text.length &&
            ac->color.r==bc->color.r && ac->color.g==bc->color.g && ac->color.b==bc->color.b && ac->color.a==bc->color.a);
        if(ac->text.length)assert(!memcmp(ac->text.data,bc->text.data,ac->text.length));
    }
}
static void single(bool styled) {
    fixture f;init(&f);cui_list_options o={0};frame(&f,styled,false,true,0,o);cui_focus(f.ui,10);
    key(&f,CUI_KEY_DOWN);key(&f,CUI_KEY_ENTER);key(&f,CUI_KEY_UP);key(&f,CUI_KEY_ENTER);
    cui_list_event_result r=frame(&f,styled,false,true,0,o);
    assert(r.events.valid && r.events.submitted && r.legacy_activated && r.events.change_count==2 && f.selected==10);
    assert(!strcmp(f.trace,"ACTACT") && f.copies[0].previous==10 && f.copies[0].value==30 && f.copies[1].previous==30);
    key(&f,CUI_KEY_ENTER);r=frame(&f,styled,false,true,0,o);
    assert(r.legacy_activated && !r.events.change_count && !strcmp(f.trace,"ACTACTAC"));
    f.redirect=true;key(&f,CUI_KEY_DOWN);key(&f,CUI_KEY_ENTER);
    r=frame(&f,styled,false,true,0,o);assert(r.legacy_activated&&!r.events.change_count&&f.selected==10);
    f.redirect=false;key(&f,CUI_KEY_ENTER);r=frame(&f,styled,false,false,0,o);
    assert(r.legacy_activated&&!r.events.change_count&&f.selected==30&&f.calls==2);
    key(&f,CUI_KEY_ESCAPE);r=frame(&f,styled,false,true,0,o);assert(!r.legacy_activated&&f.cancel_seen);
    assert(f.deferred_remove);cui_destroy(f.ui);
    init(&f);frame(&f,styled,false,true,0,o);
    assert(!cui_queue_event(f.ui,(cui_event){.type=CUI_EVENT_ACCESSIBILITY_ACTION,.accessibility_target=30,.accessibility_action=CUI_ACCESSIBILITY_ACTIVATE}));
    assert(cui_last_error(f.ui)==CUI_ERROR_INVALID_ACTION);cui_clear_error(f.ui);
    cui_destroy(f.ui);
    init(&f);o.menu=true;frame(&f,styled,false,true,0,o);f.state.active=30;
    assert(cui_queue_event(f.ui,(cui_event){.type=CUI_EVENT_ACCESSIBILITY_ACTION,.accessibility_target=30,.accessibility_action=CUI_ACCESSIBILITY_ACTIVATE}));
    assert(frame(&f,styled,false,true,0,o).events.change_count==1);
    assert(f.copies[0].event.source==CUI_COMPONENT_EVENT_SOURCE_ACCESSIBILITY);cui_destroy(f.ui);
    if(styled) {
        init(&f);o=(cui_list_options){0};frame(&f,true,false,true,0,o);click(&f,30);click(&f,10);f.consume=true;
        assert(frame(&f,true,false,true,0,o).events.change_count==1&&f.selected==30);
        assert(f.copies[0].event.source==CUI_COMPONENT_EVENT_SOURCE_POINTER);cui_destroy(f.ui);
        fixture a,b;init(&a);init(&b);o=(cui_list_options){.visible_count=2};
        frame(&a,true,true,false,0,o);frame(&b,true,false,true,0,o);
        click(&a,10);click(&b,10);o.window_changed=true;
        cui_list_event_result ar=frame(&a,true,true,false,0,o),br=frame(&b,true,false,true,0,o);
        compare(&a,&b);assert(!ar.legacy_activated&&!br.legacy_activated&&!br.events.change_count);
        cui_destroy(a.ui);cui_destroy(b.ui);
    }
    for(unsigned passive=0;passive<3;++passive)for(unsigned disabled=0;disabled<2;++disabled) {
        fixture a,b;init(&a);init(&b);o=(cui_list_options){0};
        frame(&a,styled,true,false,0,o);frame(&b,styled,false,true,0,o);cui_focus(a.ui,10);cui_focus(b.ui,10);
        key(&a,CUI_KEY_DOWN);key(&a,CUI_KEY_ENTER);key(&b,CUI_KEY_DOWN);key(&b,CUI_KEY_ENTER);o.disabled=disabled!=0;
        cui_list_event_result ar=frame(&a,styled,true,false,passive,o),br=frame(&b,styled,false,true,passive,o);
        compare(&a,&b);assert(ar.legacy_activated==br.legacy_activated);
        assert(br.events.change_count==((!passive&&!disabled)?1u:0u));
        cui_destroy(a.ui);cui_destroy(b.ui);
    }
}
static void item_changed(void *user,const cui_choice_item_change *event) {
    fixture *f=user;assert(f->calls<32 && f->trace_length>=2 && f->trace[f->trace_length-1]=='C');
    assert(event->event.target==90 && event->event.kind==CUI_COMPONENT_EVENT_CHANGE);
    assert(event->event.flags&CUI_COMPONENT_EVENT_HAS_INDEX);
    assert(event->event.frame_sequence==cui_frame_sequence(f->ui));
    assert(event->item==event->trigger_item && event->previous!=event->value);
    assert(f->values[event->item==10?0:2]==event->value);
    f->item_copies[f->calls++]=*event;trace(f,'T');f->deferred_remove=true;
    if(f->consume) {
        size_t count;cui_frame_events(f->ui,&count);
        for(size_t i=event->event.event_index+1;i<count;++i)cui_consume_event(f->ui,i);
    }
}
static cui_choice_items_event_result multi_frame(fixture *f,bool styled,bool legacy,bool handler,unsigned passive,cui_list_options o) {
    assert(cui_begin_frame(f->ui,600,300,.016f));
    cui_begin_box(f->ui,900,(cui_box){.width=500,.height=250,.inert=passive==1,.layout_only=passive==2});
    o.on_activate=activated;o.on_confirm=confirmed;o.on_cancel=cancelled;o.activation_user=f;
    cui_theme t=cui_theme_light();cui_choice_items_event_result r={0};f->multi=true;
    if(legacy)r.legacy_result=styled?cui_list_multi(f->ui,90,&t,items,3,f->values,&f->state,o)
        :cui_base_list_multi(f->ui,items,3,f->values,&f->state,o);
    else {cui_choice_item_handler h={handler?item_changed:NULL,f};
        r=styled?cui_list_multi_events(f->ui,90,&t,items,3,f->values,&f->state,o,h)
            :cui_base_list_multi_events(f->ui,90,items,3,f->values,&f->state,o,h);}
    cui_end_box(f->ui);f->draw=cui_end_frame(f->ui);assert(cui_last_error(f->ui)==CUI_OK);return r;
}
static void multi(bool styled) {
    fixture f;init(&f);cui_list_options o={0};multi_frame(&f,styled,false,true,0,o);cui_focus(f.ui,10);
    key(&f,CUI_KEY_ENTER);key(&f,CUI_KEY_ENTER);
    cui_choice_items_event_result r=multi_frame(&f,styled,false,true,0,o);
    assert(r.valid&&r.submitted&&r.legacy_result&&r.change_count==2&&!f.values[0]);
    assert(!strcmp(f.trace,"ACTACT")&&!f.item_copies[0].previous&&f.item_copies[0].value&&
        f.item_copies[1].previous&&!f.item_copies[1].value);
    f.redirect=true;key(&f,CUI_KEY_ENTER);
    r=multi_frame(&f,styled,false,true,0,o);assert(r.legacy_result&&!r.change_count&&!f.values[0]);
    f.redirect=false;key(&f,CUI_KEY_ENTER);
    r=multi_frame(&f,styled,false,false,0,o);assert(r.legacy_result&&!r.change_count&&f.values[0]&&f.calls==2);
    key(&f,CUI_KEY_ESCAPE);assert(!multi_frame(&f,styled,false,true,0,o).legacy_result&&f.cancel_seen);
    assert(f.deferred_remove);cui_destroy(f.ui);
    if(styled) {
        init(&f);multi_frame(&f,true,false,true,0,o);click(&f,10);click(&f,10);f.consume=true;
        assert(multi_frame(&f,true,false,true,0,o).change_count==1&&f.values[0]);cui_destroy(f.ui);
    }
    for(unsigned passive=0;passive<3;++passive)for(unsigned disabled=0;disabled<2;++disabled) {
        fixture a,b;init(&a);init(&b);o=(cui_list_options){0};
        multi_frame(&a,styled,true,false,0,o);multi_frame(&b,styled,false,true,0,o);
        cui_focus(a.ui,10);cui_focus(b.ui,10);key(&a,CUI_KEY_ENTER);key(&b,CUI_KEY_ENTER);o.disabled=disabled!=0;
        cui_choice_items_event_result ar=multi_frame(&a,styled,true,false,passive,o),br=multi_frame(&b,styled,false,true,passive,o);
        compare(&a,&b);assert(ar.legacy_result==br.legacy_result);
        assert(br.change_count==((!passive&&!disabled)?1u:0u));cui_destroy(a.ui);cui_destroy(b.ui);
    }
}
static void invalid_control(void) {
    fixture f;init(&f);cui_theme t=cui_theme_light();assert(cui_begin_frame(f.ui,600,300,.016f));
    assert(!cui_list_events(f.ui,0,&t,items,3,&f.selected,&f.state,(cui_list_options){0},(cui_selection_handler){0}).events.valid);
    assert(!cui_list_multi_events(f.ui,0,&t,items,3,f.values,&f.state,(cui_list_options){0},(cui_choice_item_handler){0}).valid);
    assert(cui_end_frame(f.ui).count==0 && cui_last_error(f.ui)==CUI_OK);
    cui_destroy(f.ui);
}
int main(void){single(false);single(true);multi(false);multi(true);invalid_control();return 0;}
