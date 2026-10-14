'use strict';
const assert=require('node:assert/strict'),c=require('../index.cjs');
function press(app,{x,y,width,height}) {assert.ok(width>0&&height>0);app.pointer('down',x+width/2,y+height/2).pointer('up',x+width/2,y+height/2).step();}
for(const name of ['dialog','alert_dialog','sheet','popover']) {
 const app=new c.App(),events=[],trigger=c.button('trigger').label('Open'),body=c.label('Body');
 const node=c[name]('surface','Title').detail('Description').on_close(e=>events.push(e));
 if(name!=='alert_dialog')node.children(body);if(name==='popover')node.anchor(trigger);
 app.mount(c.column().children(trigger,node)).step();node.open();app.step();assert.equal(node.is_open(),true);
 if(name!=='alert_dialog')assert.ok(body.bounds().width>0);
 node.open(false);app.step();assert.equal(events.length,1);assert.equal(events[0].result,3);app.dispose();
}
for(const name of ['tooltip','hover_card']) {
 const app=new c.App(),trigger=c.button('trigger').label('Hover'),body=c.label('Context'),node=c[name]('hover','Help').anchor(trigger);
 if(name==='hover_card')node.children(body);app.mount(c.column().children(trigger,node)).step();
 const {x,y,width,height}=trigger.bounds();app.pointer('move',x+width/2,y+height/2).step(.5);assert.equal(node.is_open(),true);
 if(name==='hover_card')assert.ok(body.bounds().width>0);app.pointer('move',600,400).step(.5);assert.equal(node.is_open(),false);app.dispose();
}
for(const name of ['avatar','bubble','empty_view','group_box','kbd','skeleton','spinner','tag','marker','shimmer','icon','message']) {
 const app=new c.App(),node=c[name]('visual',name==='icon'?'check':'Example');
 if(name==='group_box')node.children(c.label('Body'));
 if(['avatar','empty_view','group_box','skeleton','spinner','icon','message'].includes(name))node.visual_size(80,40);
 if(['empty_view','message'].includes(name))node.detail('Description');app.mount(c.column().children(node)).step();assert.ok(node.bounds().width>0);app.dispose();
}
for(const name of ['list_view','menu','stepper']) {
 const app=new c.App({width:800,height:800}),events=[],a=c.item('a','First'),b=c.item('b','Second'),node=c[name]('nav','Choose').children(a,b).on_change(e=>events.push(e));
 if(name==='stepper'){a.step_status(2);b.detail('Next step');}app.mount(c.column().children(node)).step();
 if(name==='menu')press(app,node.bounds());press(app,name==='stepper'?node.numeric_part_bounds(2):b.bounds());assert.equal(node.selected(),b);assert.ok(events.length>0);app.dispose();
}
for(const [name,part,total,start,expected] of [['pagination',3,20,10,11],['rating',14,5,2,5]]) {
 const app=new c.App(),node=c[name]('numeric').total(total).value(start);app.mount(c.column().children(node)).step();press(app,node.numeric_part_bounds(part));assert.equal(node.current_value(),expected);app.dispose();
}
{
 const app=new c.App(),node=c.scrollable('scroll').visual_size(200,100).scroll_extent(200,1000).children(c.label('Content'));app.mount(c.column().children(node)).step();const {x,y}=node.bounds();app.scroll(x+5,y+5,0,-80).step();assert.ok(node.scroll_offset()>0);app.dispose();
}
for(const name of ['description_list','status_bar','toolbar','title_bar']) {
 const app=new c.App(),entry=c.item('entry','Title');if(name==='description_list')entry.detail('Value');if(name==='status_bar')entry.step_status(1);if(name==='toolbar')entry.children(c.button('save').label('Save'));
 const node=c[name]('chrome','Workspace').children(entry);app.mount(c.column().children(node)).step();assert.ok(node.bounds().width>0);app.dispose();
}
console.log('PASS native-backed surfaces, hover, visuals, navigation, scrolling and chrome');
