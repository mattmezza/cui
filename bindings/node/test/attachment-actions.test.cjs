'use strict';
const test=require('node:test'),assert=require('node:assert/strict');
const c=require('..'),native=require(process.env.CUI_NODE_ADDON);
function enter(app){app.key('down','enter').key('up','enter').step_only();}
function tab(app){app.key('down','tab').key('up','tab').step_only();}
test('owned Attachment actions use real keyboard input and shared FIFO',{skip:process.env.CUI_TEST_LEGACY_ATTACHMENT_ACTIONS==='1'},()=>{
 const app=new c.App();let saved;
 try {
  const card=c.attachment('file','Plan.pdf').detail('Failed').attachment_status('failed').attachment_actions({open:true,remove:true,retry:true});
  app.mount(c.column().children(card)).step_only().step_only();tab(app);enter(app);
  saved=app.poll_component_notification_v1();
  assert.equal(saved.kind,'attachment-action');assert.equal(saved.action,'open');assert.equal(saved.part,'card');assert.equal(saved.source,'keyboard');assert.equal(saved.eventIndex,0);assert.equal(typeof saved.frame,'bigint');assert.equal(saved.node,card);assert.ok(Object.isFrozen(saved));
  enter(app);assert.equal(app.poll_value_notification_v1().kind,'legacy');
  tab(app);enter(app);assert.equal(app.poll_component_notification_v1().action,'retry');
  tab(app);enter(app);assert.equal(app.poll_component_notification_v1().action,'remove');
  card.attachment_actions();enter(app);assert.equal(app.poll_component_notification_v1(),undefined);
 } finally {app.dispose();}
 assert.equal(saved.action,'open');
});
test('unavailable action capability preflights mount and never drains pending input',{skip:process.env.CUI_TEST_LEGACY_ATTACHMENT_ACTIONS==='1'},()=>{
 const app=new c.App();const original=native.attachmentActions;
 try {
  for(const options of [{open:1},{remove:null},{retry:'yes'},{bogus:true},null]) {
   assert.throws(()=>c.attachment().attachment_actions(options));
  }
  const card=c.attachment('file','Plan.pdf').attachment_actions({open:true});
  app.mount(c.column().children(card)).step_only().step_only();tab(app);enter(app);
  native.attachmentActions=undefined;
  assert.equal(app.supports_attachment_actions(),false);
  assert.throws(()=>app.poll_component_notification_v1(),/unavailable/);
  assert.throws(()=>card.attachment_actions(),/unavailable/);
  const other=c.attachment('other','Other').attachment_actions({open:true});
  const empty=new c.App();try{assert.throws(()=>empty.mount(c.column().children(other)),/unavailable/);assert.equal(empty.root,null);}finally{empty.dispose();}
  assert.equal(app.root.childrenValue[0],card);
  native.attachmentActions=original;
  assert.equal(app.poll_component_notification_v1().action,'open');
 }finally{native.attachmentActions=original;app.dispose();}
});

test('legacy runtime retains ordinary Attachment and old poll', {skip:process.env.CUI_TEST_LEGACY_ATTACHMENT_ACTIONS!=='1'},()=>{
 const app=new c.App();try{
  assert.equal(app.supports_attachment_actions(),false);
  const card=c.attachment('legacy','Legacy').attachment_actions({open:true}).attachment_actions();app.mount(c.column().children(card)).step_only();
  assert.throws(()=>card.attachment_actions({open:true}),/unavailable/);
  assert.throws(()=>app.poll_component_notification_v1(),/unavailable/);
  assert.equal(app.poll_value_notification_v1(),undefined);
 }finally{app.dispose();}
});
