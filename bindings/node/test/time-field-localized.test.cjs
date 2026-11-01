'use strict';
const assert=require('node:assert/strict');const c=require('..');
const {appointments}=require('../examples/time-field-localized.cjs');
function paste(app,node,text){const b=node.bounds();assert.ok(b.width>0&&b.height>0);app.pointer('down',b.x+b.width/2,b.y+b.height/2).pointer('up',b.x+b.width/2,b.y+b.height/2).text_input(text).step_only();}
function one(){const app=new c.App();const node=c.localized_time_field('time',{precision:'second'}).time_value(9,30,0);app.mount(c.column().children(c.row().children(node)));app.step_only();return {app,node};}
{
 const ui=appointments(),{app,english,arabic}=ui;
 try{
  assert.equal(app.supports_localized_time_field(),true);app.step_only();
  assert.deepEqual(english.current_time(),[9,30,0]);assert.deepEqual(arabic.current_time(),[13,45,0]);
  const initial=english.latest_result();assert.equal(typeof initial.frame,'bigint');assert.equal(initial.node,english);
  paste(app,english,'10:15:30');const copied=app.poll_localized_time_change();
  assert.deepEqual(copied.previous,[9,30,0]);assert.deepEqual(copied.value,[10,15,30]);assert.equal(copied.frameStatus,0);assert.ok(copied.frame>initial.frame);assert.equal(app.poll_localized_time_change(),undefined);
  assert.deepEqual(english.current_time(),copied.value);assert.equal(english.latest_result().queuedChangeCount,1);
  paste(app,arabic,'٠٢:١٠:٣٠\u202fم');const localized=app.poll_localized_time_change();assert.equal(localized.node,arabic);assert.deepEqual(localized.value,[14,10,30]);
  const snapshot=english.get_config();assert.throws(()=>english.configure({...snapshot,minimum:[20,0,0],maximum:[8,0,0]}));assert.deepEqual(english.get_config(),snapshot);
  english.readonly(true);app.step_only();paste(app,english,'11:00:00');assert.deepEqual(english.current_time(),[10,15,30]);assert.equal(app.poll_localized_time_change(),undefined);
  english.readonly(false).disabled(true);app.step_only();paste(app,english,'11:00:00');assert.deepEqual(english.current_time(),[10,15,30]);assert.equal(app.poll_localized_time_change(),undefined);
  english.disabled(false);app.step_only();paste(app,english,'23:00:00');assert.deepEqual(english.current_time(),[10,15,30]);assert.equal(app.poll_localized_time_change(),undefined);assert.equal(english.latest_result().validation,5);
  english.owner_invalid(true).clear_validation();app.step_only();assert.equal(english.get_config().ownerInvalid,true);assert.deepEqual(english.current_time(),[10,15,30]);english.owner_invalid(false);
  assert.throws(()=>english.on_change(()=>{}),/on_localized_change/);
  let count=0;english.on_localized_change(e=>{count++;assert.deepEqual(english.current_time(),e.value);for(const f of [()=>app.step(),()=>app.step_only(),()=>app.run(),()=>app._dispatch(),()=>app.dispatch_localized_time_changes(),()=>app.poll_localized_time_change()])assert.throws(f,/localized TimeField/);return [0,0,0];});
  paste(app,english,'12:00:00');assert.equal(app.dispatch_localized_time_changes(),1);assert.equal(count,1);assert.deepEqual(english.current_time(),[12,0,0]);assert.equal(app.dispatch_localized_time_changes(),0);
  assert.deepEqual(copied.value,[10,15,30]);assert.ok(Object.isFrozen(copied.handle));
 }finally{app.dispose();}
}
// Copied configurations are distinct from JS native buffers and C ownership.
{
 const {app,node}=one();try{
  const native=require(process.env.CUI_NODE_ADDON),symbol=Object.getOwnPropertySymbols(node).find(s=>s.description==='CUI node');
  const raw=native.timeFieldLocalized(app.handle,3,node[symbol],null,null),saved=node.get_config();
  raw.fill(0);assert.deepEqual(node.get_config(),saved);
  const good=native.timeFieldLocalized(app.handle,3,node[symbol],null,null);
  const invalid=Buffer.from(good);invalid.writeUInt32LE(1,24);assert.throws(()=>native.timeFieldLocalized(app.handle,2,node[symbol],invalid,null));assert.deepEqual(node.get_config(),saved);
  const unaligned=Buffer.alloc(617);good.copy(unaligned,1);native.timeFieldLocalized(app.handle,2,node[symbol],unaligned.subarray(1),null);unaligned.fill(0);assert.deepEqual(node.get_config(),saved);
 }finally{app.dispose();}
}
// Pop before callback; errors/reentry never replay the consumed applied record.
{
 const {app,node}=one();try{let called=0;node.on_localized_change(()=>{called++;throw Error('localized callback sentinel');});paste(app,node,'10:00:00');assert.throws(()=>app.dispatch_localized_time_changes(),/sentinel/);assert.equal(app.localizedTimeDispatching,false);assert.equal(app.dispatch_localized_time_changes(),0);assert.equal(called,1);assert.deepEqual(node.current_time(),[10,0,0]);}finally{app.dispose();}
}
for(const teardown of ['close','dispose','remove']){
 const {app,node}=one();const records=[];node.on_localized_change(e=>{records.push(e);if(teardown==='remove')node.remove();else app[teardown]();});paste(app,node,'10:00:00');assert.equal(app.dispatch_localized_time_changes(),1);assert.equal(records.length,1);assert.equal(node.localizedChangeValue,null);assert.equal(app.localizedTimeDispatching,false);assert.deepEqual(records[0].value,[10,0,0]);if(teardown==='remove'){assert.equal(app.poll_localized_time_change(),undefined);assert.equal(node.owner,null);}app.dispose();assert.deepEqual(records[0].value,[10,0,0]);
}
// Real later-node capacity failure after TimeField applies an edit. Drain explicitly
// even though step throws; latest result remains unavailable until a healthy frame.
{
 const app=new c.App(),node=c.localized_time_field('time',{precision:'second'}).time_value(9,0,0);
 const later=c.slider_model('later',{reducedMotion:true},c.slider_single(40));
 try{
  app.enable_slider_requests(1).enable_localized_time_fields(8);app.mount(c.column().children(node,later));app.step_only();app.step_only();
  // Full queued paste avoids assuming an anonymous segment's geometry.
  assert.throws(()=>paste(app,node,'10:00:00'),/capacity/i);
  const e=app.poll_localized_time_change();assert.ok(e);assert.equal(e.frameStatus,5);assert.deepEqual(e.value,[10,0,0]);assert.throws(()=>node.latest_result(),/core/i);
  assert.equal(app.poll_localized_time_change(),undefined);later.remove();app.step_only();assert.deepEqual(node.latest_result().value,e.value);
 }finally{app.dispose();}
}
// Mount selected the default queue capacity; enabling a different capacity is atomic.
{
 const {app,node}=one();try{assert.throws(()=>app.enable_localized_time_fields(1));assert.deepEqual(node.current_time(),[9,30,0]);}finally{app.dispose();}
}
console.log('PASS actual localized kind55 UI, English/Arabic, applied copies, failure/lifetimes');
