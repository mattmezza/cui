'use strict';
const assert = require('node:assert/strict');
const c = require('..');
function click(app,node) {const r=node.bounds(); app.pointer('down',r.x+r.width/2,r.y+r.height/2).pointer('up',r.x+r.width/2,r.y+r.height/2).step_only();}
let saved;
if (!process.env.CUI_EXPECT_OLD) {
 const app=new c.App({width:800,height:800});
 try {
  const number=c.number_input('number').range(0,100000000,.25).value(16777216.25);
  let calls=0; number.on_change(()=>calls++);
  app.mount(c.column().children(number)); app.step_only(); click(app,number);
  app.key('down','up').step_only();
  saved=app.poll_value_notification_v1();
  assert.equal(saved.kind,'scalar-change'); assert.equal(saved.value,16777216.5);
  assert.equal(typeof saved.frame,'bigint'); assert.equal(calls,0); assert(Object.isFrozen(saved));
  assert.equal(app.poll_value_notification_v1(),undefined);
 } finally {app.dispose();}
}
if (saved) assert.equal(saved.value,16777216.5);
{
 const app=new c.App();
 try {
  const field=c.input('text',''); app.mount(c.column().children(field)); app.step_only(); click(app,field);
  app.text_input('héllo ✓').step_only();
  if(process.env.CUI_EXPECT_OLD) {
   assert.throws(()=>app.poll_value_notification_v1(),/unavailable/);
   assert.equal(app.poll_notification_v1().legacy.text,'héllo ✓');
  } else {
   assert.equal(app.poll_notification_v1().kind,'legacy');
   app.text_input('!').step_only(); app.text_input('?').step_only(); saved=app.poll_value_notification_v1();
   assert.equal(saved.kind,'text-change'); assert.equal(saved.text,'héllo ✓!');
   assert.equal(app.poll_notification_v1().legacy.text,'héllo ✓!?');
   field.label('programmatic');app.step_only();assert.equal(app.poll_value_notification_v1(),undefined);
   app.text_input('later').step_only(); assert.equal(app._dispatch(),0);
   assert.equal(saved.text,'héllo ✓!');
  }
 } finally {app.dispose();}
}
if (!process.env.CUI_EXPECT_OLD) {
 const app=new c.App();
 try {
  const first=c.item('first','First'),second=c.item('second','Second');
  const group=c.radio_group('group').children(first,second).select(first);
  app.mount(c.column().children(group));app.step_only();click(app,second);
  saved=app.poll_value_notification_v1();assert.equal(saved.kind,'selection-change');assert.equal(saved.selectedNode,second);
  click(app,first);assert.equal(app.poll_notification_v1().kind,'legacy');
  click(app,second);const handle={...saved.selectedHandle};second.remove();
  const native=require(process.env.CUI_NODE_ADDON);
  const symbol=Object.getOwnPropertySymbols(group)[0];
  const replacement=native.makeNode(app.handle,group[symbol],20,'replacement');
  const id=native.nodeId(replacement);assert.equal(id.slot,handle.slot);assert.notEqual(id.generation,handle.generation);
  const historical=app.poll_value_notification_v1();assert.deepEqual(historical.selectedHandle,handle);assert.equal(historical.selectedNode,undefined);
  assert(Object.isFrozen(historical.selectedHandle));
 } finally {app.dispose();}
}
console.log('owning value notifications real queue PASS');
