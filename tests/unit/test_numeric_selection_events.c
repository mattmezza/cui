/* SPDX-License-Identifier: MIT */
#include "cui/cui_numeric_selection_events.h"
#include "cui/cui_components.h"
#include <assert.h>
#include <string.h>
typedef struct {
    cui_context *ui;
    size_t value, calls;
    cui_pagination_change_event page_event;
    cui_stepper_change_event step_event;
    cui_draw_list draw;
    bool teardown_requested;
} fixture;
static fixture make(void) {
    fixture f = {0};
    f.ui = cui_create((cui_config){.width = 1400, .height = 800});
    assert(f.ui); return f;
}
static void metadata(fixture *f, const cui_component_event_metadata *e) {
    assert(e->kind == CUI_COMPONENT_EVENT_CHANGE && e->target == 2);
    assert(e->source == CUI_COMPONENT_EVENT_SOURCE_UNKNOWN);
    assert(e->flags == CUI_COMPONENT_EVENT_HAS_FRAME);
    assert(e->frame_sequence == cui_frame_sequence(f->ui));
}
static void page_changed(void *user, const cui_pagination_change_event *e) {
    fixture *f = user; metadata(f, &e->event);
    assert(e->after == f->value);
    f->page_event = *e; f->calls++; f->teardown_requested = true;
}
static void key(fixture *f, cui_id id, cui_key code) {
    cui_focus(f->ui,id);
    assert(cui_queue_event(f->ui,(cui_event){.type=CUI_EVENT_KEY_DOWN,.key=code}));
}
static void activate(fixture *f, unsigned part) {
    assert(cui_queue_event(f->ui,(cui_event){.type=CUI_EVENT_ACCESSIBILITY_ACTION,
        .accessibility_target=cui_numeric_part_id(2,part),.accessibility_action=CUI_ACCESSIBILITY_ACTIVATE}));
}
static void click(fixture *f, unsigned part, float fraction) {
    cui_rect b = cui_bounds(f->ui,cui_numeric_part_id(2,part));
    assert(b.w > 0 && b.h > 0);
    cui_event e = {.type=CUI_EVENT_POINTER_DOWN,.x=b.x+b.w*fraction,.y=b.y+b.h*.5f};
    assert(cui_queue_event(f->ui,e)); e.type=CUI_EVENT_POINTER_UP;
    assert(cui_queue_event(f->ui,e));
}
static cui_numeric_selection_event_result page_frame(fixture *f, bool styled, bool legacy,
    size_t total, bool compact, bool disabled, unsigned passive, bool handler) {
    assert(cui_begin_frame(f->ui,1400,800,.016f));
    cui_begin_box(f->ui,900,(cui_box){.width=1300,.height=700,.inert=passive==1,.layout_only=passive==2});
    cui_theme t = cui_theme_light(); cui_numeric_selection_event_result r = {0};
    if (legacy) r.control = styled ? cui_pagination(f->ui,2,&t,&f->value,total,compact,disabled)
        : cui_base_pagination(f->ui,2,&f->value,total,compact,disabled);
    else {
        cui_pagination_event_handler h = {handler ? page_changed : NULL,f};
        r = styled ? cui_pagination_events(f->ui,2,&t,&f->value,total,compact,disabled,h)
            : cui_base_pagination_events(f->ui,2,&f->value,total,compact,disabled,h);
    }
    cui_end_box(f->ui); f->draw=cui_end_frame(f->ui);
    assert(cui_last_error(f->ui)==CUI_OK); return r;
}
static void compare(fixture *a, fixture *b) {
    assert(a->value==b->value && cui_focused(a->ui)==cui_focused(b->ui));
    cui_rect x=cui_bounds(a->ui,2),y=cui_bounds(b->ui,2);
    assert(x.x==y.x && x.y==y.y && x.w==y.w && x.h==y.h);
    size_t an,bn; const cui_semantic *as=cui_semantics(a->ui,&an),*bs=cui_semantics(b->ui,&bn);
    assert(an==bn && a->draw.count==b->draw.count);
    for(size_t i=0;i<an;++i) {
        assert(as[i].id==bs[i].id && as[i].role==bs[i].role && as[i].value==bs[i].value &&
            as[i].disabled==bs[i].disabled && as[i].details.selected==bs[i].details.selected &&
            as[i].details.readonly==bs[i].details.readonly);
        cui_rect ab=cui_bounds(a->ui,as[i].id),bb=cui_bounds(b->ui,bs[i].id);
        assert(ab.x==bb.x && ab.y==bb.y && ab.w==bb.w && ab.h==bb.h);
    }
    for(size_t i=0;i<a->draw.count;++i) {
        const cui_draw_command *ac=&a->draw.commands[i],*bc=&b->draw.commands[i];
        assert(ac->type==bc->type && ac->bounds.x==bc->bounds.x && ac->bounds.y==bc->bounds.y &&
            ac->bounds.w==bc->bounds.w && ac->bounds.h==bc->bounds.h &&
            ac->color.r==bc->color.r && ac->color.g==bc->color.g && ac->color.b==bc->color.b &&
            ac->color.a==bc->color.a && ac->radius==bc->radius && ac->stroke==bc->stroke &&
            ac->font_size==bc->font_size && ac->text.length==bc->text.length);
        if(ac->text.length) assert(!memcmp(ac->text.data,bc->text.data,ac->text.length));
    }
}
static void pagination(bool styled) {
    fixture f=make(); f.value=50;
    assert(!page_frame(&f,styled,false,100,false,false,0,true).control);
    key(&f,cui_numeric_part_id(2,3),CUI_KEY_ENTER);
    assert(page_frame(&f,styled,false,100,false,false,0,true).change_count==1 && f.value==51);
    assert(f.page_event.before==50 && f.page_event.after==51 && f.page_event.total==100);
    activate(&f,4); activate(&f,2);
    assert(page_frame(&f,styled,false,100,false,false,0,true).change_count==1 && f.value==99);
    activate(&f,2); activate(&f,3);
    assert(!page_frame(&f,styled,false,100,false,false,0,true).control && f.value==99);
    assert(f.calls==2);
    activate(&f,4);
    assert(!page_frame(&f,styled,false,100,false,false,0,false).change_count && f.value==100 && f.calls==2);
    f.value=SIZE_MAX;
    assert(page_frame(&f,styled,false,SIZE_MAX,true,false,0,true).change_count==0);
    activate(&f,2);
    assert(page_frame(&f,styled,false,SIZE_MAX,true,false,0,true).change_count==1);
    assert(f.page_event.before==SIZE_MAX && f.page_event.after==SIZE_MAX-1 && f.page_event.total==SIZE_MAX);
    assert(page_frame(&f,styled,false,0,true,false,0,true).change_count==1 && f.value==0);
    assert(f.teardown_requested); /* Actual destruction deferred until outer calls completed. */
    cui_destroy(f.ui);
    if(styled) {
        f=make(); f.value=50; page_frame(&f,true,false,100,false,false,0,true);
        click(&f,4,.5f); click(&f,2,.5f);
        assert(page_frame(&f,true,false,100,false,false,0,true).change_count==1 && f.value==99);
        cui_destroy(f.ui);
    }
    for(unsigned passive=0;passive<3;++passive) for(unsigned disabled=0;disabled<2;++disabled) {
        fixture a=make(),b=make(); a.value=b.value=SIZE_MAX;
        cui_numeric_selection_event_result ar=page_frame(&a,styled,true,100,false,disabled!=0,passive,false);
        cui_numeric_selection_event_result br=page_frame(&b,styled,false,100,false,disabled!=0,passive,true);
        compare(&a,&b); assert(a.value==100 && ar.control==br.control);
        assert(br.change_count==((!passive&&!disabled)?1u:0u));
        cui_destroy(a.ui); cui_destroy(b.ui);
    }
}
static const cui_step steps[] = {
    {.title={"Start",5},.status=CUI_STEP_ACTIVE},
    {.title={"Payment",7},.description={"Details",7},.status=CUI_STEP_ERROR},
    {.title={"Finish",6},.status=CUI_STEP_COMPLETE,.disabled=true}
};
static void step_changed(void *user, const cui_stepper_change_event *e) {
    fixture *f=user; metadata(f,&e->event);
    assert(e->after==f->value); f->step_event=*e; f->calls++;
    f->teardown_requested=true;
}
static cui_numeric_selection_event_result step_frame(fixture *f,bool styled,bool legacy,
    const cui_step *items,size_t count,bool disabled,unsigned passive,bool handler,bool vertical) {
    assert(cui_begin_frame(f->ui,1400,800,.016f));
    cui_begin_box(f->ui,900,(cui_box){.width=1300,.height=700,.inert=passive==1,.layout_only=passive==2});
    cui_theme t=cui_theme_light(); cui_numeric_selection_event_result r={0};
    if(legacy) r.control=styled ? cui_stepper(f->ui,2,&t,items,count,&f->value,vertical,disabled)
        : cui_base_stepper(f->ui,2,items,count,&f->value,disabled);
    else {
        cui_stepper_event_handler h={handler?step_changed:NULL,f};
        r=styled ? cui_stepper_events(f->ui,2,&t,items,count,&f->value,vertical,disabled,h)
            : cui_base_stepper_events(f->ui,2,items,count,&f->value,disabled,h);
    }
    cui_end_box(f->ui);f->draw=cui_end_frame(f->ui);
    assert(cui_last_error(f->ui)==CUI_OK);return r;
}
static void stepper(bool styled) {
    fixture f=make();
    step_frame(&f,styled,false,steps,3,false,0,true,false);
    key(&f,cui_numeric_part_id(2,2),CUI_KEY_ENTER);
    assert(step_frame(&f,styled,false,steps,3,false,0,true,false).change_count==1 && f.value==1);
    assert(f.step_event.before==0 && f.step_event.after==1 && f.step_event.count==3);
    activate(&f,1);activate(&f,2);
    assert(!step_frame(&f,styled,false,steps,3,false,0,true,false).control && f.value==1 && f.calls==1);
    assert(!cui_queue_event(f.ui,(cui_event){.type=CUI_EVENT_ACCESSIBILITY_ACTION,
        .accessibility_target=cui_numeric_part_id(2,3),.accessibility_action=CUI_ACCESSIBILITY_ACTIVATE}));
    assert(cui_last_error(f.ui)==CUI_ERROR_INVALID_ACTION); cui_clear_error(f.ui);
    assert(!step_frame(&f,styled,false,steps,3,false,0,true,false).change_count && f.value==1);
    activate(&f,1);
    assert(!step_frame(&f,styled,false,steps,3,false,0,false,false).change_count && f.value==0 && f.calls==1);
    f.value=SIZE_MAX;
    assert(step_frame(&f,styled,false,steps,3,false,0,true,false).change_count==1 && f.value==2);
    assert(f.step_event.before==SIZE_MAX && f.step_event.after==2);
    assert(step_frame(&f,styled,false,NULL,0,false,0,true,true).change_count==1 && f.value==0);
    f.value=9;
    assert(!step_frame(&f,styled,false,NULL,3,false,0,true,false).control && f.value==9);
    assert(!step_frame(&f,styled,false,steps,101,false,0,true,false).control && f.value==9);
    assert(steps[0].status==CUI_STEP_ACTIVE && steps[1].status==CUI_STEP_ERROR && steps[2].status==CUI_STEP_COMPLETE);
    assert(f.teardown_requested);cui_destroy(f.ui);
    if(styled) {
        f=make();step_frame(&f,true,false,steps,3,false,0,true,false);
        click(&f,2,.5f);
        assert(step_frame(&f,true,false,steps,3,false,0,true,false).change_count==1 && f.value==1);
        cui_destroy(f.ui);
    }
    for(unsigned passive=0;passive<3;++passive) for(unsigned disabled=0;disabled<2;++disabled)
        for(unsigned vertical=0;vertical<2;++vertical) {
            fixture a=make(),b=make();a.value=b.value=SIZE_MAX;
            cui_numeric_selection_event_result ar=step_frame(&a,styled,true,steps,3,disabled!=0,passive,false,vertical!=0);
            cui_numeric_selection_event_result br=step_frame(&b,styled,false,steps,3,disabled!=0,passive,true,vertical!=0);
            compare(&a,&b);assert(a.value==2 && ar.control==br.control);
            assert(br.change_count==((!passive&&!disabled)?1u:0u));
            cui_destroy(a.ui);cui_destroy(b.ui);
        }
}
int main(void) { pagination(false); pagination(true); stepper(false); stepper(true); return 0; }
