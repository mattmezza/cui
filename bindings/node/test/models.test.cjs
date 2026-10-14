'use strict';
const assert=require('node:assert/strict'),c=require('../index.cjs');
function press(app,{x,y,width,height}) {assert.ok(width>0&&height>0);app.pointer('down',x+width/2,y+height/2).pointer('up',x+width/2,y+height/2).step();}
{
 const app=new c.App(),pixels=new Uint8Array([255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255]);
 const image=c.image('image').image_rgba(pixels,2,2).visual_size(80,80);pixels.fill(0);
 const attachment=c.attachment('file','plan.pdf').detail('24 KB').attachment_status('uploading').value(.4).visual_size(250,60);
 app.mount(c.column().children(image,attachment)).step();assert.ok(image.bounds().width>0);assert.equal(attachment.current_value(),.4);
 assert.throws(()=>image.image_rgba(new Uint8Array(3),2,2));image.image_rgba(new Uint8Array(16).fill(255),2,2);app.step();app.dispose();
}
{
 const app=new c.App(),events=[],parent=c.item('parent','Project').step_status(0).checked(true),child=c.item('child','Notes').step_status(1);
 const tree=c.tree('tree').visual_size(240,180).children(parent,child).on_change(e=>events.push(e));
 app.mount(c.column().children(tree)).step();press(app,child.bounds());assert.equal(tree.selected(),child);assert.ok(events.at(-1).selectedHandle.generation>0);
 events.length=0;child.disabled();app.step();press(app,child.bounds());assert.equal(events.length,0);app.dispose();
}
{
 const app=new c.App({height:700}),events=[],time=c.time_field('time').time_value(9,45,30).on_change(e=>events.push(e)),color=c.color_picker('color').color_rgba(0xff0000ff).visual_size(240,0).on_change(e=>events.push(e));
 app.mount(c.column().children(time,color)).step();press(app,time.bounds());app.key('down','up').step();assert.notDeepEqual(time.current_time(),[9,45,30]);assert.equal(events.at(-1).text,time.current_time().map(x=>String(x).padStart(2,'0')).join(':'));
 events.length=0;press(app,color.color_part_bounds(1));assert.notEqual(color.current_color(),0xff0000ff);assert.equal(events.at(-1).value,color.current_color());assert.equal(events.at(-1).text,'#'+color.current_color().toString(16).padStart(8,'0').toUpperCase());
 const before=color.current_color();color.disabled();app.step();press(app,color.color_part_bounds(1));assert.equal(color.current_color(),before);app.dispose();
}
{
 const app=new c.App({width:800,height:650}),events=[],calendar=c.calendar('calendar').date_value(2024,3,5).on_change(e=>events.push(e)),picker=c.date_picker('picker','Choose').date_value(1980,1,1);
 assert.throws(()=>picker.date_value(2024,2,30));app.mount(c.column().children(calendar,picker)).step();press(app,calendar.calendar_day_bounds(2024,3,10));assert.deepEqual(calendar.current_date(),[2024,3,10]);assert.equal(events.at(-1).value,20240310);assert.equal(events.at(-1).text,'2024-03-10');
 picker.open();app.step();assert.equal(picker.is_open(),true);picker.open(false);assert.equal(picker.is_open(),false);app.dispose();
}
{
 const app=new c.App(),events=[],otp=c.otp_input('otp','12').total(4).on_change(e=>events.push(e));
 app.mount(c.column().children(otp)).step();assert.equal(otp.current_text(),'12');press(app,otp.bounds());app.text_input('3').step();assert.equal(otp.current_text(),'123');assert.equal(events.at(-1).text,'123');
 events.length=0;otp.disabled();app.step();app.text_input('4').step();assert.equal(events.length,0);assert.equal(otp.current_text(),'123');app.dispose();
}
console.log('PASS owned image data, tree selection, time/color/date snapshots and OTP edits');

assert.throws(()=>c.description_list('static').select(c.item('outside')));
assert.throws(()=>c.status_bar('static').on_change(()=>{}));
assert.throws(()=>c.image('bad').image_rgba(new Uint8Array(4),true,1));
{
 const app=new c.App();assert.throws(()=>app.mount(c.column().children(c.tabs('tabs').children(c.item('bad').open()))));
 assert.equal(app.disposed,true); // Failed JS mounts deliberately dispose the App.
}

{
 const app=new c.App({width:700,height:500}),events=[],save=c.button('save').label('Save').on_click(e=>events.push(e));
 const form=c.form('form','Profile').children(c.item('field','Name').detail('Public name').children(c.input('name','Alex')),c.item('action','Action').children(save));
 app.mount(c.column().children(form)).step();press(app,save.bounds());assert.equal(events.length,1);form.disabled();app.step();events.length=0;press(app,save.bounds());assert.equal(events.length,0);app.dispose();
}
{
 const app=new c.App(),actions=[],changes=[],addon=c.item('go','Go').step_status(1).on_click(e=>actions.push(e)),group=c.input_group('search').detail('Search').visual_size(320,0).children(addon).on_change(e=>changes.push(e));
 app.mount(c.column().children(group)).step();press(app,addon.bounds());assert.equal(actions.length,1);press(app,group.bounds());app.text_input('a').step();assert.equal(group.current_text(),'a');assert.equal(changes.at(-1).text,'a');
 group.disabled();app.step();changes.length=0;app.text_input('b').step();assert.equal(changes.length,0);app.dispose();
}
{
 const app=new c.App({width:800,height:650}),clicked=[],closed=[],first=c.notification('first','Saved').detail('File saved').open().on_click(e=>clicked.push(e)).on_close(e=>closed.push(e)),second=c.notification('second','Synced').open().on_close(e=>closed.push(e));
 app.mount(c.column().children(first,second)).step(.4);assert.notEqual(first.bounds().y,second.bounds().y);press(app,first.bounds());assert.equal(clicked.length,1);assert.equal(first.is_open(),false);assert.equal(second.is_open(),true);
 app.step(.3);assert.equal(closed.length,1);second.open(false);app.step(.3);assert.equal(closed.length,2);app.dispose();
}

{
 const app=new c.App(),events=[],outside=c.button('outside').label('Outside').on_click(e=>events.push(e)),inside=c.button('inside').label('Inside').on_click(e=>events.push(e)),trap=c.focus_trap('trap').children(inside).open();
 app.mount(c.column().children(outside,trap)).step();press(app,outside.bounds());assert.equal(events.length,0);press(app,inside.bounds());assert.equal(events.at(-1).node,inside);trap.open(false);app.step();press(app,outside.bounds());assert.equal(events.at(-1).node,outside);app.dispose();
}

{
 const app=new c.App();assert.throws(()=>app.font_fallbacks(['']));assert.throws(()=>app.font_fallbacks(Array(33).fill('font.ttf')));assert.throws(()=>app.font_fallbacks([]),/backend unavailable/);app.dispose();
}
