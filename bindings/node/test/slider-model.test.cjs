'use strict';
const assert=require('node:assert/strict');
const c=require('..');
const source=0xfedcba9876543210n;
function make(single=false,vertical=false) {
 const app=new c.App({width:420,height:360});
 const node=c.slider_model('typed',c.slider_config({sourceGeneration:source,orientation:vertical?'vertical':'horizontal',length:200}),single?c.slider_single(60):c.slider_range(20,60));
 app.mount(c.column().children(c.row().children(node)));app.step_only();return {app,node};
}
function focus(app,node) {
 const b=node.part_bounds(4);assert.ok(b&&b.width>0&&b.height>0);
 app.pointer('down',b.x+b.width/2,b.y+b.height/2).pointer('up',b.x+b.width/2,b.y+b.height/2).step_only();
 while(app.poll_slider_request()!==undefined){};
}
for(const single of [false,true])for(const vertical of [false,true]) {
 const {app,node}=make(single,vertical);
 try {
  assert.equal(app.supports_slider_model(),true);focus(app,node);
  const initial=node.get_value(),requests=[];
  node.on_request(r=>{requests.push(r);if(r.phase===1)app.accept_slider_request(r.receipt,single?c.slider_single(55):c.slider_range(25,55));});
  app.key('down',vertical?'down':'left',15,true).key('down',vertical?'down':'left',15,true).step_only();
  assert.deepEqual(requests,[]);assert.equal(app.dispatch_slider_requests(),6);
  assert.deepEqual(requests.map(r=>r.phase),[0,1,2,0,1,2]);
  for(const r of requests){assert.deepEqual(r.current,initial);assert.equal(r.receipt.sourceGeneration,source);assert.equal(r.input.modifiers,15);assert.equal(r.input.repeat,1);assert.equal(typeof r.eventTarget,'bigint');assert.ok(Object.isFrozen(r.input));}
  assert.deepEqual(node.get_value(),single?c.slider_single(55):c.slider_range(25,55));
  assert.notDeepEqual(requests[1].requested,requests[4].requested);
  assert.throws(()=>app.accept_slider_request({...requests[1].receipt},initial),/reconstructed/);
  assert.throws(()=>{requests[1].receipt.node.slot=1;},TypeError);
  const saved=requests[1];app.step_only();assert.throws(()=>app.accept_slider_request(saved.receipt,initial));
  assert.deepEqual(saved.current,initial);
 }finally{app.dispose();}
}
// Rejection and copied records retain full double precision.
{
 const {app,node}=make();try{focus(app,node);const owner=node.get_value();app.key('down','left').step_only();
 const all=[];let r;while((r=app.poll_slider_request())!==undefined)all.push(r);
 assert.deepEqual(node.get_value(),owner);assert.equal(all.length,3);
 const precise=c.slider_range(20,60+Number.EPSILON*64);app.accept_slider_request(all[1].receipt,precise);assert.deepEqual(node.get_value(),precise);
 node.configure({...node.get_config(),sourceGeneration:source+1n},precise);
 assert.throws(()=>app.accept_slider_request(all[2].receipt,owner));
 }finally{app.dispose();}
}
// Pop once; exception leaves later entries available and resets guard.
{
 const {app,node}=make();try{focus(app,node);node.on_request(()=>{throw new Error('callback sentinel');});app.key('down','right').step_only();assert.throws(()=>app.dispatch_slider_requests(),/sentinel/);assert.equal(app.sliderDispatching,false);const r=app.poll_slider_request();assert.equal(r.phase,1);assert.equal(app.poll_slider_request().phase,2);assert.equal(app.poll_slider_request(),undefined);
 }finally{app.dispose();}
}
// Existing Node close requests shutdown; dispose actually destroys. Neither polls again.
for(const teardown of ['close','dispose']) {
 const {app,node}=make();focus(app,node);const records=[];
 node.on_request(r=>{records.push(r);if(r.phase===1){app.accept_slider_request(r.receipt,r.requested);app[teardown]();}});
 app.key('down','right').step_only();assert.equal(app.dispatch_slider_requests(),2);assert.deepEqual(records.map(r=>r.phase),[0,1]);assert.equal(node.requestValue,null);assert.equal(app.sliderDispatching,false);assert.equal(records[1].receipt.sourceGeneration,source);assert.equal(records[1].requested.kind,'range');app.dispose();
}
// Controlled-only recursion guards and removal prune remaining native requests.
{
 const {app,node}=make();try{focus(app,node);node.on_request(r=>{
 for(const f of [()=>app.step(),()=>app.step_only(),()=>app.run(),()=>app._dispatch(),()=>app.dispatch_slider_requests()])assert.throws(f,/controlled Slider/);
 if(r.phase===1)node.remove();});app.key('down','right').step_only();assert.equal(app.dispatch_slider_requests(),2);assert.equal(node.requestValue,null);assert.equal(node.owner,null);
 }finally{app.dispose();}
}
// Preflight avoids partial mount; inactive parent length is ignored.
{
 const app=new c.App();try{const n=c.slider_model('typed',{},c.slider_single(40));assert.throws(()=>app.mount(c.root_view().children(n)),/ancestry/);assert.equal(app.nodes.size,0);assert.equal(app.root,null);}finally{app.dispose();}
 assert.equal(c.slider_config({layout:'parent',length:NaN}).length,0);
 assert.throws(()=>c.slider_config({sourceGeneration:1}),/BigInt/);assert.throws(()=>c.slider_single(NaN));assert.throws(()=>c.slider_range(3,2));
}
console.log('PASS controlled Slider copied requests, both axes/pairs, explicit ownership, teardown and guards');
// Existing scalar autoaccept/CHANGE dispatch stays before the new independent FIFO.
{
 const app=new c.App({width:640,height:480}),order=[];
 const legacy=c.slider('volume').range(0,100,1).value(40).on_change(()=>order.push('legacy'));
 const node=c.slider_model('controlled',{reducedMotion:true},c.slider_range(20,60)).on_request(r=>order.push(r.phase));
 try{app.mount(c.column().children(legacy,node));app.step_only();app.step_only();
  for(const [n,fraction]of [[legacy,.7],[node,.5]]){const b=n===node?node.part_bounds(4):n.bounds();app.pointer('down',b.x+b.width*fraction,b.y+b.height/2).pointer('up',b.x+b.width*fraction,b.y+b.height/2);}
  app.step_only();assert.deepEqual(order,[]);app._dispatch();assert.equal(order[0],'legacy');assert.ok(order.slice(1).some(x=>typeof x==='number'));assert.equal(legacy.current_value(),70);assert.deepEqual(node.get_value(),c.slider_range(20,60));
 }finally{app.dispose();}
}
// Current readonly/disabled policy and configuration revisions remain C authority.
{
 const {app,node}=make();try{focus(app,node);app.key('down','right').step_only();const requests=[];let r;while((r=app.poll_slider_request())!==undefined)requests.push(r);
  node.configure({...node.get_config(),readonly:true},c.slider_range(30,70));assert.throws(()=>app.accept_slider_request(requests[1].receipt,requests[1].requested));app.step_only();app.key('down','left').step_only();assert.equal(app.poll_slider_request(),undefined);
  node.disabled();app.step_only();assert.equal(node.part_bounds(4),undefined);assert.ok(node.part_bounds(1));assert.ok(node.part_bounds(2));assert.deepEqual(node.get_value(),c.slider_range(30,70));
 }finally{app.dispose();}
}
