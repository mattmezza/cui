/* SPDX-License-Identifier: MIT */
#include "cui/cui_otp_events.h"
#include "cui/cui_components.h"
#include <assert.h>
#include <math.h>
#include <string.h>
typedef struct {
    cui_context *ui;
    cui_otp_state state;
    char buffer[2048], saved_text[2048];
    cui_otp_change_event saved;
    size_t calls, consumed;
    bool remove_requested;
    cui_draw_list draw;
} fixture;
static void init(fixture *f,size_t target,size_t capacity) {
    memset(f,0,sizeof *f);
    f->ui=cui_create((cui_config){.width=800,.height=300});assert(f->ui);
    assert(cui_otp_init(&f->state,f->buffer,capacity,target)==CUI_EDIT_OK);
}
static void changed(void *user,const cui_otp_change_event *e) {
    fixture *f=user;
    assert(e->event.kind==CUI_COMPONENT_EVENT_CHANGE && e->event.target==10);
    assert(e->event.flags==CUI_COMPONENT_EVENT_HAS_FRAME && e->event.source==CUI_COMPONENT_EVENT_SOURCE_UNKNOWN);
    assert(e->event.frame_sequence==cui_frame_sequence(f->ui));
    assert(e->after_bytes==cui_otp_value(&f->state).length && e->after_count==cui_otp_count(&f->state));
    assert(e->target_length==f->state.length && e->final_complete==(e->after_count==e->target_length));
    size_t count;const cui_semantic *items=cui_semantics(f->ui,&count);bool found=false;
    for(size_t i=0;i<count;++i) if(items[i].id==10) {assert(items[i].value==(float)e->after_count);found=true;}
    assert(found);
    f->saved=*e;f->calls++;f->remove_requested=true;
    if(e->has_text) {
        cui_str copy={0};
        assert(cui_component_event_copy_utf8(e->text,sizeof(f->saved_text)-1,f->saved_text,sizeof(f->saved_text),&copy)==CUI_COMPONENT_EVENT_COPY_OK);
        f->saved.text=copy;
        char unchanged[4]="abc";cui_str untouched={unchanged,3};
        assert(cui_component_event_copy_utf8(e->text,0,unchanged,sizeof unchanged,&untouched)==
            (e->text.length?CUI_COMPONENT_EVENT_COPY_LIMIT:CUI_COMPONENT_EVENT_COPY_OK));
        if(e->text.length) assert(!strcmp(unchanged,"abc") && untouched.length==3);
        if(e->text.length) {
            assert(cui_component_event_copy_utf8(e->text,e->text.length,unchanged,1,&untouched)==CUI_COMPONENT_EVENT_COPY_CAPACITY);
            assert(!strcmp(unchanged,"abc"));
        }
    } else assert(!e->text.data && !e->text.length);
}
static cui_otp_event_result frame(fixture *f,bool styled,bool legacy,cui_otp_options o,unsigned passive,bool handler) {
    assert(cui_begin_frame(f->ui,800,300,.016f));
    cui_begin_box(f->ui,900,(cui_box){.width=700,.height=250,.inert=passive==1,.layout_only=passive==2});
    cui_theme t=cui_theme_light();cui_otp_event_result r={0};
    if(legacy) r.control=styled?cui_otp(f->ui,10,&t,&f->state,o):cui_base_otp(f->ui,10,&f->state,o);
    else {cui_otp_event_handler h={handler?changed:NULL,f};
        r=styled?cui_otp_events(f->ui,10,&t,&f->state,o,h):cui_base_otp_events(f->ui,10,&f->state,o,h);}
    f->consumed=0;size_t event_count;cui_frame_events(f->ui,&event_count);
    for(size_t i=0;i<event_count;++i) f->consumed+=cui_event_consumed(f->ui,i)?1u:0u;
    cui_end_box(f->ui);f->draw=cui_end_frame(f->ui);assert(cui_last_error(f->ui)==CUI_OK);return r;
}
static void text(fixture *f,const char *text) {
    assert(cui_queue_event(f->ui,(cui_event){.type=CUI_EVENT_TEXT,.text_data=text,.text_length=strlen(text)}));
}
static void backspace(fixture *f) {
    assert(cui_queue_event(f->ui,(cui_event){.type=CUI_EVENT_KEY_DOWN,.key=CUI_KEY_BACKSPACE}));
}
static void focus(fixture *f,bool styled,cui_otp_options o) {frame(f,styled,false,o,0,true);cui_focus(f->ui,10);}
static void compare(fixture *a,fixture *b,cui_otp_result x,cui_otp_result y) {
    assert(!strcmp(a->buffer,b->buffer) && a->state.focused==b->state.focused &&
        a->state.cursor_visible==b->state.cursor_visible && a->state.blink_epoch==b->state.blink_epoch);
    assert(x.changed==y.changed && x.complete==y.complete && x.focused==y.focused && x.blurred==y.blurred &&
        x.change_count==y.change_count && x.complete_count==y.complete_count && x.error==y.error && x.next_wake_seconds==y.next_wake_seconds);
    assert(cui_focused(a->ui)==cui_focused(b->ui) && a->consumed==b->consumed);
    cui_rect ar={0},br={0};bool ah=cui_text_input_rect(a->ui,&ar),bh=cui_text_input_rect(b->ui,&br);
    assert(ah==bh && ar.x==br.x && ar.y==br.y && ar.w==br.w && ar.h==br.h);
    size_t an,bn;const cui_semantic *as=cui_semantics(a->ui,&an),*bs=cui_semantics(b->ui,&bn);
    assert(an==bn && a->draw.count==b->draw.count);
    for(size_t i=0;i<an;++i) {
        assert(as[i].id==bs[i].id && as[i].role==bs[i].role && as[i].value==bs[i].value &&
            as[i].disabled==bs[i].disabled && as[i].details.maximum==bs[i].details.maximum);
        ar=cui_bounds(a->ui,as[i].id);br=cui_bounds(b->ui,bs[i].id);
        assert(ar.x==br.x && ar.y==br.y && ar.w==br.w && ar.h==br.h);
    }
    for(size_t i=0;i<a->draw.count;++i) {
        const cui_draw_command *ac=&a->draw.commands[i],*bc=&b->draw.commands[i];
        assert(ac->type==bc->type && ac->bounds.x==bc->bounds.x && ac->bounds.y==bc->bounds.y &&
            ac->bounds.w==bc->bounds.w && ac->bounds.h==bc->bounds.h && ac->font_size==bc->font_size &&
            ac->color.r==bc->color.r && ac->color.g==bc->color.g && ac->color.b==bc->color.b && ac->color.a==bc->color.a &&
            ac->text.length==bc->text.length);
        if(ac->text.length) assert(!memcmp(ac->text.data,bc->text.data,ac->text.length));
    }
    if(a->state.composition || b->state.composition) {
        assert(a->state.composition && b->state.composition);
        assert(a->state.composition->active==b->state.composition->active &&
            a->state.composition->stale==b->state.composition->stale &&
            a->state.composition->length==b->state.composition->length &&
            a->state.composition->anchor==b->state.composition->anchor &&
            a->state.composition->cursor==b->state.composition->cursor);
        if(a->state.composition->length) assert(!memcmp(a->state.composition->buffer,b->state.composition->buffer,a->state.composition->length));
    }
}
static void operations(bool styled) {
    fixture f;init(&f,2,sizeof f.buffer);cui_otp_options o=cui_otp_options_default();focus(&f,styled,o);
    backspace(&f);
    cui_otp_event_result r=frame(&f,styled,false,o,0,true);
    assert(r.change_count==1 && f.saved.accepted_count==1 && !f.saved.after_bytes && !f.saved.final_complete);
    text(&f,"1");text(&f,"２");backspace(&f);text(&f,"2");backspace(&f);
    r=frame(&f,styled,false,o,0,true);
    assert(r.change_count==1 && f.saved.accepted_count==5 && f.saved.completion_count==2 &&
        !f.saved.final_complete && !strcmp(f.saved_text,"1"));
    text(&f,"2");backspace(&f);
    r=frame(&f,styled,false,o,0,true);
    assert(r.change_count==1 && f.saved.before_bytes==1 && f.saved.after_bytes==1 && f.saved.accepted_count==2);
    assert(!strcmp(f.saved_text,"1"));
    assert(cui_otp_set_value(&f.state,cui_string("AéZ"))==CUI_EDIT_OK);
    backspace(&f);r=frame(&f,styled,false,o,0,true);
    assert(r.change_count==1 && f.saved.before_count==3 && f.saved.after_count==2 &&
        f.saved.before_bytes==4 && f.saved.after_bytes==3 && f.saved.completion_count==1 && !strcmp(f.saved_text,"Aé"));
    assert(cui_otp_set_value(&f.state,cui_string("00"))==CUI_EDIT_OK);
    assert(!frame(&f,styled,false,o,0,true).change_count && !strcmp(f.saved_text,"Aé")); /* Explicit copy survived owner update. */
    size_t calls=f.calls;backspace(&f);
    assert(!frame(&f,styled,false,o,0,false).change_count && f.calls==calls && !strcmp(f.buffer,"0"));
    f.state.masked=true;text(&f,"8");
    assert(frame(&f,styled,false,o,0,true).change_count==1 && !f.saved.has_text && !f.saved.text.length);
    assert(f.remove_requested);cui_destroy(f.ui); /* Teardown after outer calls/draw consumption. */
    init(&f,0,sizeof f.buffer);focus(&f,styled,o);backspace(&f);
    assert(frame(&f,styled,false,o,0,true).change_count==1 && f.saved.completion_count==1 && f.saved.final_complete);
    cui_destroy(f.ui);
    init(&f,6,5);focus(&f,styled,o);
    text(&f,"1");text(&f,"2");text(&f,"3");text(&f,"4");text(&f,"5");
    r=frame(&f,styled,false,o,0,true);
    assert(r.control.error==CUI_EDIT_CAPACITY && r.change_count==1 && f.saved.accepted_count==4 && !strcmp(f.saved_text,"1234"));
    cui_destroy(f.ui);
}
static void errors_and_limits(bool styled) {
    fixture f;init(&f,4,sizeof f.buffer);cui_otp_options o=cui_otp_options_default();focus(&f,styled,o);
    /* Earlier accepted input remains observable even when a later session error is INVALID_ARGUMENT. */
    cui_otp_composition overlap={.buffer=f.buffer,.capacity=sizeof f.buffer};
    f.state.composition=&overlap;
    text(&f,"1");
    assert(cui_queue_event(f.ui,(cui_event){.type=CUI_EVENT_COMPOSITION_UPDATE,.text_data="2",.text_length=1}));
    cui_otp_event_result r=frame(&f,styled,false,o,0,true);
    assert(r.control.error==CUI_EDIT_INVALID_ARGUMENT && r.change_count==1 &&
        f.saved.accepted_count==1 && !strcmp(f.saved_text,"1"));
    f.state.composition=NULL;
    size_t calls=f.calls;
    assert(cui_queue_event(f.ui,(cui_event){.type=CUI_EVENT_KEY_DOWN,.key=CUI_KEY_ENTER}));
    assert(cui_queue_event(f.ui,(cui_event){.type=CUI_EVENT_TEXT,.text="3",.modifiers=CUI_MOD_CTRL}));
    assert(cui_queue_event(f.ui,(cui_event){.type=CUI_EVENT_KEY_DOWN,.key=CUI_KEY_BACKSPACE,.ime_composing=true}));
    assert(!frame(&f,styled,false,o,0,true).change_count && f.calls==calls && !strcmp(f.buffer,"1"));
    text(&f,"23");
    assert(frame(&f,styled,false,o,0,true).change_count==1 && !strcmp(f.buffer,"12")); /* First digit only. */
    cui_destroy(f.ui);init(&f,4,sizeof f.buffer);focus(&f,styled,o);
    memset(f.buffer,'a',1026);f.buffer[1026]=0;
    backspace(&f);r=frame(&f,styled,false,o,0,true);
    assert(r.change_count==1 && f.saved.before_bytes==1026 && f.saved.after_bytes==1025 &&
        f.saved.before_count==1026 && f.saved.after_count==1025 && f.saved.text.length==1025);
    cui_destroy(f.ui);
}
static void composition(bool styled) {
    fixture a,b;init(&a,2,sizeof a.buffer);init(&b,2,sizeof b.buffer);
    cui_otp_options o=cui_otp_options_default();a.state.masked=b.state.masked=true;
    focus(&a,styled,o);focus(&b,styled,o);
    cui_event update={.type=CUI_EVENT_COMPOSITION_UPDATE,.text_data="１２",.text_length=6,.composition_anchor=3,.composition_cursor=6};
    assert(cui_queue_event(a.ui,update)&&cui_queue_event(b.ui,update));
    cui_otp_event_result ar=frame(&a,styled,true,o,0,false),br=frame(&b,styled,false,o,0,true);
    compare(&a,&b,ar.control,br.control);assert(!br.change_count && b.state.composition->active);
    cui_event commit={.type=CUI_EVENT_COMPOSITION_COMMIT,.text_data="１２",.text_length=6};
    assert(cui_queue_event(a.ui,commit)&&cui_queue_event(b.ui,commit));
    assert(cui_queue_event(a.ui,commit)&&cui_queue_event(b.ui,commit));
    ar=frame(&a,styled,true,o,0,false);br=frame(&b,styled,false,o,0,true);
    compare(&a,&b,ar.control,br.control);
    assert(b.consumed==2);
    assert(br.change_count==1 && b.saved.accepted_count==1 && !b.saved.has_text && !strcmp(b.buffer,"1"));
    assert(cui_queue_event(a.ui,update)&&cui_queue_event(b.ui,update));
    frame(&a,styled,true,o,0,false);frame(&b,styled,false,o,0,true);
    assert(cui_queue_event(a.ui,(cui_event){.type=CUI_EVENT_COMPOSITION_CANCEL}));
    assert(cui_queue_event(b.ui,(cui_event){.type=CUI_EVENT_COMPOSITION_CANCEL}));
    ar=frame(&a,styled,true,o,0,false);br=frame(&b,styled,false,o,0,true);
    compare(&a,&b,ar.control,br.control);assert(!br.change_count);
    cui_destroy(a.ui);cui_destroy(b.ui);
}
static void paired(bool styled) {
    for(unsigned passive=0;passive<3;++passive) for(unsigned disabled=0;disabled<2;++disabled) {
        fixture a,b;init(&a,4,sizeof a.buffer);init(&b,4,sizeof b.buffer);
        cui_otp_options o=cui_otp_options_default();focus(&a,styled,o);focus(&b,styled,o);
        text(&a,"1");text(&a,"２");text(&b,"1");text(&b,"２");o.disabled=disabled!=0;
        cui_otp_event_result ar=frame(&a,styled,true,o,passive,false),br=frame(&b,styled,false,o,passive,true);
        compare(&a,&b,ar.control,br.control);
        assert(br.change_count==((!passive&&!disabled)?1u:0u));
        cui_destroy(a.ui);cui_destroy(b.ui);
    }
}
static void invalid(bool styled) {
    fixture f;init(&f,4,sizeof f.buffer);cui_otp_options o=cui_otp_options_default();
    memset(f.buffer,'x',sizeof f.buffer); /* No terminating NUL anywhere in declared capacity. */
    assert(frame(&f,styled,false,o,0,true).control.error==CUI_EDIT_INVALID_ARGUMENT && !f.calls);
    f.buffer[0]=(char)0xff;f.buffer[1]=0;
    assert(frame(&f,styled,false,o,0,true).control.error==CUI_EDIT_INVALID_ARGUMENT && !f.calls);
    f.buffer[0]=0;o.cell_size=NAN;
    assert(frame(&f,styled,false,o,0,true).control.error==CUI_EDIT_INVALID_ARGUMENT && !f.calls);
    cui_destroy(f.ui);
}
int main(void) {
    for(unsigned styled=0;styled<2;++styled) {operations(styled!=0);errors_and_limits(styled!=0);composition(styled!=0);paired(styled!=0);invalid(styled!=0);}
    return 0;
}
