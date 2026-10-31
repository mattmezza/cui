'use strict';
const assert=require('node:assert/strict');
const {App,ShadowPart:P,BoxShadow,ShadowPreset,checkbox,radio_item,radio_group,switch_control,slider,column,item,tabs,button}=require('..');
const layers=[BoxShadow.css(0,0,8)];
const check=checkbox('check','Check').part_shadows(P.MAIN,layers);layers.length=0;
const sw=switch_control('sw','Switch').part_shadows(P.TRACK,[new BoxShadow(0,0,1)]).part_shadows(P.THUMB,[]);
const slide=slider('slide').part_shadows(P.MAIN,[]);
const a=radio_item('a','A').part_shadows(P.MAIN,[]),b=radio_item('b','B').part_shadows(P.ROOT,[new BoxShadow()]);
const app=new App();
try{
 app.mount(column().children(check,sw,slide,radio_group('g').children(a,b)));app.step();
 assert.equal(check.shadow_target(P.MAIN).canonicalPart,P.INDICATOR);
 assert.equal(slide.shadow_target(P.THUMB).supported,false);
 assert.equal(check.partShadowStyles.get(P.INDICATOR).layers.length,1);
 const bounds=check.bounds();let calls=0;
 check.on_change(()=>{calls++;check.part_shadows(P.INDICATOR,[]);});
 app.pointer('down',bounds.x+bounds.width/2,bounds.y+bounds.height/2).pointer('up',bounds.x+bounds.width/2,bounds.y+bounds.height/2);app.step();assert.equal(calls,1);
 check.part_shadow_preset(P.ROOT,ShadowPreset.SM).part_shadows(P.INDICATOR,[]);app.step();
 assert.deepEqual(check.bounds(),bounds);const old=check.partShadowStyles.get(P.ROOT);
 assert.throws(()=>check.part_shadows(P.THUMB,[]));assert.equal(check.partShadowStyles.get(P.ROOT),old);
 check.reset_part_shadows(P.MAIN);a.reset_part_shadows(P.MAIN);app.step();
 assert.equal(check.partShadowStyles.has(P.INDICATOR),false);
 const plain=button('plain');assert.throws(()=>plain.shadow_target(P.ROOT));
}finally{app.dispose();}
assert.throws(()=>check.part_shadows(P.MAIN,[]));
const badApp=new App(),wrong=item('wrong').part_shadows(P.INDICATOR,[]);
assert.throws(()=>badApp.mount(column().children(tabs('tabs').children(wrong))));
assert.equal(badApp.disposed,true);
console.log('PASS semantic parts, owned lists, aliases, capability, geometry and mount rollback');
