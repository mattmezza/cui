/* SPDX-License-Identifier: MIT */
#include "cui/cui_table_events.h"
#include <assert.h>
#include <string.h>
#define COL_A UINT64_C(9007199254740993)
#define COL_B UINT64_C(18446744073709551001)
typedef struct {
    cui_context *ui;
    cui_table_model *model;
    cui_table_state state;
    cui_table_row rows[2];
    cui_table_column columns[2];
    cui_table_sort_change sorts[32];
    cui_table_selection_change selections[32];
    cui_table_column_width_change widths[32];
    cui_table_column_move_change moves[32];
    size_t width_calls, move_calls;
    cui_table_activation activations[32];
    cui_table_context_request contexts[32];
    size_t activation_calls, context_calls;
    bool observe_actions, consume_selection;
    size_t selection_calls, trace_length;
    char trace[128];
    bool observe_selection;
    size_t sort_calls, renders;
    bool render_started, remove_requested, consume_first;
    cui_draw_list draw;
} fixture;
static void init(fixture *f) {
    /* Virtualized fixtures submit a small viewport even for the 4096-row model.
     * Bound context storage to that viewport instead of repeatedly zeroing the
     * application-sized defaults across the legacy/facade comparison matrix. */
    memset(f,0,sizeof *f);f->ui=cui_create((cui_config){.width=600,.height=300,
        .max_elements=256,.max_commands=1024});assert(f->ui);
    f->model=cui_table_create(2);assert(f->model);
    f->rows[0]=(cui_table_row){1,{"One",3},false};f->rows[1]=(cui_table_row){2,{"Two",3},false};
    f->columns[0]=(cui_table_column){.key=COL_A,.label={"Alpha",5},.width=150,.sortable=true};
    f->columns[1]=(cui_table_column){.key=COL_B,.label={"Beta",4},.width=150,.sortable=true};
    assert(cui_table_bind(f->model,f->rows,2,f->columns,2)==CUI_TABLE_OK);
    f->state.sort_column=COL_A;
}
static void finish(fixture *f){cui_table_destroy(f->model);cui_destroy(f->ui);}
static void sort_changed(void *user,const cui_table_sort_change *event) {
    fixture *f=user;assert(!f->render_started&&f->sort_calls<32);
    assert(event->event.target==90&&event->event.kind==CUI_COMPONENT_EVENT_CHANGE);
    assert(event->event.flags&CUI_COMPONENT_EVENT_HAS_INDEX);
    assert(event->event.frame_sequence==cui_frame_sequence(f->ui));
    assert(event->column==f->state.sort_column&&event->value==f->state.sort);
    assert(event->previous_column!=event->column||event->previous!=event->value);
    f->sorts[f->sort_calls++]=*event;f->remove_requested=true;
    assert(f->trace_length<sizeof f->trace);f->trace[f->trace_length++]='O';
}
static void selection_changed(void *user,const cui_table_selection_change *event) {
    fixture *f=user;assert(!f->render_started&&f->selection_calls<32);
    assert(event->event.target==90&&event->event.kind==CUI_COMPONENT_EVENT_CHANGE);
    assert(event->event.flags&CUI_COMPONENT_EVENT_HAS_INDEX);
    assert(event->event.frame_sequence==cui_frame_sequence(f->ui));
    assert(event->value.row==f->state.row&&event->value.column==f->state.column);
    assert(event->previous.row!=event->value.row||event->previous.column!=event->value.column);
    f->selections[f->selection_calls++]=*event;f->remove_requested=true;
    assert(f->trace_length<sizeof f->trace);f->trace[f->trace_length++]='S';
    if(f->consume_selection)assert(cui_consume_event(f->ui,event->event.event_index));
}
static void width_changed(void *user,const cui_table_column_width_change *change) {
    fixture *f=user;assert(!f->render_started&&f->width_calls<32);
    assert(change->event.target==90&&change->event.kind==CUI_COMPONENT_EVENT_CHANGE&&(change->event.flags&CUI_COMPONENT_EVENT_HAS_INDEX));
    assert(change->event.frame_sequence==cui_frame_sequence(f->ui)&&change->previous_width!=change->width);
    cui_table_column col;bool found=false;for(size_t i=0;i<2;i++){assert(cui_table_get_column(f->model,i,&col));if(col.key==change->column){assert(col.width==change->width);found=true;}}
    assert(found);if(change->event.source==CUI_COMPONENT_EVENT_SOURCE_POINTER)assert(f->state.resizing==change->column);
    f->widths[f->width_calls++]=*change;f->remove_requested=true;f->trace[f->trace_length++]='W';
}
static void column_moved(void *user,const cui_table_column_move_change *change) {
    fixture *f=user;assert(!f->render_started&&f->move_calls<32);
    assert(change->event.target==90&&change->event.kind==CUI_COMPONENT_EVENT_CHANGE&&(change->event.flags&CUI_COMPONENT_EVENT_HAS_INDEX));
    assert(change->event.frame_sequence==cui_frame_sequence(f->ui)&&change->previous_index!=change->index);
    cui_table_column col;assert(cui_table_get_column(f->model,change->index,&col)&&col.key==change->column);
    if(change->event.source==CUI_COMPONENT_EVENT_SOURCE_POINTER)assert(f->state.dragging==change->column&&f->state.drag_active);
    f->moves[f->move_calls++]=*change;f->remove_requested=true;f->trace[f->trace_length++]='P';
}
static void activated(void *user,const cui_table_activation *event) {
    fixture *f=user;assert(!f->render_started&&f->activation_calls<32);
    assert(event->event.target==90&&event->event.kind==CUI_COMPONENT_EVENT_ACTIVATE&&(event->event.flags&CUI_COMPONENT_EVENT_HAS_INDEX));
    assert(event->event.frame_sequence==cui_frame_sequence(f->ui));
    assert(event->area==CUI_TABLE_ACTION_CELL||event->area==CUI_TABLE_ACTION_ROW_HEADER);
    if(event->area==CUI_TABLE_ACTION_ROW_HEADER)assert(!event->column);
    f->activations[f->activation_calls++]=*event;f->remove_requested=true;assert(f->trace_length<sizeof f->trace);f->trace[f->trace_length++]='A';
}
static void context_requested(void *user,const cui_table_context_request *event) {
    fixture *f=user;assert(!f->render_started&&f->context_calls<32);
    assert(event->event.target==90&&event->event.kind==CUI_COMPONENT_EVENT_REQUEST&&(event->event.flags&CUI_COMPONENT_EVENT_HAS_INDEX));
    assert(event->event.frame_sequence==cui_frame_sequence(f->ui)&&event->event.source==CUI_COMPONENT_EVENT_SOURCE_POINTER);
    assert(!(event->event.flags&CUI_COMPONENT_EVENT_HAS_ACTIVATION));
    f->contexts[f->context_calls++]=*event;f->remove_requested=true;assert(f->trace_length<sizeof f->trace);f->trace[f->trace_length++]='Q';
}
static cui_str label(void *user,size_t row,cui_id column){
    fixture *f=user;assert(row<2&&(column==COL_A||column==COL_B));f->render_started=true;f->renders++;
    return f->rows[row].label;
}
static cui_table_options options(void){return (cui_table_options){.width=300,.height=120,.row_height=32,.header_height=32,.selection_mode=CUI_TABLE_SELECT_CELL};}
static cui_table_event_result frame(fixture *f,unsigned mode,bool legacy,bool handler,unsigned passive,cui_table_options o) {
    assert(cui_begin_frame(f->ui,600,300,.016f));f->render_started=false;
    cui_begin_box(f->ui,900,(cui_box){.width=500,.height=250,.inert=passive==1,.layout_only=passive==2});
    if(f->consume_first)cui_consume_event(f->ui,0);
    cui_theme t=cui_theme_light();cui_table_event_result r={0};
    cui_table_event_handler h={.on_selection=handler&&f->observe_selection?selection_changed:NULL,
        .on_sort=handler?sort_changed:NULL,
        .on_column_width=handler?width_changed:NULL,.on_column_move=handler?column_moved:NULL,
        .on_activate=handler&&f->observe_actions?activated:NULL,
        .on_context_request=handler&&f->observe_actions?context_requested:NULL,.user=f};
    if(mode==2) {
        if(legacy)r.control=cui_data_table(f->ui,90,&t,f->model,&f->state,o,label,NULL,f);
        else r=cui_data_table_events(f->ui,90,&t,f->model,&f->state,o,label,NULL,f,h);
    } else if(mode==1) {
        if(legacy)r.control=cui_base_data_table(f->ui,90,f->model,&f->state,o);
        else r=cui_base_data_table_events(f->ui,90,f->model,&f->state,o,h);
    } else {
        if(legacy)r.control=cui_table_reduce(f->ui,90,f->model,&f->state,o);
        else r=cui_table_reduce_events(f->ui,90,f->model,&f->state,o,h);
        if(r.control.error==CUI_TABLE_OK||r.control.error==CUI_TABLE_CAPACITY) {
            cui_table_submit_root(f->ui,90,f->model,&f->state,o);
            cui_table_submit_header(f->ui,90,f->model,&f->state,o);
            cui_table_submit_body(f->ui,90,f->model,&f->state,o,NULL,NULL);
        }
    }
    cui_end_box(f->ui);f->draw=cui_end_frame(f->ui);assert(cui_last_error(f->ui)==CUI_OK);return r;
}
static void key(fixture *f,cui_key code){assert(cui_queue_event(f->ui,(cui_event){.type=CUI_EVENT_KEY_DOWN,.key=code}));}
static void click(fixture *f,cui_id id){cui_rect b=cui_bounds(f->ui,id);assert(b.w>0&&b.h>0);
    assert(cui_queue_event(f->ui,(cui_event){.type=CUI_EVENT_POINTER_DOWN,.x=b.x+2,.y=b.y+2}));
    assert(cui_queue_event(f->ui,(cui_event){.type=CUI_EVENT_POINTER_UP,.x=b.x+2,.y=b.y+2}));}
static void compare(fixture *a,fixture *b,cui_table_result x,cui_table_result y){
    assert(x.error==y.error&&x.selection_changed==y.selection_changed&&x.sort_changed==y.sort_changed&&
        x.columns_changed==y.columns_changed&&x.visible_rows_changed==y.visible_rows_changed&&x.visible_columns_changed==y.visible_columns_changed&&
        x.load_more==y.load_more&&x.context_requested==y.context_requested&&x.context_row==y.context_row&&x.context_column==y.context_column&&
        x.selection_mode_requested==y.selection_mode_requested&&x.requested_selection_mode==y.requested_selection_mode&&
        x.activated_row==y.activated_row&&x.activated_column==y.activated_column);
    assert(a->state.sort==b->state.sort&&a->state.sort_column==b->state.sort_column&&a->state.row==b->state.row&&a->state.column==b->state.column&&
        a->state.cursor_row==b->state.cursor_row&&a->state.cursor_column==b->state.cursor_column&&a->state.scroll_x==b->state.scroll_x&&a->state.scroll_y==b->state.scroll_y&&
        a->state.visible_begin==b->state.visible_begin&&a->state.visible_end==b->state.visible_end&&cui_focused(a->ui)==cui_focused(b->ui));
    size_t an,bn;const cui_semantic *as=cui_semantics(a->ui,&an),*bs=cui_semantics(b->ui,&bn);assert(an==bn&&a->draw.count==b->draw.count);
    for(size_t i=0;i<an;++i){assert(as[i].id==bs[i].id&&as[i].role==bs[i].role&&as[i].value==bs[i].value&&as[i].disabled==bs[i].disabled&&as[i].details.selected==bs[i].details.selected);
        cui_rect ar=cui_bounds(a->ui,as[i].id),br=cui_bounds(b->ui,bs[i].id);assert(ar.x==br.x&&ar.y==br.y&&ar.w==br.w&&ar.h==br.h);}
    for(size_t i=0;i<a->draw.count;++i){const cui_draw_command *ac=&a->draw.commands[i],*bc=&b->draw.commands[i];
        assert(ac->type==bc->type&&ac->bounds.x==bc->bounds.x&&ac->bounds.y==bc->bounds.y&&ac->bounds.w==bc->bounds.w&&ac->bounds.h==bc->bounds.h&&
            ac->text.length==bc->text.length&&ac->color.r==bc->color.r&&ac->color.g==bc->color.g&&ac->color.b==bc->color.b&&ac->color.a==bc->color.a);
        if(ac->text.length)assert(!memcmp(ac->text.data,bc->text.data,ac->text.length));}
}
static void sorts(unsigned mode){
    fixture f;init(&f);cui_table_options o=options();frame(&f,mode,false,true,0,o);cui_focus(f.ui,cui_table_header_id(90,COL_A));
    key(&f,CUI_KEY_ENTER);key(&f,CUI_KEY_ENTER);key(&f,CUI_KEY_ENTER);
    cui_table_event_result r=frame(&f,mode,false,true,0,o);
    assert(r.valid&&r.submitted&&r.control.sort_changed&&r.sort_count==3&&f.state.sort==CUI_TABLE_SORT_DEFAULT);
    assert(f.sorts[0].previous_column==COL_A&&f.sorts[0].column==COL_A&&f.sorts[0].previous==CUI_TABLE_SORT_DEFAULT&&f.sorts[0].value==CUI_TABLE_SORT_ASCENDING);
    assert(f.sorts[1].value==CUI_TABLE_SORT_DESCENDING&&f.sorts[2].value==CUI_TABLE_SORT_DEFAULT);
    cui_focus(f.ui,cui_table_header_id(90,COL_B));key(&f,CUI_KEY_ENTER);r=frame(&f,mode,false,true,0,o);
    assert(r.sort_count==1&&f.sorts[3].previous_column==COL_A&&f.sorts[3].column==COL_B);
    assert(f.sorts[3].event.source==CUI_COMPONENT_EVENT_SOURCE_KEYBOARD);
    key(&f,CUI_KEY_ENTER);r=frame(&f,mode,false,false,0,o);assert(r.control.sort_changed&&!r.sort_count&&f.sort_calls==4);
    assert(f.remove_requested);finish(&f);
    init(&f);o.sort_cycle=CUI_TABLE_SORT_CYCLE_SOURCE;frame(&f,mode,false,true,0,o);cui_focus(f.ui,cui_table_header_id(90,COL_A));key(&f,CUI_KEY_ENTER);
    assert(frame(&f,mode,false,true,0,o).sort_count==1&&f.sorts[0].value==CUI_TABLE_SORT_DESCENDING);finish(&f);
    if(mode==2){init(&f);o=options();frame(&f,mode,false,true,0,o);click(&f,cui_table_header_id(90,COL_A));
        assert(frame(&f,mode,false,true,0,o).sort_count==1&&f.sorts[0].event.source==CUI_COMPONENT_EVENT_SOURCE_POINTER);finish(&f);}
    for(unsigned passive=0;passive<3;++passive)for(unsigned restricted=0;restricted<3;++restricted){
        fixture a,b;init(&a);init(&b);o=options();frame(&a,mode,true,false,0,o);frame(&b,mode,false,true,0,o);
        cui_focus(a.ui,cui_table_header_id(90,COL_A));cui_focus(b.ui,cui_table_header_id(90,COL_A));key(&a,CUI_KEY_ENTER);key(&b,CUI_KEY_ENTER);
        o.disabled=restricted==1;o.loading=restricted==2;
        cui_table_event_result ar=frame(&a,mode,true,false,passive,o),br=frame(&b,mode,false,true,passive,o);compare(&a,&b,ar.control,br.control);
        assert(br.sort_count==((!passive&&!restricted)?1u:0u));finish(&a);finish(&b);}
}
static void progress_and_consumption(void){
    fixture f;init(&f);cui_table_options o=options();o.multiple_selection=true;
    frame(&f,2,false,true,0,o);click(&f,cui_table_header_id(90,COL_A));click(&f,cui_table_cell_id(90,2,COL_A));
    cui_table_event_result r=frame(&f,2,false,true,0,o);
    assert(r.valid&&r.control.error==CUI_TABLE_CAPACITY&&r.sort_count==1&&f.state.sort==CUI_TABLE_SORT_ASCENDING);
    finish(&f);
    fixture a,b;init(&a);init(&b);o=options();frame(&a,2,true,false,0,o);frame(&b,2,false,true,0,o);
    cui_focus(a.ui,cui_table_header_id(90,COL_A));cui_focus(b.ui,cui_table_header_id(90,COL_A));key(&a,CUI_KEY_ENTER);key(&b,CUI_KEY_ENTER);
    a.consume_first=b.consume_first=true;
    cui_table_event_result ar=frame(&a,2,true,false,0,o),br=frame(&b,2,false,true,0,o);
    compare(&a,&b,ar.control,br.control);assert(br.sort_count==(ar.control.sort_changed?1u:0u));finish(&a);finish(&b);
}
static void selections(unsigned mode){
    fixture f;init(&f);f.observe_selection=true;f.state.row=1;f.state.column=COL_A;
    cui_table_options o=options();frame(&f,mode,false,true,0,o);cui_focus(f.ui,cui_table_cell_id(90,1,COL_A));
    key(&f,CUI_KEY_DOWN);key(&f,CUI_KEY_UP);
    cui_table_event_result r=frame(&f,mode,false,true,0,o);
    assert(r.valid&&r.submitted&&!r.control.selection_changed&&r.selection_count==2&&f.state.row==1);
    assert(f.selections[0].previous.row==1&&f.selections[0].value.row==2&&f.selections[1].previous.row==2&&f.selections[1].value.row==1);
    assert(f.selections[0].value.column==COL_A&&f.selections[0].mode==CUI_TABLE_SELECT_CELL);
    key(&f,CUI_KEY_ENTER);r=frame(&f,mode,false,true,0,o);assert(!r.selection_count&&r.control.activated_row==1);
    o.manual_selection=true;key(&f,CUI_KEY_DOWN);r=frame(&f,mode,false,true,0,o);assert(!r.selection_count&&f.state.row==1&&f.state.cursor_row==2);
    key(&f,CUI_KEY_ENTER);r=frame(&f,mode,false,true,0,o);assert(r.selection_count==1&&f.state.row==2);
    key(&f,CUI_KEY_ESCAPE);r=frame(&f,mode,false,true,0,o);assert(r.selection_count==1&&!f.state.row&&!f.state.column);
    key(&f,CUI_KEY_ESCAPE);assert(!frame(&f,mode,false,true,0,o).selection_count);
    assert(f.remove_requested);finish(&f);
    init(&f);f.observe_selection=true;o=options();o.selection_mode=CUI_TABLE_SELECT_COLUMN;f.state.column=COL_A;
    frame(&f,mode,false,true,0,o);cui_focus(f.ui,cui_table_header_id(90,COL_B));key(&f,CUI_KEY_ENTER);
    r=frame(&f,mode,false,true,0,o);assert(r.selection_count==1&&r.sort_count==1&&!strcmp(f.trace,"SO"));
    assert(!f.selections[0].value.row&&f.selections[0].value.column==COL_B&&f.selections[0].mode==CUI_TABLE_SELECT_COLUMN);finish(&f);
    init(&f);f.observe_selection=true;o=options();o.row_header=true;f.state.row=1;f.state.column=COL_A;
    frame(&f,mode,false,true,0,o);cui_focus(f.ui,cui_table_row_header_id(90,2));key(&f,CUI_KEY_ENTER);
    r=frame(&f,mode,false,true,0,o);assert(r.selection_count==1&&r.control.selection_mode_requested&&r.control.requested_selection_mode==CUI_TABLE_SELECT_ROW);
    assert(f.selections[0].mode==CUI_TABLE_SELECT_ROW&&f.selections[0].value.row==2&&!f.selections[0].value.column);finish(&f);
    init(&f);f.observe_selection=true;o=options();f.state.row=1;f.state.column=COL_A;f.columns[1].nonselectable=true;
    frame(&f,mode,false,true,0,o);cui_focus(f.ui,cui_table_cell_id(90,1,COL_B));key(&f,CUI_KEY_ENTER);
    assert(!frame(&f,mode,false,true,0,o).selection_count&&f.state.column==COL_A);finish(&f);
    init(&f);f.observe_selection=true;o=options();o.multiple_selection=true;
    cui_table_state before=f.state;cui_table_column columns[2];memcpy(columns,f.columns,sizeof columns);
    r=frame(&f,mode,false,true,0,o);assert(!r.valid&&r.control.error==CUI_TABLE_INVALID&&!r.selection_count&&!r.sort_count);
    assert(!memcmp(&before,&f.state,sizeof before)&&!memcmp(columns,f.columns,sizeof columns));
    assert(frame(&f,mode,false,false,0,o).valid);finish(&f); /* Nil selection callback accepts multi; ignores user. */
    if(mode==2){init(&f);f.observe_selection=true;o=options();f.state.row=1;f.state.column=COL_A;frame(&f,mode,false,true,0,o);
        click(&f,cui_table_cell_id(90,2,COL_A));click(&f,cui_table_cell_id(90,1,COL_A));r=frame(&f,mode,false,true,0,o);
        assert(r.selection_count==2&&!r.control.selection_changed&&f.selections[0].event.source==CUI_COMPONENT_EVENT_SOURCE_POINTER);finish(&f);}
    init(&f);f.observe_selection=true;o=options();
    const cui_id row_a=UINT64_C(9007199254740995),row_b=UINT64_C(18446744073709551003);
    f.rows[0].key=row_a;f.rows[1].key=row_b;
    assert(cui_table_bind(f.model,f.rows,2,f.columns,2)==CUI_TABLE_OK);
    f.state.row=row_a;f.state.column=COL_A;frame(&f,mode,false,true,0,o);
    cui_focus(f.ui,cui_table_cell_id(90,row_a,COL_A));key(&f,CUI_KEY_DOWN);
    assert(frame(&f,mode,false,true,0,o).selection_count==1&&f.selections[0].previous.row==row_a&&f.selections[0].value.row==row_b);
    key(&f,CUI_KEY_UP);assert(!frame(&f,mode,false,false,0,o).selection_count&&f.state.row==row_a&&f.selection_calls==1);finish(&f);
    for(unsigned policy=0;policy<3;++policy){
        init(&f);f.observe_selection=true;o=options();o.selection_mode=(cui_table_selection_mode)policy;
        f.state.row=policy==CUI_TABLE_SELECT_COLUMN?0:1;f.state.column=policy==CUI_TABLE_SELECT_ROW?0:COL_A;
        o.row_selection_disabled=policy==CUI_TABLE_SELECT_ROW;
        o.column_selection_disabled=policy==CUI_TABLE_SELECT_COLUMN;
        o.cell_selection_disabled=policy==CUI_TABLE_SELECT_CELL;
        frame(&f,mode,false,true,0,o);
        cui_focus(f.ui,policy==CUI_TABLE_SELECT_COLUMN?cui_table_header_id(90,COL_B):cui_table_cell_id(90,1,COL_A));
        key(&f,policy==CUI_TABLE_SELECT_COLUMN?CUI_KEY_ENTER:CUI_KEY_DOWN);
        r=frame(&f,mode,false,true,0,o);assert(!r.selection_count&&!r.control.selection_changed);
        if(policy==CUI_TABLE_SELECT_COLUMN)assert(r.sort_count==1); /* Selection gating does not disable sorting. */
        finish(&f);
    }
    for(unsigned passive=0;passive<3;++passive)for(unsigned restricted=0;restricted<3;++restricted){
        fixture a,b;init(&a);init(&b);b.observe_selection=true;a.state.row=b.state.row=1;a.state.column=b.state.column=COL_A;o=options();
        frame(&a,mode,true,false,0,o);frame(&b,mode,false,true,0,o);cui_focus(a.ui,cui_table_cell_id(90,1,COL_A));cui_focus(b.ui,cui_table_cell_id(90,1,COL_A));
        key(&a,CUI_KEY_DOWN);key(&b,CUI_KEY_DOWN);o.disabled=restricted==1;o.loading=restricted==2;
        cui_table_event_result ar=frame(&a,mode,true,false,passive,o),br=frame(&b,mode,false,true,passive,o);compare(&a,&b,ar.control,br.control);
        assert(br.selection_count==((!passive&&!restricted)?1u:0u));finish(&a);finish(&b);}
}
/* Set snapshots deliberately copy entries: retaining the payload's spans is invalid. */
typedef struct {
    fixture f; /* First: existing label/sort test callbacks borrow this fixture. */
    cui_table_selection owner[8], before[8], after[8];
    cui_table_selection copied_before[32][8], copied_after[32][8];
    cui_table_selection_set_change changes[32];
    size_t calls;
    bool sort_only;
} set_fixture;
static bool selection_less(cui_table_selection a,cui_table_selection b) {
    return a.row<b.row||(a.row==b.row&&a.column<b.column);
}
static void set_changed(void *user,const cui_table_selection_set_change *change) {
    set_fixture *s=user;fixture *f=&s->f;assert(s->calls<32&&!f->render_started);
    assert(change->event.target==90&&change->event.kind==CUI_COMPONENT_EVENT_CHANGE);
    assert(change->event.flags&CUI_COMPONENT_EVENT_HAS_INDEX);
    assert(change->event.frame_sequence==cui_frame_sequence(f->ui));
    assert(change->value_primary.row==f->state.row&&change->value_primary.column==f->state.column);
    assert(change->value.count==f->state.selection_count);
    const cui_table_selection_span spans[2]={change->previous,change->value};
    for(size_t j=0;j<2;j++)for(size_t i=1;i<spans[j].count;i++)
        assert(!selection_less(spans[j].items[i],spans[j].items[i-1]));
    size_t n=s->calls++;s->changes[n]=*change;
    if(change->previous.count<=8) {
        if(change->previous.count)memcpy(s->copied_before[n],change->previous.items,change->previous.count*sizeof(cui_table_selection));
        s->changes[n].previous.items=s->copied_before[n];
    } else s->changes[n].previous.items=NULL; /* Max-size case checks all entries below. */
    if(change->value.count<=8) {
        if(change->value.count)memcpy(s->copied_after[n],change->value.items,change->value.count*sizeof(cui_table_selection));
        s->changes[n].value.items=s->copied_after[n];
    } else s->changes[n].value.items=NULL;
    f->remove_requested=true;assert(f->trace_length<sizeof f->trace);f->trace[f->trace_length++]='M';
    if(f->consume_selection)assert(cui_consume_event(f->ui,change->event.event_index));
}
static void set_init(set_fixture *s) {
    memset(s,0,sizeof *s);init(&s->f);s->f.state.selection=s->owner;s->f.state.selection_capacity=8;
}
static cui_table_selection_workspace workspace(set_fixture *s) {
    return (cui_table_selection_workspace){s->before,s->after,8};
}
static cui_table_set_event_result set_frame(set_fixture *s,unsigned mode,bool legacy,bool handler,
    unsigned passive,cui_table_options o,cui_table_selection_workspace w) {
    fixture *f=&s->f;assert(cui_begin_frame(f->ui,600,300,.016f));f->render_started=false;
    cui_begin_box(f->ui,900,(cui_box){.width=500,.height=250,.inert=passive==1,.layout_only=passive==2});
    size_t input_count;cui_frame_events(f->ui,&input_count);assert(input_count<=128);
    bool consumed[128];for(size_t i=0;i<input_count;i++)consumed[i]=cui_event_consumed(f->ui,i);
    cui_theme t=cui_theme_light();cui_table_set_event_result r={0};
    cui_table_set_event_handler h={.on_selection=handler&&!s->sort_only?set_changed:NULL,.on_sort=handler?sort_changed:NULL,
        .on_column_width=handler?width_changed:NULL,.on_column_move=handler?column_moved:NULL,
        .on_activate=handler&&f->observe_actions?activated:NULL,
        .on_context_request=handler&&f->observe_actions?context_requested:NULL,.user=s};
    if(mode==2) {
        if(legacy)r.events.control=cui_data_table(f->ui,90,&t,f->model,&f->state,o,label,NULL,f);
        else r=cui_data_table_set_events(f->ui,90,&t,f->model,&f->state,o,label,NULL,f,w,h);
    } else if(mode==1) {
        if(legacy)r.events.control=cui_base_data_table(f->ui,90,f->model,&f->state,o);
        else r=cui_base_data_table_set_events(f->ui,90,f->model,&f->state,o,w,h);
    } else {
        if(legacy)r.events.control=cui_table_reduce(f->ui,90,f->model,&f->state,o);
        else r=cui_table_reduce_set_events(f->ui,90,f->model,&f->state,o,w,h);
        if(r.events.control.error==CUI_TABLE_OK||r.events.control.error==CUI_TABLE_CAPACITY) {
            cui_table_submit_root(f->ui,90,f->model,&f->state,o);
            cui_table_submit_header(f->ui,90,f->model,&f->state,o);
            cui_table_submit_body(f->ui,90,f->model,&f->state,o,NULL,NULL);
        }
    }
    if(!legacy&&r.preflight!=CUI_TABLE_SET_PREFLIGHT_OK)
        for(size_t i=0;i<input_count;i++)assert(consumed[i]==cui_event_consumed(f->ui,i));
    cui_end_box(f->ui);f->draw=cui_end_frame(f->ui);assert(cui_last_error(f->ui)==CUI_OK);return r;
}
static void modified_key(fixture *f,cui_key code,unsigned mods) {
    assert(cui_queue_event(f->ui,(cui_event){.type=CUI_EVENT_KEY_DOWN,.key=code,.modifiers=mods}));
}
static void set_compare(set_fixture *a,set_fixture *b,cui_table_result ar,cui_table_result br) {
    compare(&a->f,&b->f,ar,br);assert(a->f.state.selection_count==b->f.state.selection_count);
    assert(!memcmp(a->f.state.selection,b->f.state.selection,a->f.state.selection_count*sizeof(cui_table_selection)));
}
static void set_transitions(unsigned mode) {
    set_fixture a,b;set_init(&a);set_init(&b);cui_table_options o=options();o.multiple_selection=true;
    cui_table_set_event_result ar=set_frame(&a,mode,true,false,0,o,workspace(&a));
    cui_table_set_event_result br=set_frame(&b,mode,false,true,0,o,workspace(&b));
    set_compare(&a,&b,ar.events.control,br.events.control);assert(br.events.valid&&!br.events.selection_count);
    cui_focus(a.f.ui,cui_table_cell_id(90,1,COL_A));cui_focus(b.f.ui,cui_table_cell_id(90,1,COL_A));
    key(&a.f,CUI_KEY_ENTER);key(&b.f,CUI_KEY_ENTER);key(&a.f,CUI_KEY_DOWN);key(&b.f,CUI_KEY_DOWN);
    key(&a.f,CUI_KEY_UP);key(&b.f,CUI_KEY_UP);
    ar=set_frame(&a,mode,true,false,0,o,workspace(&a));br=set_frame(&b,mode,false,true,0,o,workspace(&b));
    set_compare(&a,&b,ar.events.control,br.events.control);assert(br.events.selection_count==3&&b.calls==3);
    assert(b.changes[0].previous.count==0&&b.changes[0].value.items[0].row==1);
    assert(b.changes[1].previous.items[0].row==1&&b.changes[1].value.items[0].row==2);
    assert(b.changes[2].previous.items[0].row==2&&b.changes[2].value.items[0].row==1);
    key(&b.f,CUI_KEY_DOWN);key(&b.f,CUI_KEY_UP);
    br=set_frame(&b,mode,false,true,0,o,workspace(&b));assert(br.events.selection_count==2&&!br.events.control.selection_changed);
    key(&b.f,CUI_KEY_ENTER);assert(!set_frame(&b,mode,false,true,0,o,workspace(&b)).events.selection_count);
    /* Callback copies remain independent of later scratch reuse. */
    assert(b.changes[1].value.items[0].row==2&&b.f.remove_requested);
    key(&b.f,CUI_KEY_ESCAPE);br=set_frame(&b,mode,false,true,0,o,workspace(&b));assert(br.events.selection_count==1&&b.f.state.selection_count==0);
    key(&b.f,CUI_KEY_ESCAPE);assert(!set_frame(&b,mode,false,true,0,o,workspace(&b)).events.selection_count);
    finish(&a.f);finish(&b.f);
    /* Ctrl removes only one duplicate, preserving truthful multiplicity. */
    set_init(&b);b.owner[0]=b.owner[1]=(cui_table_selection){1,COL_A};b.f.state.selection_count=2;
    b.f.state.row=1;b.f.state.column=COL_A;set_frame(&b,mode,false,true,0,o,workspace(&b));
    cui_focus(b.f.ui,cui_table_cell_id(90,1,COL_A));modified_key(&b.f,CUI_KEY_ENTER,CUI_MOD_CTRL);
    br=set_frame(&b,mode,false,true,0,o,workspace(&b));assert(br.events.selection_count==1&&b.changes[0].previous.count==2&&b.changes[0].value.count==1);
    modified_key(&b.f,CUI_KEY_ENTER,CUI_MOD_CTRL);modified_key(&b.f,CUI_KEY_ENTER,CUI_MOD_CTRL);
    br=set_frame(&b,mode,false,true,0,o,workspace(&b));assert(br.events.selection_count==2&&!br.events.control.selection_changed);
    finish(&b.f);
    /* Range rebuild changes storage order only: canonical equal sets emit nothing. */
    set_init(&b);b.owner[0]=(cui_table_selection){2,COL_B};b.owner[1]=(cui_table_selection){2,COL_A};
    b.owner[2]=(cui_table_selection){1,COL_B};b.owner[3]=(cui_table_selection){1,COL_A};b.f.state.selection_count=4;
    b.f.state.row=2;b.f.state.column=COL_B;b.f.state.anchor_row=1;b.f.state.anchor_column=COL_A;
    set_frame(&b,mode,false,true,0,o,workspace(&b));cui_focus(b.f.ui,cui_table_cell_id(90,2,COL_B));
    modified_key(&b.f,CUI_KEY_ENTER,CUI_MOD_SHIFT);br=set_frame(&b,mode,false,true,0,o,workspace(&b));
    assert(!br.events.selection_count&&!br.events.control.selection_changed&&b.owner[0].row==1&&b.owner[0].column==COL_A);
    finish(&b.f);
    /* Earlier success remains observable when a later operation exceeds capacity. */
    set_init(&a);set_init(&b);a.f.state.selection_capacity=b.f.state.selection_capacity=1;
    set_frame(&a,mode,true,false,0,o,workspace(&a));set_frame(&b,mode,false,true,0,o,workspace(&b));
    cui_focus(a.f.ui,cui_table_cell_id(90,1,COL_A));cui_focus(b.f.ui,cui_table_cell_id(90,1,COL_A));
    key(&a.f,CUI_KEY_ENTER);key(&b.f,CUI_KEY_ENTER);modified_key(&a.f,CUI_KEY_DOWN,CUI_MOD_CTRL);modified_key(&b.f,CUI_KEY_DOWN,CUI_MOD_CTRL);
    ar=set_frame(&a,mode,true,false,0,o,workspace(&a));br=set_frame(&b,mode,false,true,0,o,workspace(&b));
    set_compare(&a,&b,ar.events.control,br.events.control);assert(br.events.control.error==CUI_TABLE_CAPACITY&&br.events.selection_count==1&&br.preflight==CUI_TABLE_SET_PREFLIGHT_OK);
    modified_key(&a.f,CUI_KEY_ENTER,CUI_MOD_SHIFT);modified_key(&b.f,CUI_KEY_ENTER,CUI_MOD_SHIFT);
    ar=set_frame(&a,mode,true,false,0,o,workspace(&a));br=set_frame(&b,mode,false,true,0,o,workspace(&b));
    set_compare(&a,&b,ar.events.control,br.events.control);assert(br.events.control.error==CUI_TABLE_CAPACITY&&!br.events.selection_count&&b.calls==1);

    finish(&a.f);finish(&b.f);
    /* Row-header effective ROW mode and column selection followed by sort. */
    set_init(&b);o.row_header=true;set_frame(&b,mode,false,true,0,o,workspace(&b));
    cui_focus(b.f.ui,cui_table_row_header_id(90,2));key(&b.f,CUI_KEY_ENTER);
    br=set_frame(&b,mode,false,true,0,o,workspace(&b));assert(br.events.selection_count==1&&b.changes[0].mode==CUI_TABLE_SELECT_ROW&&b.changes[0].value.items[0].column==0&&br.events.control.selection_mode_requested);finish(&b.f);
    set_init(&b);o.row_header=false;o.selection_mode=CUI_TABLE_SELECT_COLUMN;
    set_frame(&b,mode,false,true,0,o,workspace(&b));cui_focus(b.f.ui,cui_table_header_id(90,COL_B));key(&b.f,CUI_KEY_ENTER);
    br=set_frame(&b,mode,false,true,0,o,workspace(&b));assert(br.events.selection_count==1&&br.events.sort_count==1&&b.changes[0].value.items[0].row==0);
    assert(b.f.trace_length==2&&b.f.trace[0]=='M'&&b.f.trace[1]=='O');finish(&b.f);
    for(unsigned passive=0;passive<3;passive++)for(unsigned policy=0;policy<3;policy++) {
        set_init(&a);set_init(&b);o=options();o.multiple_selection=true;
        set_frame(&a,mode,true,false,0,o,workspace(&a));set_frame(&b,mode,false,true,0,o,workspace(&b));
        cui_focus(a.f.ui,cui_table_cell_id(90,1,COL_A));cui_focus(b.f.ui,cui_table_cell_id(90,1,COL_A));
        key(&a.f,CUI_KEY_ENTER);key(&b.f,CUI_KEY_ENTER);o.disabled=policy==1;o.loading=policy==2;
        ar=set_frame(&a,mode,true,false,passive,o,workspace(&a));br=set_frame(&b,mode,false,true,passive,o,workspace(&b));
        set_compare(&a,&b,ar.events.control,br.events.control);assert(br.events.selection_count==((!passive&&!policy)?1u:0u));finish(&a.f);finish(&b.f);
    }
    if(mode==2) {
        set_init(&b);o=options();o.multiple_selection=true;set_frame(&b,mode,false,true,0,o,workspace(&b));
        click(&b.f,cui_table_cell_id(90,2,COL_B));assert(set_frame(&b,mode,false,true,0,o,workspace(&b)).events.selection_count==1);
        assert(b.changes[0].event.source==CUI_COMPONENT_EVENT_SOURCE_POINTER&&b.changes[0].value.items[0].column==COL_B);finish(&b.f);
    }
}
static void set_ranges_and_keys(unsigned mode) {
    set_fixture s;set_init(&s);cui_table_options o=options();o.multiple_selection=true;
    s.owner[0]=(cui_table_selection){1,COL_A};s.owner[1]=(cui_table_selection){2,COL_A};s.f.state.selection_count=2;
    s.f.state.row=1;s.f.state.column=COL_A;s.f.state.anchor_row=1;s.f.state.anchor_column=COL_A;
    set_frame(&s,mode,false,true,0,o,workspace(&s));cui_focus(s.f.ui,cui_table_cell_id(90,1,COL_A));
    modified_key(&s.f,CUI_KEY_DOWN,CUI_MOD_SHIFT);
    cui_table_set_event_result primary=set_frame(&s,mode,false,true,0,o,workspace(&s));
    assert(primary.events.selection_count==1&&s.changes[0].previous.count==2&&s.changes[0].value.count==2);
    assert(!memcmp(s.changes[0].previous.items,s.changes[0].value.items,2*sizeof(cui_table_selection))&&s.changes[0].previous_primary.row==1&&s.changes[0].value_primary.row==2);
    finish(&s.f);set_init(&s);

    const cui_id first=UINT64_C(9007199254740995),last=UINT64_MAX-3;
    s.f.rows[0].key=first;s.f.rows[1].key=last;
    assert(cui_table_bind(s.f.model,s.f.rows,2,s.f.columns,2)==CUI_TABLE_OK);
    set_frame(&s,mode,false,true,0,o,workspace(&s));cui_focus(s.f.ui,cui_table_cell_id(90,first,COL_A));
    key(&s.f,CUI_KEY_ENTER);set_frame(&s,mode,false,true,0,o,workspace(&s));
    modified_key(&s.f,CUI_KEY_DOWN,CUI_MOD_SHIFT);modified_key(&s.f,CUI_KEY_RIGHT,CUI_MOD_SHIFT);
    cui_table_set_event_result r=set_frame(&s,mode,false,true,0,o,workspace(&s));
    assert(r.events.selection_count==2&&s.changes[1].previous.count==1&&s.changes[1].value.count==2&&s.changes[2].value.count==4);
    assert(s.changes[2].value.items[0].row==first&&s.changes[2].value.items[0].column==COL_A&&s.changes[2].value.items[3].row==last&&s.changes[2].value.items[3].column==COL_B);
    finish(&s.f);
    for(unsigned selection_mode=0;selection_mode<3;selection_mode++) {
        set_init(&s);o=options();o.multiple_selection=true;o.selection_mode=(cui_table_selection_mode)selection_mode;
        o.row_selection_disabled=selection_mode==CUI_TABLE_SELECT_ROW;
        o.column_selection_disabled=selection_mode==CUI_TABLE_SELECT_COLUMN;
        o.cell_selection_disabled=selection_mode==CUI_TABLE_SELECT_CELL;
        set_frame(&s,mode,false,true,0,o,workspace(&s));
        cui_focus(s.f.ui,selection_mode==CUI_TABLE_SELECT_COLUMN?cui_table_header_id(90,COL_A):cui_table_cell_id(90,1,COL_A));
        key(&s.f,CUI_KEY_ENTER);r=set_frame(&s,mode,false,true,0,o,workspace(&s));assert(!r.events.selection_count&&!s.f.state.selection_count);
        if(selection_mode==CUI_TABLE_SELECT_COLUMN)assert(r.events.sort_count==1);
        finish(&s.f);
    }
    set_init(&s);o=options();o.multiple_selection=true;s.f.columns[0].nonselectable=true;
    set_frame(&s,mode,false,true,0,o,workspace(&s));cui_focus(s.f.ui,cui_table_cell_id(90,1,COL_A));key(&s.f,CUI_KEY_ENTER);
    assert(!set_frame(&s,mode,false,true,0,o,workspace(&s)).events.selection_count);
    /* Missing observer still applies selection; no workspace is needed. */
    s.f.columns[0].nonselectable=false;key(&s.f,CUI_KEY_ENTER);
    r=set_frame(&s,mode,false,false,0,o,(cui_table_selection_workspace){0});assert(r.events.valid&&!r.events.selection_count&&s.f.state.selection_count==1);
    finish(&s.f);
}
static void set_preflight(unsigned mode) {
    set_fixture s;set_init(&s);cui_table_options o=options();o.multiple_selection=true;
    set_frame(&s,mode,false,true,0,o,workspace(&s));cui_focus(s.f.ui,cui_table_cell_id(90,1,COL_A));key(&s.f,CUI_KEY_ENTER);
    cui_table_selection_workspace bad[12];cui_table_set_preflight expected[12];size_t n=0;
#define BAD(w,e) do {bad[n]=(w);expected[n++]=(e);}while(0)
    BAD(((cui_table_selection_workspace){s.before,s.after,7}),CUI_TABLE_SET_PREFLIGHT_WORKSPACE_CAPACITY);
    BAD(((cui_table_selection_workspace){s.before,s.before,8}),CUI_TABLE_SET_PREFLIGHT_OVERLAP);
    BAD(((cui_table_selection_workspace){s.before,s.before+1,7}),CUI_TABLE_SET_PREFLIGHT_WORKSPACE_CAPACITY);
    BAD(((cui_table_selection_workspace){s.owner,s.after,8}),CUI_TABLE_SET_PREFLIGHT_OVERLAP);
    BAD(((cui_table_selection_workspace){(cui_table_selection *)&s.f.state,s.after,8}),CUI_TABLE_SET_PREFLIGHT_OVERLAP);
    BAD(((cui_table_selection_workspace){(cui_table_selection *)s.f.rows,s.after,8}),CUI_TABLE_SET_PREFLIGHT_OVERLAP);
    BAD(((cui_table_selection_workspace){(cui_table_selection *)s.f.columns,s.after,8}),CUI_TABLE_SET_PREFLIGHT_OVERLAP);
    BAD(((cui_table_selection_workspace){(cui_table_selection *)s.f.model,s.after,8}),CUI_TABLE_SET_PREFLIGHT_OVERLAP);
    BAD(((cui_table_selection_workspace){NULL,s.after,8}),CUI_TABLE_SET_PREFLIGHT_INVALID);
    BAD(((cui_table_selection_workspace){s.before,s.after,4097}),CUI_TABLE_SET_PREFLIGHT_INVALID);
    BAD(((cui_table_selection_workspace){(cui_table_selection *)((unsigned char *)s.before+1),s.after,8}),CUI_TABLE_SET_PREFLIGHT_INVALID);
    BAD(((cui_table_selection_workspace){(cui_table_selection *)(UINTPTR_MAX-(_Alignof(cui_table_selection)-1)),s.after,8}),CUI_TABLE_SET_PREFLIGHT_INVALID);
#undef BAD
    for(size_t i=0;i<n;i++) {
        size_t renders=s.f.renders;
        cui_table_state state=s.f.state;cui_table_column cols[2];memcpy(cols,s.f.columns,sizeof cols);
        cui_table_selection before[8],after[8],owner[8];memcpy(before,s.before,sizeof before);memcpy(after,s.after,sizeof after);memcpy(owner,s.owner,sizeof owner);
        cui_table_set_event_result r=set_frame(&s,mode,false,true,0,o,bad[i]);
        assert(r.preflight==expected[i]&&!r.events.valid&&!r.events.submitted&&!r.events.selection_count&&!r.events.sort_count&&!r.events.column_width_count&&!r.events.column_move_count&&!r.events.count_overflow);
        assert(r.events.control.error==CUI_TABLE_INVALID&&!s.calls&&s.f.renders==renders);
        assert(!memcmp(&state,&s.f.state,sizeof state)&&!memcmp(cols,s.f.columns,sizeof cols)&&!memcmp(before,s.before,sizeof before)&&!memcmp(after,s.after,sizeof after)&&!memcmp(owner,s.owner,sizeof owner));
        /* Requeue after each rejected frame; no failed call may consume input. */
        key(&s.f,CUI_KEY_ENTER);
    }
    set_frame(&s,mode,false,true,0,o,workspace(&s)); /* Restore semantics omitted by failed submissions. */
    cui_focus(s.f.ui,cui_table_cell_id(90,1,COL_A));key(&s.f,CUI_KEY_ENTER);
    assert(set_frame(&s,mode,false,true,0,o,workspace(&s)).events.selection_count==1);
    o.multiple_selection=false;cui_table_state saved=s.f.state;
    assert(set_frame(&s,mode,false,true,0,o,workspace(&s)).preflight==CUI_TABLE_SET_PREFLIGHT_INVALID&&!memcmp(&saved,&s.f.state,sizeof saved));
    /* Nil handler ignores even invalid scratch and preserves ordinary single mode. */
    assert(set_frame(&s,mode,false,false,0,o,bad[n-1]).events.valid);finish(&s.f);
    set_init(&s);o=options();o.multiple_selection=true;s.f.state.selection_count=9;
    saved=s.f.state;assert(set_frame(&s,mode,false,true,0,o,workspace(&s)).preflight==CUI_TABLE_SET_PREFLIGHT_INVALID);
    assert(!memcmp(&saved,&s.f.state,sizeof saved));finish(&s.f);
    set_init(&s);s.sort_only=true;o=options();o.multiple_selection=true;
    set_frame(&s,mode,false,true,0,o,bad[n-1]);cui_focus(s.f.ui,cui_table_header_id(90,COL_A));key(&s.f,CUI_KEY_ENTER);
    cui_table_set_event_result sorted=set_frame(&s,mode,false,true,0,o,bad[n-1]);
    assert(sorted.events.valid&&sorted.events.sort_count==1&&!sorted.events.selection_count&&!s.calls);finish(&s.f);
    /* Genuine partial overlap, with valid full allocations and enough capacity. */
    set_init(&s);o.multiple_selection=true;s.f.state.selection_capacity=7;
    assert(set_frame(&s,mode,false,true,0,o,(cui_table_selection_workspace){s.before,s.before+1,7}).preflight==CUI_TABLE_SET_PREFLIGHT_OVERLAP);finish(&s.f);
    set_init(&s);o=options();o.multiple_selection=true;s.f.state.selection=NULL;s.f.state.selection_capacity=0;
    set_frame(&s,mode,false,true,0,o,(cui_table_selection_workspace){0});cui_focus(s.f.ui,cui_table_cell_id(90,1,COL_A));key(&s.f,CUI_KEY_ENTER);
    cui_table_set_event_result r=set_frame(&s,mode,false,true,0,o,(cui_table_selection_workspace){0});
    assert(r.events.valid&&r.events.control.error==CUI_TABLE_CAPACITY&&!r.events.selection_count);
    s.f.state.row=1;s.f.state.column=COL_A;key(&s.f,CUI_KEY_ESCAPE);
    r=set_frame(&s,mode,false,true,0,o,(cui_table_selection_workspace){0});assert(r.events.selection_count==1&&s.changes[0].previous.count==0&&s.changes[0].value.count==0);finish(&s.f);
}
static cui_table_selection max_owner[4096],max_before[4096],max_after[4096];
static cui_table_row max_rows[4096];
static void set_maximum(void) {
    for(unsigned pattern=0;pattern<3;pattern++) {
        set_fixture s;set_init(&s);cui_table_options o=options();o.multiple_selection=true;
        for(size_t i=0;i<4096;i++)max_owner[i]=(cui_table_selection){
            pattern==0?UINT64_MAX-i:pattern==1?UINT64_C(9007199254740993)+i:UINT64_MAX,
            pattern==2?COL_B:COL_A};
        s.f.state.selection=max_owner;s.f.state.selection_capacity=s.f.state.selection_count=4096;
        cui_table_selection_workspace w={max_before,max_after,4096};
        set_frame(&s,0,false,true,0,o,w);cui_focus(s.f.ui,cui_table_cell_id(90,1,COL_A));key(&s.f,CUI_KEY_ENTER);
        cui_table_set_event_result r=set_frame(&s,0,false,true,0,o,w);
        assert(r.events.selection_count==1&&s.changes[0].previous.count==4096&&s.changes[0].value.count==1);
        for(size_t i=0;i<4096;i++) {
            cui_id expected=pattern==0?UINT64_MAX-4095+i:pattern==1?UINT64_C(9007199254740993)+i:UINT64_MAX;
            assert(max_before[i].row==expected&&max_before[i].column==(pattern==2?COL_B:COL_A));
        }
        /* Owner storage is never sorted; only original select's first entry changed. */
        assert(max_owner[1].row==(pattern==0?UINT64_MAX-1:pattern==1?UINT64_C(9007199254740994):UINT64_MAX));
        finish(&s.f);
    }
    /* A real Shift+End row-range operation creates the maximum AFTER snapshot. */
    set_fixture s;set_init(&s);cui_table_destroy(s.f.model);s.f.model=cui_table_create(4096);assert(s.f.model);
    for(size_t i=0;i<4096;i++)max_rows[i]=(cui_table_row){.key=UINT64_MAX-i};
    assert(cui_table_bind(s.f.model,max_rows,4096,s.f.columns,1)==CUI_TABLE_OK);
    s.f.state.selection=max_owner;s.f.state.selection_capacity=4096;s.f.state.selection_count=1;
    max_owner[0]=(cui_table_selection){UINT64_MAX,0};s.f.state.row=UINT64_MAX;
    s.f.state.anchor_row=UINT64_MAX;s.f.state.anchor_column=COL_A;
    cui_table_options o=options();o.multiple_selection=true;o.selection_mode=CUI_TABLE_SELECT_ROW;
    cui_table_selection_workspace w={max_before,max_after,4096};
    set_frame(&s,0,false,true,0,o,w);cui_focus(s.f.ui,cui_table_cell_id(90,UINT64_MAX,COL_A));
    modified_key(&s.f,CUI_KEY_END,CUI_MOD_SHIFT);
    cui_table_set_event_result r=set_frame(&s,0,false,true,0,o,w);
    assert(r.events.selection_count==1&&s.changes[0].previous.count==1&&s.changes[0].value.count==4096);
    for(size_t i=0;i<4096;i++)assert(max_after[i].row==UINT64_MAX-4095+i&&max_after[i].column==0);
    assert(max_owner[0].row==UINT64_MAX&&max_owner[4095].row==UINT64_MAX-4095);
    key(&s.f,CUI_KEY_ESCAPE);r=set_frame(&s,0,false,true,0,o,w);
    assert(r.events.selection_count==1&&s.changes[1].previous.count==4096&&!s.changes[1].value.count);
    finish(&s.f);
}

static cui_table_event_result column_frame(set_fixture *s,unsigned mode,bool sets,bool legacy,
    bool handler,unsigned passive,cui_table_options o) {
    o.multiple_selection=sets;
    return sets?set_frame(s,mode,legacy,handler,passive,o,workspace(s)).events:
        frame(&s->f,mode,legacy,handler,passive,o);
}
static void column_init(set_fixture *s) {
    set_init(s);
    for(size_t i=0;i<2;i++){s->f.columns[i].resizable=true;s->f.columns[i].movable=true;}
    s->f.columns[0].minimum=140;s->f.columns[0].maximum=166;
}
static void columns_compare(set_fixture *a,set_fixture *b,cui_table_event_result ar,cui_table_event_result br) {
    set_compare(a,b,ar.control,br.control);
    for(size_t i=0;i<2;i++)assert(a->f.columns[i].key==b->f.columns[i].key&&a->f.columns[i].width==b->f.columns[i].width);
    assert(a->f.state.resizing==b->f.state.resizing&&a->f.state.dragging==b->f.state.dragging&&a->f.state.drag_active==b->f.state.drag_active);
}
static void pointer(fixture *f,cui_event_type type,float x,float y,cui_pointer_button button) {
    assert(cui_queue_event(f->ui,(cui_event){.type=type,.x=x,.y=y,.pointer_button=button}));
}
static void pointer_resize_events(fixture *f,bool cancel,bool secondary) {
    cui_rect grip=cui_bounds(f->ui,cui_table_resize_id(90,COL_A));assert(grip.w>0&&grip.h>0);
    float x=grip.x+grip.w/2,y=grip.y+grip.h/2;
    cui_pointer_button button=secondary?CUI_POINTER_SECONDARY:CUI_POINTER_PRIMARY;
    pointer(f,CUI_EVENT_POINTER_DOWN,x,y,button);
    pointer(f,CUI_EVENT_POINTER_MOVE,x+80,y,button);
    pointer(f,CUI_EVENT_POINTER_MOVE,x+90,y,button); /* Clamped no-op. */
    pointer(f,CUI_EVENT_POINTER_MOVE,x-80,y,button);
    if(cancel)assert(cui_queue_event(f->ui,(cui_event){.type=CUI_EVENT_CANCEL}));
    else pointer(f,CUI_EVENT_POINTER_UP,x-80,y,button);
}
static void pointer_drag_events(fixture *f,bool cancel,float distance) {
    cui_rect first=cui_bounds(f->ui,cui_table_header_id(90,COL_A));
    cui_rect last=cui_bounds(f->ui,cui_table_header_id(90,COL_B));
    assert(first.w>0&&last.w>0);float x=first.x+20,y=first.y+first.h/2;
    float destination=distance?x+distance:last.x+last.w-20;
    pointer(f,CUI_EVENT_POINTER_DOWN,x,y,CUI_POINTER_PRIMARY);
    pointer(f,CUI_EVENT_POINTER_MOVE,destination,y,CUI_POINTER_PRIMARY);
    if(cancel)assert(cui_queue_event(f->ui,(cui_event){.type=CUI_EVENT_CANCEL}));
    else pointer(f,CUI_EVENT_POINTER_UP,destination,y,CUI_POINTER_PRIMARY);
}
static void column_changes(unsigned mode,bool sets) {
    set_fixture a,b;column_init(&a);column_init(&b);cui_table_options o=options();
    column_frame(&a,mode,sets,true,false,0,o);column_frame(&b,mode,sets,false,true,0,o);
    cui_focus(a.f.ui,cui_table_resize_id(90,COL_A));cui_focus(b.f.ui,cui_table_resize_id(90,COL_A));
    key(&a.f,CUI_KEY_RIGHT);key(&b.f,CUI_KEY_RIGHT);key(&a.f,CUI_KEY_LEFT);key(&b.f,CUI_KEY_LEFT);
    cui_table_event_result ar=column_frame(&a,mode,sets,true,false,0,o),br=column_frame(&b,mode,sets,false,true,0,o);
    columns_compare(&a,&b,ar,br);assert(br.column_width_count==2&&br.control.columns_changed&&b.f.columns[0].width==150);
    assert(b.f.widths[0].column==COL_A&&b.f.widths[0].previous_width==150&&b.f.widths[0].width==158);
    assert(b.f.widths[1].previous_width==158&&b.f.widths[1].width==150&&b.f.widths[0].event.source==CUI_COMPONENT_EVENT_SOURCE_KEYBOARD);
    /* Optional observers preserve writes; programmatic helpers never notify. */
    assert(cui_table_resize(b.f.model,COL_A,158)&&b.f.width_calls==2);
    key(&b.f,CUI_KEY_LEFT);br=column_frame(&b,mode,sets,false,false,0,o);assert(!br.column_width_count&&b.f.width_calls==2&&b.f.columns[0].width==150);
    finish(&a.f);finish(&b.f);
    column_init(&a);column_init(&b);column_frame(&a,mode,sets,true,false,0,o);column_frame(&b,mode,sets,false,true,0,o);
    cui_focus(a.f.ui,cui_table_header_id(90,COL_A));cui_focus(b.f.ui,cui_table_header_id(90,COL_A));
    modified_key(&a.f,CUI_KEY_RIGHT,CUI_MOD_CTRL);modified_key(&b.f,CUI_KEY_RIGHT,CUI_MOD_CTRL);
    modified_key(&a.f,CUI_KEY_LEFT,CUI_MOD_CTRL);modified_key(&b.f,CUI_KEY_LEFT,CUI_MOD_CTRL);
    ar=column_frame(&a,mode,sets,true,false,0,o);br=column_frame(&b,mode,sets,false,true,0,o);
    columns_compare(&a,&b,ar,br);assert(br.column_move_count==2&&br.control.columns_changed&&b.f.columns[0].key==COL_A);
    assert(b.f.moves[0].column==COL_A&&b.f.moves[0].previous_index==0&&b.f.moves[0].index==1);
    assert(b.f.moves[1].column==COL_A&&b.f.moves[1].previous_index==1&&b.f.moves[1].index==0);
    assert(b.f.moves[0].event.source==CUI_COMPONENT_EVENT_SOURCE_KEYBOARD);
    modified_key(&b.f,CUI_KEY_LEFT,CUI_MOD_CTRL);br=column_frame(&b,mode,sets,false,true,0,o);assert(!br.column_move_count&&!br.control.columns_changed);
    assert(cui_table_move_column(b.f.model,COL_A,1)&&b.f.move_calls==2);
    assert(cui_table_move_column(b.f.model,COL_A,0)&&b.f.move_calls==2);
    finish(&a.f);finish(&b.f);
    for(unsigned restriction=0;restriction<2;restriction++) {
        column_init(&a);column_init(&b);
        if(restriction==0)a.f.columns[0].disabled=b.f.columns[0].disabled=true;
        else a.f.columns[0].resizable=b.f.columns[0].resizable=false;
        column_frame(&a,mode,sets,true,false,0,o);column_frame(&b,mode,sets,false,true,0,o);
        cui_focus(a.f.ui,cui_table_resize_id(90,COL_A));cui_focus(b.f.ui,cui_table_resize_id(90,COL_A));
        key(&a.f,CUI_KEY_RIGHT);key(&b.f,CUI_KEY_RIGHT);
        ar=column_frame(&a,mode,sets,true,false,0,o);br=column_frame(&b,mode,sets,false,true,0,o);
        columns_compare(&a,&b,ar,br);assert(!br.column_width_count&&!br.control.columns_changed);finish(&a.f);finish(&b.f);
    }
    /* Fixed-region, immovable and disabled restrictions keep their old meanings. */
    for(unsigned restriction=0;restriction<4;restriction++) {
        column_init(&a);column_init(&b);
        if(restriction==0)a.f.columns[0].fixed_left=b.f.columns[0].fixed_left=true;
        if(restriction==1)a.f.columns[1].movable=b.f.columns[1].movable=false;
        if(restriction==2)a.f.columns[0].disabled=b.f.columns[0].disabled=true;
        if(restriction==3)a.f.columns[0].movable=b.f.columns[0].movable=false;
        column_frame(&a,mode,sets,true,false,0,o);column_frame(&b,mode,sets,false,true,0,o);
        cui_focus(a.f.ui,cui_table_header_id(90,COL_A));cui_focus(b.f.ui,cui_table_header_id(90,COL_A));
        modified_key(&a.f,CUI_KEY_RIGHT,CUI_MOD_CTRL);modified_key(&b.f,CUI_KEY_RIGHT,CUI_MOD_CTRL);
        ar=column_frame(&a,mode,sets,true,false,0,o);br=column_frame(&b,mode,sets,false,true,0,o);
        columns_compare(&a,&b,ar,br);assert(!br.column_move_count&&!br.control.columns_changed);finish(&a.f);finish(&b.f);
    }
    for(unsigned passive=0;passive<3;passive++)for(unsigned restricted=0;restricted<3;restricted++) {
        column_init(&a);column_init(&b);column_frame(&a,mode,sets,true,false,0,o);column_frame(&b,mode,sets,false,true,0,o);
        cui_focus(a.f.ui,cui_table_resize_id(90,COL_A));cui_focus(b.f.ui,cui_table_resize_id(90,COL_A));
        key(&a.f,CUI_KEY_RIGHT);key(&b.f,CUI_KEY_RIGHT);o.disabled=restricted==1;o.loading=restricted==2;
        ar=column_frame(&a,mode,sets,true,false,passive,o);br=column_frame(&b,mode,sets,false,true,passive,o);
        columns_compare(&a,&b,ar,br);assert(br.column_width_count==((!passive&&!restricted)?1u:0u));finish(&a.f);finish(&b.f);
        o=options();column_init(&a);column_init(&b);column_frame(&a,mode,sets,true,false,0,o);column_frame(&b,mode,sets,false,true,0,o);
        cui_focus(a.f.ui,cui_table_header_id(90,COL_A));cui_focus(b.f.ui,cui_table_header_id(90,COL_A));
        modified_key(&a.f,CUI_KEY_RIGHT,CUI_MOD_CTRL);modified_key(&b.f,CUI_KEY_RIGHT,CUI_MOD_CTRL);o.disabled=restricted==1;o.loading=restricted==2;
        ar=column_frame(&a,mode,sets,true,false,passive,o);br=column_frame(&b,mode,sets,false,true,passive,o);
        columns_compare(&a,&b,ar,br);assert(br.column_move_count==((!passive&&!restricted)?1u:0u));finish(&a.f);finish(&b.f);o=options();
    }
    /* Mixed input freezes payload facts before the moved descriptor changes slot. */
    column_init(&a);column_init(&b);column_frame(&a,2,sets,true,false,0,o);column_frame(&b,2,sets,false,true,0,o);
    pointer_resize_events(&a.f,false,false);pointer_resize_events(&b.f,false,false);
    pointer_drag_events(&a.f,false,0);pointer_drag_events(&b.f,false,0);
    ar=column_frame(&a,mode,sets,true,false,0,o);br=column_frame(&b,mode,sets,false,true,0,o);
    columns_compare(&a,&b,ar,br);assert(br.column_width_count==2&&br.column_move_count==1&&!br.sort_count);
    assert(b.f.trace_length==3&&!memcmp(b.f.trace,"WWP",3)&&b.f.columns[1].key==COL_A&&b.f.widths[1].column==COL_A&&b.f.widths[1].width==140);
    assert(b.f.widths[0].event.event_index<b.f.widths[1].event.event_index&&b.f.widths[1].event.event_index<b.f.moves[0].event.event_index);
    finish(&a.f);finish(&b.f);
    /* Prime with real Styled geometry, then reduce through each facade. Base
     * submission alone creates semantics, not painted resize/header bounds. */
    for(unsigned variant=0;variant<3;variant++) {
        column_init(&a);column_init(&b);column_frame(&a,2,sets,true,false,0,o);column_frame(&b,2,sets,false,true,0,o);
        pointer_resize_events(&a.f,variant==1,variant==2);pointer_resize_events(&b.f,variant==1,variant==2);
        ar=column_frame(&a,mode,sets,true,false,0,o);br=column_frame(&b,mode,sets,false,true,0,o);
        columns_compare(&a,&b,ar,br);assert(br.column_width_count==(variant==2?0u:2u));
        if(variant!=2){assert(b.f.widths[0].width==166&&b.f.widths[1].width==140&&b.f.columns[0].width==140);assert(b.f.widths[0].event.source==CUI_COMPONENT_EVENT_SOURCE_POINTER);}
        assert(!b.f.state.resizing);finish(&a.f);finish(&b.f);
    }
    for(unsigned variant=0;variant<4;variant++) {
        column_init(&a);column_init(&b);
        if(variant==3)a.f.columns[0].fixed_left=b.f.columns[0].fixed_left=true;
        column_frame(&a,2,sets,true,false,0,o);column_frame(&b,2,sets,false,true,0,o);
        pointer_drag_events(&a.f,variant==1,variant==2?3:0);pointer_drag_events(&b.f,variant==1,variant==2?3:0);
        ar=column_frame(&a,mode,sets,true,false,0,o);br=column_frame(&b,mode,sets,false,true,0,o);
        columns_compare(&a,&b,ar,br);assert(br.column_move_count==(variant==0?1u:0u));
        if(variant==0)assert(b.f.moves[0].column==COL_A&&b.f.moves[0].previous_index==0&&b.f.moves[0].index==1&&b.f.moves[0].event.source==CUI_COMPONENT_EVENT_SOURCE_POINTER&&!br.control.sort_changed);
        if(variant==2)assert(br.sort_count==1&&br.control.sort_changed); /* Below threshold remains a click. */
        assert(!b.f.state.dragging&&!b.f.state.drag_active&&b.f.remove_requested==(variant==0||variant==2));finish(&a.f);finish(&b.f);
    }
}

static cui_table_event_result action_frame(set_fixture *s,unsigned mode,bool sets,bool legacy,
    bool handlers,unsigned passive,cui_table_options options_value) {
    options_value.multiple_selection=sets;
    return sets?set_frame(s,mode,legacy,handlers,passive,options_value,
        s->sort_only?(cui_table_selection_workspace){0}:workspace(s)).events:
        frame(&s->f,mode,legacy,handlers,passive,options_value);
}
static void action_init(set_fixture *s) {
    set_init(s);s->f.observe_actions=true;s->f.observe_selection=true;
}
static void secondary(fixture *f,cui_id id) {
    cui_rect bounds=cui_bounds(f->ui,id);assert(bounds.w>0&&bounds.h>0);
    pointer(f,CUI_EVENT_POINTER_DOWN,bounds.x+bounds.w/2,bounds.y+bounds.h/2,CUI_POINTER_SECONDARY);
    pointer(f,CUI_EVENT_POINTER_UP,bounds.x+bounds.w/2,bounds.y+bounds.h/2,CUI_POINTER_SECONDARY);
}
static void actions(unsigned mode,bool sets) {
    set_fixture a,b;action_init(&a);action_init(&b);cui_table_options o=options();
    action_frame(&a,mode,sets,true,false,0,o);action_frame(&b,mode,sets,false,true,0,o);
    cui_focus(a.f.ui,cui_table_cell_id(90,1,COL_A));cui_focus(b.f.ui,cui_table_cell_id(90,1,COL_A));
    for(size_t i=0;i<3;i++){key(&a.f,CUI_KEY_ENTER);key(&b.f,CUI_KEY_ENTER);}
    cui_table_event_result ar=action_frame(&a,mode,sets,true,false,0,o),br=action_frame(&b,mode,sets,false,true,0,o);
    set_compare(&a,&b,ar.control,br.control);assert(br.activation_count==3&&br.selection_count==1&&!br.context_request_count);
    assert(b.f.trace_length==4&&!memcmp(b.f.trace,sets?"MAAA":"SAAA",4));
    for(size_t i=0;i<3;i++)assert(b.f.activations[i].area==CUI_TABLE_ACTION_CELL&&b.f.activations[i].row==1&&b.f.activations[i].column==COL_A);
    key(&a.f,CUI_KEY_DOWN);key(&b.f,CUI_KEY_DOWN);key(&a.f,CUI_KEY_ENTER);key(&b.f,CUI_KEY_ENTER);
    ar=action_frame(&a,mode,sets,true,false,0,o);br=action_frame(&b,mode,sets,false,true,0,o);
    set_compare(&a,&b,ar.control,br.control);assert(br.activation_count==1&&b.f.activations[3].row==2&&b.f.activations[3].event.source==CUI_COMPONENT_EVENT_SOURCE_KEYBOARD);
    /* Copies retain exact facts after later actions and scratch reuse. */
    assert(b.f.activations[0].row==1&&b.f.remove_requested);finish(&a.f);finish(&b.f);
    assert(b.f.activations[0].row==1&&b.f.activations[0].column==COL_A); /* Copied facts outlive context. */
    /* A selection observer consumes input; action metadata was already frozen. */
    action_init(&b);b.f.consume_selection=true;action_frame(&b,mode,sets,false,true,0,o);
    cui_focus(b.f.ui,cui_table_cell_id(90,1,COL_A));key(&b.f,CUI_KEY_ENTER);
    br=action_frame(&b,mode,sets,false,true,0,o);assert(br.activation_count==1&&br.selection_count==1);
    cui_component_event_metadata before=sets?b.changes[0].event:b.f.selections[0].event;
    cui_component_event_metadata after=b.f.activations[0].event;
    assert(before.flags==after.flags&&before.frame_sequence==after.frame_sequence&&before.event_index==after.event_index&&before.source==after.source);
    if(before.flags&CUI_COMPONENT_EVENT_HAS_ACTIVATION)assert(before.activation.target==after.activation.target);
    finish(&b.f);
    /* No selection observer/workspace is needed for action/context observation. */
    action_init(&b);b.f.observe_selection=false;b.sort_only=true;action_frame(&b,mode,sets,false,true,0,o);
    cui_focus(b.f.ui,cui_table_cell_id(90,1,COL_A));key(&b.f,CUI_KEY_ENTER);
    br=action_frame(&b,mode,sets,false,true,0,o);assert(br.activation_count==1&&!br.selection_count&&b.f.state.row==1);
    key(&b.f,CUI_KEY_ENTER);br=action_frame(&b,mode,sets,false,false,0,o);assert(!br.activation_count&&b.f.activation_calls==1);finish(&b.f);
    for(unsigned policy=0;policy<2;policy++) {
        action_init(&a);action_init(&b);
        if(policy==0)a.f.columns[0].nonselectable=b.f.columns[0].nonselectable=true;
        else o.cell_selection_disabled=true;
        action_frame(&a,mode,sets,true,false,0,o);action_frame(&b,mode,sets,false,true,0,o);
        cui_focus(a.f.ui,cui_table_cell_id(90,1,COL_A));cui_focus(b.f.ui,cui_table_cell_id(90,1,COL_A));key(&a.f,CUI_KEY_ENTER);key(&b.f,CUI_KEY_ENTER);
        ar=action_frame(&a,mode,sets,true,false,0,o);br=action_frame(&b,mode,sets,false,true,0,o);
        set_compare(&a,&b,ar.control,br.control);assert(br.activation_count==1&&!br.selection_count&&!b.f.state.row);finish(&a.f);finish(&b.f);o=options();
    }
    if(sets) {
        action_init(&a);action_init(&b);a.f.state.selection_capacity=b.f.state.selection_capacity=0;
        action_frame(&a,mode,true,true,false,0,o);action_frame(&b,mode,true,false,true,0,o);
        cui_focus(a.f.ui,cui_table_cell_id(90,1,COL_A));cui_focus(b.f.ui,cui_table_cell_id(90,1,COL_A));key(&a.f,CUI_KEY_ENTER);key(&b.f,CUI_KEY_ENTER);
        ar=action_frame(&a,mode,true,true,false,0,o);br=action_frame(&b,mode,true,false,true,0,o);
        set_compare(&a,&b,ar.control,br.control);assert(br.control.error==CUI_TABLE_CAPACITY&&br.activation_count==1&&!br.selection_count);finish(&a.f);finish(&b.f);
    }
    /* Row selection may be disabled, but the accepted row-header action remains. */
    action_init(&b);o.row_header=true;o.row_selection_disabled=true;
    action_frame(&b,mode,sets,false,true,0,o);cui_focus(b.f.ui,cui_table_row_header_id(90,2));key(&b.f,CUI_KEY_ENTER);
    br=action_frame(&b,mode,sets,false,true,0,o);assert(br.activation_count==1&&!br.selection_count&&!br.control.selection_mode_requested);
    assert(b.f.activations[0].area==CUI_TABLE_ACTION_ROW_HEADER&&b.f.activations[0].row==2&&!b.f.activations[0].column);finish(&b.f);o=options();
    for(unsigned passive=0;passive<3;passive++)for(unsigned policy=0;policy<3;policy++) {
        action_init(&a);action_init(&b);action_frame(&a,mode,sets,true,false,0,o);action_frame(&b,mode,sets,false,true,0,o);
        cui_focus(a.f.ui,cui_table_cell_id(90,1,COL_A));cui_focus(b.f.ui,cui_table_cell_id(90,1,COL_A));key(&a.f,CUI_KEY_ENTER);key(&b.f,CUI_KEY_ENTER);
        o.disabled=policy==1;o.loading=policy==2;
        ar=action_frame(&a,mode,sets,true,false,passive,o);br=action_frame(&b,mode,sets,false,true,passive,o);
        set_compare(&a,&b,ar.control,br.control);assert(br.activation_count==((!passive&&!policy)?1u:0u));finish(&a.f);finish(&b.f);o=options();
    }
    /* Real pointer routing uses Styled geometry for all three reducer facades. */
    action_init(&a);action_init(&b);o.row_header=true;
    const cui_id first=UINT64_C(9007199254740995),last=UINT64_MAX-3;
    a.f.rows[0].key=b.f.rows[0].key=first;a.f.rows[1].key=b.f.rows[1].key=last;
    assert(cui_table_bind(a.f.model,a.f.rows,2,a.f.columns,2)==CUI_TABLE_OK);
    assert(cui_table_bind(b.f.model,b.f.rows,2,b.f.columns,2)==CUI_TABLE_OK);
    action_frame(&a,2,sets,true,false,0,o);action_frame(&b,2,sets,false,true,0,o);
    click(&a.f,cui_table_cell_id(90,first,COL_B));click(&b.f,cui_table_cell_id(90,first,COL_B));
    click(&a.f,cui_table_row_header_id(90,last));click(&b.f,cui_table_row_header_id(90,last));
    ar=action_frame(&a,mode,sets,true,false,0,o);br=action_frame(&b,mode,sets,false,true,0,o);
    set_compare(&a,&b,ar.control,br.control);assert(br.activation_count==2&&br.control.activated_row==last&&br.control.activated_column==COL_B);
    assert(b.f.activations[0].row==first&&b.f.activations[0].column==COL_B&&b.f.activations[1].row==last&&!b.f.activations[1].column&&b.f.activations[1].area==CUI_TABLE_ACTION_ROW_HEADER);
    assert(b.f.activations[0].event.source==CUI_COMPONENT_EVENT_SOURCE_POINTER);finish(&a.f);finish(&b.f);
    /* Default table roles support a11y focus, but not ACTIVATE. Do not remap. */
    action_init(&b);o=options();action_frame(&b,mode,sets,false,true,0,o);
    cui_id cell=cui_table_cell_id(90,1,COL_A);
    assert(cui_accessibility_action_supported(b.f.ui,cell,CUI_ACCESSIBILITY_FOCUS));
    assert(!cui_accessibility_action_supported(b.f.ui,cell,CUI_ACCESSIBILITY_ACTIVATE));
    assert(cui_queue_accessibility_action(b.f.ui,cell,CUI_ACCESSIBILITY_FOCUS));
    assert(!action_frame(&b,mode,sets,false,true,0,o).activation_count&&cui_focused(b.f.ui)==cell);
    assert(!cui_queue_accessibility_action(b.f.ui,cell,CUI_ACCESSIBILITY_ACTIVATE));
    assert(cui_last_error(b.f.ui)==CUI_ERROR_INVALID_ACTION&&!b.f.activation_calls);finish(&b.f);
}
static void contexts(unsigned mode,bool sets) {
    set_fixture a,b;action_init(&a);action_init(&b);cui_table_options o=options();o.row_header=true;
    b.sort_only=true;b.f.observe_selection=false; /* Context-only set handling with no workspace. */
    a.f.state.row=b.f.state.row=1;a.f.state.column=b.f.state.column=COL_A;
    if(sets){a.owner[0]=b.owner[0]=(cui_table_selection){1,COL_A};a.f.state.selection_count=b.f.state.selection_count=1;}
    action_frame(&a,2,sets,true,false,0,o);action_frame(&b,2,sets,false,true,0,o);
    cui_id targets[]={cui_table_cell_id(90,2,COL_B),cui_table_row_header_id(90,2),cui_table_header_id(90,COL_A),cui_table_cell_id(90,2,COL_B)};
    for(size_t i=0;i<4;i++){secondary(&a.f,targets[i]);secondary(&b.f,targets[i]);}
    cui_table_event_result ar=action_frame(&a,mode,sets,true,false,0,o),br=action_frame(&b,mode,sets,false,true,0,o);
    set_compare(&a,&b,ar.control,br.control);assert(br.context_request_count==4&&!br.activation_count&&!br.selection_count&&b.f.state.row==1&&b.f.state.column==COL_A);
    assert(b.f.contexts[0].area==CUI_TABLE_CONTEXT_CELL&&b.f.contexts[0].row==2&&b.f.contexts[0].column==COL_B);
    assert(b.f.contexts[1].area==CUI_TABLE_CONTEXT_ROW_HEADER&&b.f.contexts[1].row==2&&!b.f.contexts[1].column);
    assert(b.f.contexts[2].area==CUI_TABLE_CONTEXT_COLUMN_HEADER&&!b.f.contexts[2].row&&b.f.contexts[2].column==COL_A);
    assert(b.f.contexts[3].row==2&&br.control.context_row==2&&br.control.context_column==COL_B&&b.f.trace_length==4&&!memcmp(b.f.trace,"QQQQ",4));
    action_frame(&b,2,sets,false,true,0,o);secondary(&b.f,cui_table_cell_id(90,2,COL_B));
    br=action_frame(&b,mode,sets,false,false,0,o);assert(br.control.context_requested&&!br.context_request_count&&b.f.context_calls==4);
    finish(&a.f);finish(&b.f);assert(b.f.contexts[0].row==2&&b.f.contexts[0].column==COL_B);
    for(unsigned policy=0;policy<3;policy++) {
        action_init(&a);action_init(&b);o=options();o.row_header=true;
        if(policy==0)a.f.rows[1].disabled=b.f.rows[1].disabled=true;
        if(policy==1)a.f.columns[1].disabled=b.f.columns[1].disabled=true;
        if(policy==2)a.f.columns[1].nonselectable=b.f.columns[1].nonselectable=true;
        action_frame(&a,2,sets,true,false,0,o);action_frame(&b,2,sets,false,true,0,o);
        secondary(&a.f,cui_table_cell_id(90,2,COL_B));secondary(&b.f,cui_table_cell_id(90,2,COL_B));
        secondary(&a.f,cui_table_row_header_id(90,2));secondary(&b.f,cui_table_row_header_id(90,2));
        ar=action_frame(&a,mode,sets,true,false,0,o);br=action_frame(&b,mode,sets,false,true,0,o);
        set_compare(&a,&b,ar.control,br.control);assert(br.context_request_count==(policy==0?0u:policy==1?1u:2u));
        if(policy==1){assert(b.f.contexts[0].area==CUI_TABLE_CONTEXT_ROW_HEADER);}
        finish(&a.f);finish(&b.f);
    }
    for(unsigned passive=0;passive<3;passive++)for(unsigned policy=0;policy<3;policy++) {
        action_init(&a);action_init(&b);o=options();action_frame(&a,2,sets,true,false,0,o);action_frame(&b,2,sets,false,true,0,o);
        secondary(&a.f,cui_table_cell_id(90,2,COL_A));secondary(&b.f,cui_table_cell_id(90,2,COL_A));o.disabled=policy==1;o.loading=policy==2;
        ar=action_frame(&a,mode,sets,true,false,passive,o);br=action_frame(&b,mode,sets,false,true,passive,o);
        set_compare(&a,&b,ar.control,br.control);assert(br.context_request_count==((!passive&&!policy)?1u:0u));finish(&a.f);finish(&b.f);
    }
}

int main(void){for(unsigned mode=0;mode<3;++mode){sorts(mode);selections(mode);set_transitions(mode);set_ranges_and_keys(mode);set_preflight(mode);column_changes(mode,false);column_changes(mode,true);actions(mode,false);actions(mode,true);contexts(mode,false);contexts(mode,true);}progress_and_consumption();set_maximum();return 0;}
