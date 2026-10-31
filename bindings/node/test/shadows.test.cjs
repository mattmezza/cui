'use strict';
const assert=require('node:assert/strict');
const {App,BoxShadow,ShadowStyle,ShadowKind,ShadowPreset,CornerRadii,button,column,checkbox}=require('..');
const layer=BoxShadow.css(0,2,8,-1,0x10203040);
assert.equal(layer.values[2],4);
const source=[layer,new BoxShadow(0,0,1,1,ShadowKind.INSET)];
const target=button('target').label('Go').box_shadows(source,new CornerRadii(3,4,5,6));
source.length=0;assert.equal(target.shadowStyleValue.layers.length,2);
const badApp=new App({width:360,height:240});
const bad=button('bad-mount').box_shadows([]);bad.kind=5;
assert.throws(()=>badApp.mount(column().children(bad)));
assert.equal(badApp.disposed,true); // Existing JS rollback disposes the failed App.
assert.throws(()=>bad.box_shadows([]));
const app=new App({width:360,height:240});
try{
 app.mount(column().children(target));app.step();const before=target.bounds();let clicks=0;
 target.on_click(()=>{clicks++;target.box_shadows([]);});
 app.pointer('down',before.x+before.width/2,before.y+before.height/2).pointer('up',before.x+before.width/2,before.y+before.height/2);app.step();
 assert.equal(clicks,1);assert.equal(target.shadowStyleValue.layers.length,0);
 target.shadow_preset(ShadowPreset.MD,new CornerRadii(4,4,4,4));const old=target.shadowStyleValue;
 assert.throws(()=>target.box_shadows(Array(9).fill(layer)));assert.equal(target.shadowStyleValue,old);
 assert.throws(()=>target.shadow_preset(999));assert.equal(target.shadowStyleValue,old);
 app.step();assert.deepEqual(target.bounds(),before);
 target.reset_box_shadows();assert.equal(target.shadowStyleValue,null);app.step();
 assert.throws(()=>checkbox('no').box_shadows([]));
 assert.throws(()=>new BoxShadow(1e300));assert.throws(()=>BoxShadow.css(0,0,513));
 assert.throws(()=>new BoxShadow(0,0,0,0,0,0x100000000));
}finally{app.dispose();}
assert.throws(()=>target.box_shadows([]));
console.log('PASS shadow typed ownership, callback, geometry, clear/reset and validation');
