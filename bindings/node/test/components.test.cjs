'use strict';
const assert=require('node:assert/strict');
const c=require('../index.cjs');
function press(app,node) { const {x,y,width,height}=node.bounds();app.pointer('down',x+width/2,y+height/2).pointer('up',x+width/2,y+height/2).step(); }
for(const name of ['checkbox','radio','switch_control','toggle']) {
  const app=new c.App({width:800,height:800}), events=[];
  const first=c[name]('first','First').on_change(e=>events.push(e));
  const second=c[name]('second','Disabled').disabled();
  app.mount(c.column().children(first,second)).step();press(app,first);
  assert.equal(first.is_checked(),true);assert.equal(events[0].checked,true);
  press(app,second);assert.equal(second.is_checked(),false);assert.equal(events.length,1);app.dispose();
}
{
  const app=new c.App(),s=c.slider('slider').range(10,100,5).value(30),n=c.number_input('num').range(-100,100,1).value(-20),p=c.progress('progress').value(.5),b=c.badge('badge','Ready');
  app.mount(c.column().children(s,n,p,b)).step();assert.equal(s.current_value(),30);assert.equal(n.current_value(),-20);assert.equal(p.current_value(),.5);assert.ok(b.bounds().width>0);assert.throws(()=>s.value(NaN));app.dispose();
}
for(const name of ['input','textarea']) {
  const app=new c.App(),events=[],f=c[name]('field').on_change(e=>events.push(e.text));
  app.mount(c.column().children(f)).step();press(app,f);app.text_input('é').step();app.text_input('✓').step();
  assert.equal(f.current_text(),'é✓');assert.deepEqual(events,['é','é✓']);assert.throws(()=>app.text_input('bad\0text'));app.dispose();assert.throws(()=>f.current_text());
}
console.log('PASS real component state, disabled, ranges and UTF-8 callbacks');

{
 const app=new c.App({width:800,height:800}),events=[];
 const a=c.item('a','First'),b=c.item('b','Second');
 const group=c.radio_group('group').children(a,b).select(a).on_change(e=>events.push(e));
 const content=c.label('Expanded'),fold=c.item('fold','Details').children(content);
 app.mount(c.column().children(group,c.accordion('accordion').children(fold))).step();
 assert.equal(group.selected(),a);press(app,b);assert.equal(group.selected(),b);assert.ok(events[0].selectedHandle.generation>0);
 assert.throws(()=>content.bounds());press(app,fold);assert.ok(content.bounds().width>0);assert.throws(()=>group.select(c.item('foreign')));app.dispose();
}
for(const name of ['collapsible','tabs']) {
 const app=new c.App(),content=c.label('Panel');
 const node=name==='tabs'?c.tabs('tabs').children(c.item('tab','Tab').children(content)):c.collapsible('fold','Details').children(content).open();
 if(name==='tabs')node.select(node.childrenValue[0]);
 app.mount(c.column().children(node,c.alert('alert','Notice').detail('More detail'))).step();assert.ok(content.bounds().width>0);app.dispose();
}

{
 const app=new c.App(),events=[],a=c.item('apple','Apple'),b=c.item('banana','Banana');
 const field=c.select('fruit','Fruit').detail('Choose').children(a,b).on_change(e=>events.push(e));
 const combo=c.combobox('search','Ch').detail('Search').children(c.item('cherry','Cherry'));
 app.mount(c.column().children(field,combo)).step();press(app,field);press(app,b);assert.equal(field.selected(),b);assert.ok(events[0].selectedHandle.generation>0);
 press(app,combo);app.text_input('e').step();assert.equal(combo.current_text(),'Che');app.dispose();
}

{
 const app=new c.App(),events=[],option=c.item('csv','Export CSV');
 const node=c.dropdown_button('export','Export').children(option).on_click(e=>events.push(e)).on_change(e=>events.push(e));
 app.mount(c.column().children(node)).step();
 for(const part of ['action','menu']) {const {x,y,width,height}=node.part_bounds(part);app.pointer('down',x+width/2,y+height/2).pointer('up',x+width/2,y+height/2).step();}
 assert.equal(events.length,1);press(app,option);assert.equal(events.length,2);assert.ok(events[1].selectedHandle.generation>0);app.dispose();
}
