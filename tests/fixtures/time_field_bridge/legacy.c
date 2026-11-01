/* SPDX-License-Identifier: MIT */
/* Old ABI only; unchanged program runs in separate processes against old/new DSOs. */
#include "cui/cui_bridge.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
static void snapshot(cui_bridge_app *a, cui_bridge_node n, const char *phase) {
    cui_bridge_time t;
    cui_bridge_rect r;
    assert(cui_bridge_get_time(a,n,&t)==CUI_BRIDGE_OK);
    assert(cui_bridge_node_bounds(a,n,&r)==CUI_BRIDGE_OK);
    printf("%s node=%" PRIu32 ":%" PRIu32 " value=%" PRIu32 ":%" PRIu32 ":%" PRIu32 " bounds=%a,%a,%a,%a\n",phase,n.slot,n.generation,
        t.hour,t.minute,t.second,(double)r.x,(double)r.y,(double)r.width,(double)r.height);
    cui_bridge_event_ex e;
    char text[32]; uint32_t required;
    cui_bridge_status s;
    while((s=cui_bridge_poll_event_ex(a,&e,text,sizeof text,&required))==CUI_BRIDGE_OK)
        printf("event=%" PRIu32 " node=%" PRIu32 ":%" PRIu32 " frame=%" PRIu64 " text=%" PRIu32 ":%s\n",e.type,e.node.slot,e.node.generation,
            e.frame_sequence,required,text);
    assert(s==CUI_BRIDGE_EMPTY);
}
int main(void) {
    cui_bridge_options o={.struct_size=sizeof o,.abi_version=1,.width=640,.height=480};
    cui_bridge_status s;
    cui_bridge_app *a=cui_bridge_create(&o,&s); assert(a&&s==CUI_BRIDGE_OK);
    cui_bridge_node root,row,n;
    assert(cui_bridge_node_create(a,(cui_bridge_node){0},CUI_BRIDGE_COLUMN,"root",&root)==CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(a,root,CUI_BRIDGE_ROW,"row",&row)==CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(a,row,CUI_BRIDGE_TIME_FIELD,"time",&n)==CUI_BRIDGE_OK);
    assert(cui_bridge_set_time(a,n,(cui_bridge_time){9,45,30})==CUI_BRIDGE_OK);
    assert(cui_bridge_step(a,.016f)==CUI_BRIDGE_OK); snapshot(a,n,"initial");
    cui_bridge_rect r; assert(cui_bridge_node_bounds(a,n,&r)==CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(a,CUI_BRIDGE_POINTER_DOWN,r.x+r.width/2,r.y+r.height/2)==CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(a,CUI_BRIDGE_POINTER_UP,r.x+r.width/2,r.y+r.height/2)==CUI_BRIDGE_OK);
    assert(cui_bridge_step(a,.016f)==CUI_BRIDGE_OK);
    assert(cui_bridge_queue_key(a,CUI_BRIDGE_KEY_DOWN,CUI_BRIDGE_KEY_UP_ARROW,0,0)==CUI_BRIDGE_OK);
    assert(cui_bridge_step(a,.016f)==CUI_BRIDGE_OK); snapshot(a,n,"key");
    assert(cui_bridge_set_time(a,n,(cui_bridge_time){24,0,0})==CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_time(a,n,(cui_bridge_time){23,59,59})==CUI_BRIDGE_OK);
    assert(cui_bridge_set_disabled(a,n,1)==CUI_BRIDGE_OK);
    assert(cui_bridge_step(a,.016f)==CUI_BRIDGE_OK);
    assert(cui_bridge_queue_key(a,CUI_BRIDGE_KEY_DOWN,CUI_BRIDGE_KEY_UP_ARROW,0,0)==CUI_BRIDGE_OK);
    assert(cui_bridge_step(a,.016f)==CUI_BRIDGE_OK); snapshot(a,n,"disabled");
    cui_bridge_destroy(a); return 0;
}
