'use strict';
// Synthetic owned-wire tests, deliberately separate from real C input proof.
const assert=require('node:assert/strict');
const install=require('../time-field-localized.cjs');
const NODE=Symbol();class Node {constructor(kind){this.kind=kind;this.childrenValue=[];this.timeValue=[0,0,0];}key(k){this.keyValue=k;}}
let configWire,changeWire,resultWire,configureFailure=false;const operations=[];
const native={timeFieldLocalized(handle,op,node,value){operations.push(op);if(op===0)return true;if(op===2){if(configureFailure)throw Error('rejected');configWire=Buffer.from(value);}if(op===3)return configWire;if(op===4)return resultWire;if(op===6){const copy=changeWire;changeWire=undefined;return copy;}}};
const api=install(native,Node,NODE,x=>`${x.slot}:${x.generation}`),c=api.exports;
const app={handle:{},nodes:new Map(),_alive(){if(this.disposed)throw Error('disposed');}};
assert.equal(c.localized_time_config().editGeneration,1n);
for(const v of [{disabled:null},{readonly:1},{editGeneration:1},{editGeneration:0n},{editGeneration:1n<<64n},{cycle:'guess'},{names:'Hour'},{minimum:[1.1,0,0]},{minimum:[9,0,1]},{minimum:[19,0,0],maximum:[18,0,0]},{names:['\ud800','','','']}])assert.throws(()=>c.localized_time_config(v));
const sourceLocale={...c.time_locale_arabic(),digits:[...c.time_locale_arabic().digits]};
const config=c.localized_time_config({locale:sourceLocale,cycle:'h12',precision:'second',editGeneration:0xffffffffffffffffn,names:['ساعة','دقيقة','ثانية','فترة']});
sourceLocale.digits[0]='x';assert.equal(config.locale.digits[0],'٠');assert.ok(Object.isFrozen(config.locale.digits));
assert.throws(()=>c.localized_time_config({locale:{...c.time_locale_english(),am:'A',pm:'AM'}}));
const node=c.localized_time_field('time',config);node[NODE]={};node.owner=app;
api.enable(app,2);node.configure(config);assert.equal(configWire.length,616);assert.equal(configWire.readBigUInt64LE(56),0xffffffffffffffffn);
assert.equal(configWire.readUInt32LE(24),0);assert.equal(configWire.readUInt32LE(28),0);
assert.deepEqual(node.get_config(),config);const saved=node.get_config();
configureFailure=true;assert.throws(()=>node.configure({...config,readonly:true}),/rejected/);configureFailure=false;assert.equal(node.localizedConfigValue.readonly,false);assert.deepEqual(saved,config);
for(const mutate of [b=>b.writeUInt32LE(1,24),b=>b.writeUInt32LE(17,64),b=>b[68]=0,b=>b[83]=1,b=>b.writeUInt32LE(2,12),b=>b.writeBigUInt64LE(0n,56),b=>b.writeUInt32LE(1,32)]){
 const old=configWire;configWire=Buffer.from(old);mutate(configWire);assert.throws(()=>node.get_config());configWire=old;
}
app.nodes.set('7:9',node);
function change(){const b=Buffer.alloc(64);b.writeUInt32LE(64);b.writeUInt32LE(1,4);b.writeUInt32LE(10,8);b.writeUInt32LE(7,16);b.writeUInt32LE(9,20);b.writeBigUInt64LE(0xffffffffffffffffn,24);b.writeUInt32LE(9,32);b.writeUInt32LE(10,44);return b;}
changeWire=change();const raw=changeWire,event=api.poll(app);raw.fill(0);assert.deepEqual(event.previous,[9,0,0]);assert.deepEqual(event.value,[10,0,0]);assert.equal(event.frame,0xffffffffffffffffn);assert.equal(event.frameStatus,10);assert.equal(event.node,node);assert.ok(Object.isFrozen(event.value));assert.equal(api.poll(app),undefined);
for(const mutate of [b=>b.writeUInt32LE(1,12),b=>b.writeUInt32LE(24,44),b=>b.writeUInt32LE(7,56),b=>b.writeUInt32LE(11,8),b=>b.writeUInt32LE(0,20),b=>b.writeBigUInt64LE(0n,24),b=>b.writeUInt32LE(9,44)]){changeWire=change();mutate(changeWire);assert.throws(()=>api.poll(app));}
function result(){const b=Buffer.alloc(80);b.writeUInt32LE(80);b.writeUInt32LE(1,4);b.writeUInt32LE(7,16);b.writeUInt32LE(9,20);b.writeBigUInt64LE(0xffffffffffffffffn,24);b.writeUInt32LE(1,40);b.writeUInt32LE(9,48);b.writeUInt32LE(10,60);b.writeUInt32LE(1,72);return b;}
resultWire=result();const resultCopy=node.latest_result();resultWire.fill(0);assert.equal(resultCopy.changed,true);assert.deepEqual(resultCopy.value,[10,0,0]);
for(const mutate of [b=>b.writeUInt32LE(1,8),b=>b.writeUInt32LE(10,76),b=>b.writeUInt32LE(2,40),b=>b.writeUInt32LE(2,72),b=>b.writeUInt32LE(8,16),b=>b.writeUInt32LE(0,40)]){resultWire=result();mutate(resultWire);assert.throws(()=>node.latest_result());}
// Exact observation vocabulary and coherence, independent of the C parser.
for(const validation of [1,2,6]){
 changeWire=change();changeWire.writeUInt32LE(validation,56);
 const before=[...node.timeValue];assert.throws(()=>api.poll(app));assert.deepEqual(node.timeValue,before);
 resultWire=result();resultWire.writeUInt32LE(validation,36);assert.throws(()=>node.latest_result());
}
for(const mutate of [b=>b.writeUInt32LE(1,8),b=>b.writeUInt32LE(0,16)]){
 changeWire=change();mutate(changeWire);const before=[...node.timeValue];assert.throws(()=>api.poll(app));assert.deepEqual(node.timeValue,before);
}
resultWire=result();resultWire.writeUInt32LE(0,16);assert.throws(()=>node.latest_result());
resultWire=result();resultWire.writeUInt32LE(7,32);assert.throws(()=>node.latest_result());
// Valid unchanged observations are results, not applied changes. A queue count
// cannot claim an applied change when changed=false; changed=true may queue0.
function unchanged(){const b=result();b.writeUInt32LE(0,40);b.writeUInt32LE(9,60);b.writeUInt32LE(0,72);return b;}
for(const status of [0,1,2,3,4,5,6])for(const validation of [0,3,4,5]){
 resultWire=unchanged();resultWire.writeUInt32LE(status,32);resultWire.writeUInt32LE(validation,36);
 const valid=node.latest_result();assert.equal(valid.status,status);assert.equal(valid.validation,validation);assert.equal(valid.changed,false);assert.equal(valid.queuedChangeCount,0);assert.deepEqual(valid.previous,valid.value);
}
resultWire=unchanged();resultWire.writeUInt32LE(1,72);assert.throws(()=>node.latest_result());
resultWire=result();resultWire.writeUInt32LE(0,72);assert.equal(node.latest_result().changed,true);assert.equal(node.latest_result().queuedChangeCount,0);
for(const validation of [0,3,4,5])for(const frameStatus of [0,2,3,4,5,6,7,8,9,10]){
 changeWire=change();changeWire.writeUInt32LE(validation,56);changeWire.writeUInt32LE(frameStatus,8);
 const valid=api.poll(app);assert.equal(valid.validation,validation);assert.equal(valid.frameStatus,frameStatus);
}
let calls=0;node.on_localized_change(e=>{calls++;assert.equal(e.frameStatus,10);assert.throws(()=>api.dispatch(app),/nested/);throw Error('callback failed');});
changeWire=change();assert.throws(()=>api.dispatch(app),/callback failed/);assert.equal(calls,1);assert.equal(app.localizedTimeDispatching,false);assert.equal(api.dispatch(app),0);assert.equal(calls,1);
node.on_localized_change(()=>{app.closed=true;});changeWire=change();assert.equal(api.dispatch(app),1);assert.equal(api.dispatch(app),0);app.closed=false;
node.on_localized_change(()=>{});api.release(node);assert.equal(node.localizedChangeValue,null);
const absent=install({},Node,NODE,x=>`${x.slot}:${x.generation}`);assert.equal(absent.supports(app),false);assert.throws(()=>absent.enable(app),/unavailable/);
app.disposed=true;assert.throws(()=>api.poll(app),/disposed/);
console.log('PASS copied localized TimeField protocol; no UI evidence');
