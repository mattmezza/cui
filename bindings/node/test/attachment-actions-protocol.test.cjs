'use strict';
const assert=require('node:assert/strict'),{App}=require('..');
const app=new App();try{
 if(['valid','index-only','source-only'].includes(process.env.CUI_ATTACHMENT_FIXTURE)) {
  const event=app.poll_component_notification_v1();
  assert.equal(event.action,'retry');assert.equal(event.part,'media-retry');assert.equal(event.source,process.env.CUI_ATTACHMENT_FIXTURE==='index-only'?undefined:'pointer');
  assert.equal(event.eventIndex,process.env.CUI_ATTACHMENT_FIXTURE==='source-only'?undefined:0xffffffff);assert.equal(event.frame,(1n<<64n)-1n);
  assert.deepEqual(event.handle,{slot:0xffffffff,generation:0xfffffffe});
 }else assert.throws(()=>app.poll_component_notification_v1(),/invalid/);
}finally{app.dispose();}
