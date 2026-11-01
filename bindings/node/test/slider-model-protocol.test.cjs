'use strict';
// Conversion-only provider. Values below are synthetic, not UI evidence.
const assert=require('node:assert/strict');
const install=require('../slider-model.cjs');
const NODE=Symbol();class Node {constructor(kind){this.kind=kind;this.childrenValue=[];}key(k){this.keyValue=k;}}
const handles=x=>`${x.slot}:${x.generation}`;
const edge=(1n<<64n)-1n;
function raw() {return [1,0,1,1,3,7,[edge,0xffffffff,0xffffffff,edge,edge,edge,edge],edge,edge,edge,[1,20,60],[1,20,59],[0,100,1,0],[0,100,1,0],[3,1,5,15,0,0,1,0,0,0,edge,edge,0n,0]];}
let source=raw();let configOutput=[1,0,0n,0,100,1,0,16,200,0];let operations=[];
const native={sliderModel(handle,op,node,a,b){operations.push(op);if(op===0)return true;if(op===7)return source;if(op===4)return configOutput;if(op===8){assert.equal(a[0],edge);assert.equal(b[0],1);}return undefined;}};
// Observe receipt authority writes in this isolated conversion-only process.
const RealWeakMap=global.WeakMap;const mapWrites=[];
class ObservedWeakMap extends RealWeakMap {set(key,value){mapWrites.push([this,key]);return super.set(key,value);}}
let moduleApi;
try {global.WeakMap=ObservedWeakMap;moduleApi=install(native,Node,NODE,handles);}
finally {global.WeakMap=RealWeakMap;}
const app={handle:{},nodes:new Map(),_alive(){}};
assert.throws(()=>moduleApi.poll(app),/successful queue enable/);
moduleApi.enable(app);const copied=moduleApi.poll(app);source[6][0]=0n;source[10][1]=0;
assert.equal(copied.receipt.appEpoch,edge);assert.equal(copied.current.start,20);assert.equal(copied.input.pointerId,edge);assert.ok(Object.isFrozen(copied.receipt.node));
moduleApi.accept(app,copied.receipt,moduleApi.exports.slider_range(25,55));
assert.throws(()=>moduleApi.accept(app,{...copied.receipt},copied.current),/reconstructed/);
for(const mutate of [a=>a[0]=99,a=>a[2]=2,a=>a[5]=8,a=>a[6][0]=1,a=>a[10][0]=2,a=>a[11][1]=NaN,a=>a[12][2]=-1,a=>a[14][3]=16,a=>a[14][10]=-1n,a=>a[14][1]=2,a=>a[14][13]=.25,a=>{a[14][0]=13;a[14][9]=2;a[14][13]=.25;},a=>{a[5]=0;a[8]=1n;},a=>{a[5]=0;a[8]=0n;a[9]=0n;}]) {
 source=raw();mutate(source);const writes=mapWrites.length;assert.throws(()=>moduleApi.poll(app));assert.equal(mapWrites.length,writes,"malformed request installed receipt authority");
}
source=raw();source[4]=4;source[14]=[13,1,0,0,0,0,0,0,0,2,0n,0n,edge,0];
assert.equal(moduleApi.poll(app).input.accessibilityValue,0); // INCREMENT, inactive numeric.
source[14][1]=3;source[14][9]=4;source[14][13]=.25;
assert.equal(moduleApi.poll(app).input.accessibilityValue,.25); // Explicit SET evidence.
assert.equal(copied.current.start,20); // Previous owned record remains unchanged.
assert.throws(()=>moduleApi.exports.slider_config({requests:null}));assert.throws(()=>moduleApi.exports.slider_config({sourceGeneration:null}));
const missing=install({},Node,NODE,handles);assert.equal(missing.supports(app),false);assert.throws(()=>missing.enable(app),/unavailable/);
source=raw();const other={handle:{},nodes:new Map(),_alive(){}};moduleApi.enable(other);assert.throws(()=>moduleApi.accept(other,copied.receipt,copied.current),/foreign/);
const node=moduleApi.exports.slider_model('synthetic',{},moduleApi.exports.slider_single(40));node.owner=app;node[NODE]={};
assert.equal(node.get_config().length,200);configOutput=[1,0,0n,0,100,1,0,16,200,1];assert.throws(()=>node.get_config(),/invalid Slider config copy/);
configOutput[8]=0;assert.equal(node.get_config().length,0);
console.log('PASS synthetic typed copy protocol, exact BigInt, immutable receipts and malformed rejection');
