/* SPDX-License-Identifier: MIT */
#include "cui/cui_edit.h"
#include <stdlib.h>
#include <string.h>
typedef struct { size_t length,cursor,anchor; } snapshot;
struct cui_edit { cui_edit_config config; size_t length,cursor,anchor; char *history,*scratch; snapshot *states; size_t slots,count,index; uint64_t revision; };
static bool decode(const unsigned char *s,size_t n,size_t *width) {
    if(!n||s[0]==0)return false;
    unsigned char a=s[0];size_t z;unsigned cp;
    if(a<0x80){*width=1;return true;}
    if(a>=0xc2&&a<=0xdf){z=2;cp=a&31u;}else if(a>=0xe0&&a<=0xef){z=3;cp=a&15u;}else if(a>=0xf0&&a<=0xf4){z=4;cp=a&7u;}else return false;
    if(n<z)return false;
    for(size_t i=1;i<z;i++){if((s[i]&0xc0u)!=0x80u)return false;cp=(cp<<6)|(s[i]&63u);}
    if((z==2&&cp<0x80)||(z==3&&cp<0x800)||(z==4&&cp<0x10000)||cp>0x10ffff||(cp>=0xd800&&cp<=0xdfff))return false;
    *width=z;return true;
}
bool cui_edit_valid_utf8(cui_str text) { if(text.length&&!text.data)return false;size_t i=0;while(i<text.length){size_t z;if(!decode((const unsigned char *)text.data+i,text.length-i,&z))return false;i+=z;}return true; }
static bool accepted(cui_edit *e,cui_str t) { if(!cui_edit_valid_utf8(t))return false;if(!e->config.multiline)for(size_t i=0;i<t.length;i++)if(t.data[i]=='\n'||t.data[i]=='\r')return false;return true; }
static bool boundary(cui_edit *e,size_t p) { return p<=e->length&&(p==e->length||((unsigned char)e->config.buffer[p]&0xc0u)!=0x80u); }
static void save_state(cui_edit *e) { if(e->slots)e->states[e->index]=(snapshot){e->length,e->cursor,e->anchor}; }
static void record(cui_edit *e) {
    if(!e->slots)return;
    if(!e->count){e->count=1;e->index=0;memcpy(e->history,e->config.buffer,e->length+1);save_state(e);return;}
    e->count=e->index+1;
    if(e->count==e->slots){memmove(e->history,e->history+e->config.capacity,(e->slots-1)*e->config.capacity);memmove(e->states,e->states+1,(e->slots-1)*sizeof(*e->states));e->count--;}
    e->index=e->count++;memcpy(e->history+e->index*e->config.capacity,e->config.buffer,e->length+1);save_state(e);
}
cui_edit *cui_edit_create(cui_edit_config c) {
    if(!c.buffer||!c.capacity)return NULL;
    size_t n=0;while(n<c.capacity&&c.buffer[n])n++;if(n==c.capacity)return NULL;
    cui_edit *e=calloc(1,sizeof(*e));if(!e)return NULL;e->config=c;e->length=e->cursor=e->anchor=n;
    e->scratch=malloc(c.capacity);if(!e->scratch){free(e);return NULL;}
    if(!accepted(e,(cui_str){c.buffer,n})){cui_edit_destroy(e);return NULL;}
    if(c.capacity>SIZE_MAX-sizeof(snapshot)){cui_edit_destroy(e);return NULL;}
    e->slots=c.history_bytes/(c.capacity+sizeof(snapshot));
    if(c.max_history_steps&&c.max_history_steps<SIZE_MAX&&e->slots>c.max_history_steps+1)e->slots=c.max_history_steps+1;
    if(e->slots<2)e->slots=0;
    if(e->slots){e->history=malloc(e->slots*c.capacity);e->states=calloc(e->slots,sizeof(*e->states));if(!e->history||!e->states){cui_edit_destroy(e);return NULL;}record(e);}
    return e;
}
void cui_edit_destroy(cui_edit *e) { if(e){free(e->scratch);free(e->history);free(e->states);free(e);} }
cui_edit_state cui_edit_get_state(const cui_edit *e) { return e?(cui_edit_state){e->length,e->cursor,e->anchor,e->slots&&e->index>0,e->slots&&e->index+1<e->count,e->revision,e->config.multiline}:(cui_edit_state){0}; }
cui_str cui_edit_get_text(const cui_edit *e) { return e?(cui_str){e->config.buffer,e->length}:(cui_str){0}; }
cui_edit_result cui_edit_select(cui_edit *e,size_t a,size_t p) { if(!e)return CUI_EDIT_INVALID_ARGUMENT;if(!boundary(e,a)||!boundary(e,p))return CUI_EDIT_BOUNDARY;e->anchor=a;e->cursor=p;save_state(e);return CUI_EDIT_OK; }
static size_t previous(cui_edit *e,size_t p) { if(p){p--;while(p&&((unsigned char)e->config.buffer[p]&0xc0u)==0x80u)p--;}return p; }
static size_t next(cui_edit *e,size_t p) { if(p<e->length){p++;while(p<e->length&&((unsigned char)e->config.buffer[p]&0xc0u)==0x80u)p++;}return p; }
cui_edit_result cui_edit_move(cui_edit *e,cui_edit_motion m,bool extend) {
    if(!e)return CUI_EDIT_INVALID_ARGUMENT;
    size_t p=e->cursor;
    if(!extend&&e->anchor!=e->cursor&&(m==CUI_EDIT_PREVIOUS||m==CUI_EDIT_NEXT))p=m==CUI_EDIT_PREVIOUS?(e->anchor<p?e->anchor:p):(e->anchor>p?e->anchor:p);
    else switch(m){case CUI_EDIT_PREVIOUS:p=previous(e,p);break;case CUI_EDIT_NEXT:p=next(e,p);break;case CUI_EDIT_START:p=0;break;case CUI_EDIT_END:p=e->length;break;case CUI_EDIT_LINE_START:while(p&&e->config.buffer[p-1]!='\n')p=previous(e,p);break;case CUI_EDIT_LINE_END:while(p<e->length&&e->config.buffer[p]!='\n')p=next(e,p);break;default:return CUI_EDIT_INVALID_ARGUMENT;}
    return cui_edit_select(e,extend?e->anchor:p,p);
}
static cui_edit_result replace(cui_edit *e,cui_str text,size_t start,size_t end) {
    if(!cui_edit_valid_utf8(text))return CUI_EDIT_INVALID_UTF8;
    if(!accepted(e,text))return CUI_EDIT_LINE_BREAK;
    size_t remaining=e->length-(end-start);if(text.length>e->config.capacity-1-remaining)return CUI_EDIT_CAPACITY;
    if(!text.length&&start==end)return CUI_EDIT_OK;
    if(text.length==end-start&&(!text.length||!memcmp(e->config.buffer+start,text.data,text.length))){e->cursor=e->anchor=start+text.length;save_state(e);return CUI_EDIT_OK;}
    /* Support a slice of the attached buffer without invalidating its source. */
    if(text.length)memcpy(e->scratch,text.data,text.length);
    save_state(e);memmove(e->config.buffer+start+text.length,e->config.buffer+end,e->length-end+1);if(text.length)memcpy(e->config.buffer+start,e->scratch,text.length);
    e->length=remaining+text.length;e->cursor=e->anchor=start+text.length;e->revision++;record(e);return CUI_EDIT_OK;
}
cui_edit_result cui_edit_insert(cui_edit *e,cui_str text) {
    if(!e)return CUI_EDIT_INVALID_ARGUMENT;
    return replace(e,text,e->cursor<e->anchor?e->cursor:e->anchor,e->cursor>e->anchor?e->cursor:e->anchor);
}
static cui_edit_result erase(cui_edit *e,bool backwards) {
    if(!e)return CUI_EDIT_INVALID_ARGUMENT;
    if(e->anchor!=e->cursor)return cui_edit_insert(e,(cui_str){0});
    size_t start=backwards?previous(e,e->cursor):e->cursor,end=backwards?e->cursor:next(e,e->cursor);
    return replace(e,(cui_str){0},start,end);
}
cui_edit_result cui_edit_backspace(cui_edit *e) { return erase(e,true); }
cui_edit_result cui_edit_delete(cui_edit *e) { return erase(e,false); }
static void restore(cui_edit *e) { snapshot s=e->states[e->index];e->length=s.length;e->cursor=s.cursor;e->anchor=s.anchor;memcpy(e->config.buffer,e->history+e->index*e->config.capacity,e->length+1); }
cui_edit_result cui_edit_undo(cui_edit *e) { if(!e)return CUI_EDIT_INVALID_ARGUMENT;if(!e->slots||!e->index)return CUI_EDIT_NO_HISTORY;save_state(e);e->index--;restore(e);e->revision++;return CUI_EDIT_OK; }
cui_edit_result cui_edit_redo(cui_edit *e) { if(!e)return CUI_EDIT_INVALID_ARGUMENT;if(!e->slots||e->index+1>=e->count)return CUI_EDIT_NO_HISTORY;save_state(e);e->index++;restore(e);e->revision++;return CUI_EDIT_OK; }
cui_edit_result cui_edit_apply_events(cui_edit *e,cui_context *ui,cui_id id,bool disabled) {
    if(!e||!ui||!id)return CUI_EDIT_INVALID_ARGUMENT;
    if(disabled)return CUI_EDIT_OK;
    size_t count;const cui_event *events=cui_frame_events(ui,&count);cui_edit_result first=CUI_EDIT_OK;
    for(size_t i=0;i<count;i++){
        if(cui_event_target(ui,i)!=id)continue;
        cui_event v=events[i];cui_edit_result r=CUI_EDIT_OK;
        if(v.type==CUI_EVENT_TEXT){size_t n=0;while(n<sizeof(v.text)&&v.text[n])n++;r=n==sizeof(v.text)?CUI_EDIT_INVALID_ARGUMENT:cui_edit_insert(e,(cui_str){v.text,n});}
        else if(v.type==CUI_EVENT_KEY_DOWN){bool shift=(v.modifiers&CUI_MOD_SHIFT)!=0;switch(v.key){case CUI_KEY_LEFT:r=cui_edit_move(e,CUI_EDIT_PREVIOUS,shift);break;case CUI_KEY_RIGHT:r=cui_edit_move(e,CUI_EDIT_NEXT,shift);break;case CUI_KEY_HOME:r=cui_edit_move(e,(v.modifiers&CUI_MOD_CTRL)?CUI_EDIT_START:CUI_EDIT_LINE_START,shift);break;case CUI_KEY_END:r=cui_edit_move(e,(v.modifiers&CUI_MOD_CTRL)?CUI_EDIT_END:CUI_EDIT_LINE_END,shift);break;case CUI_KEY_BACKSPACE:r=cui_edit_backspace(e);break;case CUI_KEY_DELETE:r=cui_edit_delete(e);break;case CUI_KEY_ENTER:if(e->config.multiline)r=cui_edit_insert(e,cui_string("\n"));break;case CUI_KEY_A:if(v.modifiers&(CUI_MOD_CTRL|CUI_MOD_META))r=cui_edit_select(e,0,e->length);break;default:break;}}
        if(first==CUI_EDIT_OK&&r!=CUI_EDIT_OK)first=r;
    }return first;
}
