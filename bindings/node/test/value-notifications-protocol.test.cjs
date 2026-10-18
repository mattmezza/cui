'use strict';
// Synthetic native provider; does not claim these values arose from UI input.
const assert=require('node:assert/strict');const c=require('..');const app=new c.App();
try {
 const mode=process.env.CUI_VALUE_FIXTURE;
 if(['size','unavailable','missing','limit','zero','repeat','tag','kind','utf8','nul','flags'].includes(mode)) {
  assert.throws(()=>app.poll_value_notification_v1(),/unavailable|layout|limit|capacity|invalid/);
 } else {
  const events=[];let e;while((e=app.poll_value_notification_v1())!==undefined)events.push(e);
  assert.deepEqual(events.map(x=>x.frame),[(1n<<53n)-1n,1n<<53n,(1n<<53n)+1n,(1n<<64n)-1n]);
  assert.deepEqual(events.map(x=>x.kind),['scalar-change','text-change','selection-change','selection-change']);
  assert.equal(events[0].value,16777216.5);assert.equal(events[1].text.length,mode==='capacity'?8192:3);
  assert.deepEqual(events[2].selectedHandle,{slot:0xffffffff,generation:0xffffffff});assert.equal(events[2].selectedNode,undefined);
 }
}finally{app.dispose();}
