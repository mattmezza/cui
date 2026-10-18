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
    size_t selection_calls, trace_length;
    char trace[128];
    bool observe_selection;
    size_t sort_calls, renders;
    bool render_started, remove_requested, consume_first;
    cui_draw_list draw;
} fixture;
static void init(fixture *f) {
    memset(f,0,sizeof *f);f->ui=cui_create((cui_config){.width=600,.height=300});assert(f->ui);
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
        .on_sort=handler?sort_changed:NULL,.user=f};
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
int main(void){for(unsigned mode=0;mode<3;++mode){sorts(mode);selections(mode);}progress_and_consumption();return 0;}
