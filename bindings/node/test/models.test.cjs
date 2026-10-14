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

{
 const app=new c.App(),messages=c.message_scroller('messages').visual_size(280,100).children(...Array.from({length:20},(_,i)=>c.item('m'+i,'Message '+i).detail('Alex').step_status(i%2)));
 app.mount(c.column().children(messages));assert.throws(()=>messages.scroll_to(0));app.step();assert.ok(messages.scroll_offset()>0);messages.scroll_to(0);app.step();assert.equal(messages.scroll_offset(),0);assert.throws(()=>messages.scroll_to(-1));app.dispose();
}
{
 const app=new c.App(),events=[],slides=Array.from({length:3},(_,i)=>c.item('s'+i,'Slide '+i)),carousel=c.carousel('slides').visual_size(240,100).children(...slides).on_change(e=>events.push(e));
 app.mount(c.column().children(carousel)).step();assert.equal(carousel.selected(),slides[0]);const {x,y}=carousel.bounds();app.scroll(x+30,y+30,-1,0).step();assert.equal(carousel.selected(),slides[1]);assert.ok(events.at(-1).selectedHandle.generation>0);
 carousel.select(slides[2]).disabled();app.step();events.length=0;app.scroll(x+30,y+30,1,0).step();assert.equal(carousel.selected(),slides[2]);assert.equal(events.length,0);app.dispose();
}
{
 const app=new c.App(),events=[],first=c.item('first','First').panel_size(240).on_change(e=>events.push(e)),second=c.item('second','Second'),split=c.resizable('split').visual_size(420,160).children(first,second);
 app.mount(c.column().children(split)).step();const before=first.current_panel_size(),r=split.resizable_handle_bounds(first),x=r.x+r.width/2,y=r.y+r.height/2;
 for(const [phase,px] of [['down',x],['move',x+30],['up',x+30]])app.pointer(phase,px,y).step();
 const after=first.current_panel_size();assert.ok(after>before);assert.ok(events.at(-1).value>before);assert.throws(()=>split.resizable_handle_bounds(second));
 split.disabled();app.step();const r2=split.resizable_handle_bounds(first),x2=r2.x+r2.width/2,y2=r2.y+r2.height/2;events.length=0;
 for(const [phase,px] of [['down',x2],['move',x2-30],['up',x2-30]])app.pointer(phase,px,y2).step();
 assert.equal(first.current_panel_size(),after);assert.equal(events.length,0);app.dispose();
}
{
 const app=new c.App(),events=[],view=c.text_view('view','**Hello** world').visual_size(320,120).on_change(e=>events.push(e));
 app.mount(c.column().children(view)).step();view.select_text(0,5);assert.equal(view.selection_text(),'Hello');assert.equal(view.current_text(),'**Hello** world');
 view.text_format('html').label('<b>HTML</b> text');app.step();view.select_text(0,4);assert.equal(view.selection_text(),'HTML');
 view.text_format('plain').label('é text');app.step();assert.throws(()=>view.select_text(0,1));view.select_text(0,2);assert.equal(view.selection_text(),'é');
 view.label('Plain text');app.step();view.select_text(0,0);const {x,y}=view.bounds();app.pointer('down',x+10,y+14).pointer('move',x+60,y+14).pointer('up',x+60,y+14).step();assert.ok(events.at(-1).text);app.dispose();
}
{
 const app=new c.App(),events=[],editor=c.editor('editor','first\nsecond').detail('c').visual_size(300,120).on_change(e=>events.push(e)),other=c.editor('other').visual_size(300,120);
 app.mount(c.column().children(editor,other)).step();press(app,editor.bounds());app.text_input('✓').step();assert.ok(editor.current_text().includes('✓'));assert.equal(events.at(-1).text,editor.current_text());assert.equal(other.current_text(),'');
 editor.disabled();app.step();events.length=0;app.text_input('X').step();assert.equal(events.length,0);app.dispose();
}
console.log('PASS message scrolling, carousel selection, native divider drag, text selection and independent editors');

{
 const app=new c.App({height:700}),data=[['Jan',5],['Feb',8]],points=[[20,30],[60,50]],chart=c.chart('chart').chart_data(data).chart_kind('bar').visual_size(240,120),plot=c.plot('plot').plot_points(points).visual_size(240,120);
 const table=c.table('table').visual_size(300,0).children(c.item('header').children(c.item('name','Name'),c.item('score','Score')),c.item('row').children(c.item('alex','Alex'),c.item('number','95')));
 data[0][0]='Changed';points[0][0]=999;app.mount(c.column().children(chart,plot,table)).step();assert.deepEqual(chart.current_datum(0),['Jan',5]);assert.deepEqual(plot.current_point(0),[20,30]);assert.ok(table.bounds().width>0);
 assert.throws(()=>chart.chart_data([['bad',NaN]]));assert.throws(()=>plot.plot_points([[Infinity,0]]));assert.deepEqual(chart.current_datum(1),['Feb',8]);assert.deepEqual(plot.current_point(1),[60,50]);
 chart.chart_data([['negative',-2]]);assert.throws(()=>chart.chart_kind('pie'));assert.deepEqual(chart.current_datum(0),['negative',-2]);chart.chart_data([['valid',2]]).chart_kind('pie');app.step();app.dispose();
}
{
 const app=new c.App(),events=[],rows=[['Alex','95','A'],['Bob','87','B']],table=c.data_table('results').table_data(['Name','Score','Team'],rows).selection_mode('cell').visual_size(400,180).on_change(e=>events.push(e));rows[0][0]='Changed';
 app.mount(c.column().children(table)).step();assert.equal(table.current_cell(0,0),'Alex');assert.throws(()=>table.table_data(['Name'],[['extra','column']]));assert.equal(table.current_cell(1,0),'Bob');
 press(app,table.table_cell_bounds(1,2));press(app,table.table_header_bounds(2));const snapshots=events.map(e=>JSON.parse(e.text));assert.equal(snapshots[0].reason,'selection');assert.equal(snapshots[0].row,2);assert.equal(snapshots[0].column,3);assert.equal(snapshots[0].sortColumn,0);assert.equal(snapshots[1].reason,'sort');assert.equal(snapshots[1].sortColumn,3);assert.ok(snapshots[1].sortDirection>0);assert.equal(table.table_state().sortColumn,3);
 table.table_data(['Name','Score','Team'],[['Bob','87','B'],['Alex','95','A']]);app.step();assert.equal(table.current_cell(0,0),'Bob');table.disabled();app.step();events.length=0;press(app,table.table_header_bounds(0));assert.equal(events.length,0);app.dispose();
}
console.log('PASS copied chart/plot models, table composition and exact DataTable selection/sort snapshots');

{
 const app=new c.App(),events=[],open=c.item('open','Open'),close=c.item('close','Close').detail('exit').on_click(e=>events.push(e)),command=c.command('actions').visual_size(300,150).children(open,close);
 app.mount(c.column().children(command)).step();assert.equal(command.selected(),open);press(app,command.item_bounds(close));assert.equal(command.selected(),close);assert.equal(events.at(-1).node,close);
 command.label('exit');app.step();assert.equal(command.current_text(),'exit');assert.equal(command.selected(),close);close.disabled();app.step();assert.equal(command.selected(),undefined);assert.throws(()=>command.select(close));app.dispose();
}
{
 const app=new c.App(),events=[],items=Array.from({length:20},(_,i)=>c.item('i'+i,'Row '+i)),virtual=c.virtual_list('rows').visual_size(200,96).children(...items).on_change(e=>events.push(e));
 app.mount(c.column().children(virtual)).step();assert.ok(virtual.item_bounds(items[0]).width>0);assert.throws(()=>virtual.item_bounds(items.at(-1)));const {x,y}=virtual.bounds();app.scroll(x+20,y+20,0,-1).step();assert.ok(virtual.scroll_offset()>0);assert.ok(events.at(-1).value>0);
 virtual.scroll_to(500);app.step();assert.ok(virtual.item_bounds(items.at(-1)).width>0);virtual.disabled();app.step();const before=virtual.scroll_offset();events.length=0;app.scroll(x+20,y+20,0,1).step();assert.equal(virtual.scroll_offset(),before);assert.equal(events.length,0);app.dispose();
}
console.log('PASS native command filtering/confirmation and virtual list scrolling');

{
 const app=new c.App(),changes=[],clicks=[],home=c.item('home','Home'),settings=c.item('settings','Settings').on_click(e=>clicks.push(e)),sidebar=c.sidebar('navigation').visual_size(240,180).children(home,settings).on_change(e=>changes.push(e));
 app.mount(c.column().children(sidebar)).step();press(app,settings.bounds());assert.equal(sidebar.selected(),settings);assert.equal(clicks.at(-1).node,settings);assert.ok(changes.at(-1).selectedHandle.generation>0);
 sidebar.open(false);app.step();assert.equal(sidebar.is_open(),false);sidebar.open();app.step();assert.equal(sidebar.is_open(),true);home.disabled();app.step();changes.length=0;press(app,home.bounds());assert.equal(sidebar.selected(),settings);assert.equal(changes.length,0);app.dispose();
 const closed=new c.App(),rail=c.sidebar('closed').open(false).children(c.item('home','Home'));closed.mount(c.column().children(rail)).step();assert.equal(rail.is_open(),false);closed.dispose();
}
{
 const app=new c.App();assert.equal(app.current_theme_preset(),undefined);const names=app.theme_presets();assert.ok(names.length>=3);assert.ok(names.every(Boolean));app.mount(c.column().children(c.button('action').label('Action')));
 for(let i=0;i<names.length;i++){app.theme_preset(i).step();assert.equal(app.current_theme_preset(),i);}assert.throws(()=>app.theme_preset(names.length));assert.equal(app.current_theme_preset(),names.length-1);app.dispose();
}
console.log('PASS Sidebar navigation, exact cell selection and app theme presets');

{
 const app=new c.App(),events=[],button=c.button('save').label('Save').on_click(e=>events.push(e)),root=c.root_view().padding(24).children(c.column().children(button));
 app.mount(root).step();assert.ok(button.bounds().x>=24);press(app,button.bounds());assert.equal(events.at(-1).node,button);root.padding(40);app.step();assert.ok(button.bounds().x>=40);app.dispose();
 const invalid=new c.App();assert.throws(()=>invalid.mount(c.column().children(c.root_view('nested'))));assert.equal(invalid.disposed,true);
}
console.log('PASS native root view padding and child callbacks');

{
 const app=new c.App();assert.throws(()=>app.clipboard_set('Unicode ✓'),/backend unavailable/);assert.throws(()=>app.clipboard_get(),/backend unavailable/);assert.throws(()=>app.clipboard_set('bad\0text'));assert.throws(()=>app.clipboard_set('x'.repeat(65537)));app.dispose();
 assert.throws(()=>c.plot('bad').plot_points([[16385,0]]));assert.throws(()=>c.data_table('bad').table_data(['Name'],[['x'.repeat(129)]]));
}

{
 const app=new c.App(),events=[],speech=c.speech('recognition').on_change(e=>events.push(e));app.mount(c.column().children(speech)).step();assert.throws(()=>speech.speech_report('early',true));speech.speech_start();assert.throws(()=>speech.speech_start());speech.speech_report('hel');app.step();speech.speech_report('hello ✓',true);app.step();assert.deepEqual(events.map(e=>e.text),['','hel','hello ✓']);assert.deepEqual(events.map(e=>e.value),[0,1,1]);assert.equal(speech.speech_active(),true);assert.equal(speech.speech_transcript(),'hello ✓');speech.speech_finish();app.step();assert.equal(events.at(-1).value,2);assert.equal(events.at(-1).checked,false);assert.equal(speech.speech_active(),false);assert.equal(speech.speech_transcript(),'hello ✓');speech.disabled();assert.throws(()=>speech.speech_start());app.dispose();
}
console.log('PASS Speech host reports and copied lifecycle snapshots');

{
 const app=new c.App(),events=[],red=c.item('red','Red'),blue=c.item('blue','Blue'),round=c.item('round','Round'),color=c.item('color','Choose a color').question(true).children(red,blue),shape=c.item('shape','Choose a shape').question(false).children(round),survey=c.questionnaire('survey').children(color,shape).on_change(e=>events.push(e));
 app.mount(c.column().children(survey)).step();assert.equal(survey.questionnaire_state().current,color);assert.equal(survey.questionnaire_state().complete,false);assert.throws(()=>survey.questionnaire_next());assert.throws(()=>red.label('Changed after freeze'));assert.equal(red.textValue,'Red');assert.throws(()=>red.disabled());assert.equal(red.disabledValue,false);
 press(app,red.bounds());press(app,blue.bounds());assert.deepEqual(events.map(e=>JSON.parse(e.text).selected),[[1],[2]]);assert.equal(blue.choice_selected(),true);assert.equal(red.choice_selected(),false);survey.questionnaire_next();app.step();assert.equal(survey.questionnaire_state().current,shape);assert.ok(round.bounds().width>0);survey.questionnaire_previous();app.step();assert.equal(survey.questionnaire_state().current,color);survey.questionnaire_next();app.step();assert.throws(()=>survey.questionnaire_submit());press(app,round.bounds());survey.questionnaire_submit();app.step();assert.equal(survey.questionnaire_state().complete,true);app.dispose();
}
console.log('PASS native Questionnaire validation, choices and copied snapshots');

{
 const app=new c.App({width:700,height:500}),events=[],first=c.item('first','First').children(c.label('Panel body')),second=c.item('second','Second').children(c.input('editor','Notes')),dock=c.dock('workspace').visual_size(480,300).children(first,second).on_change(e=>events.push(e));
 app.mount(c.column().children(dock)).step();assert.equal(dock.dock_active(),first);assert.ok(dock.dock_panel_bounds(second).width>0);const before=dock.dock_serialize();assert.ok(before.startsWith("CUIDOCK 1 "));assert.throws(()=>dock.dock_move_split(second,first,'diagonal'));assert.equal(dock.dock_serialize(),before);assert.throws(()=>second.label('Frozen panel metadata'));assert.equal(second.textValue,'Second');
 dock.dock_move_split(second,first,'horizontal',.4);app.step();const after=dock.dock_serialize();assert.ok(after.startsWith("CUIDOCK 1 "));assert.notEqual(after,before);assert.notEqual(dock.dock_panel_bounds(first).x,dock.dock_panel_bounds(second).x);dock.dock_activate(second);app.step();assert.equal(dock.dock_active(),second);assert.ok(events.at(-1).selectedHandle.generation>0);dock.disabled();assert.throws(()=>dock.dock_activate(first));app.dispose();
}
console.log('PASS native Dock split layout, activation snapshots and metadata freeze');

{
 const app=new c.App({width:900,height:600}),fields=[],pages=[];
 const toggle=c.switch_control('updates','Automatic updates').checked().on_change(e=>fields.push(e));
 const name=c.input('name','Alex').detail('Display name').on_change(e=>fields.push(e));
 const count=c.number_input('count').detail('Retry count').range(0,10).value(3).on_change(e=>fields.push(e));
 const general=c.item('general','General').children(c.item('account','Account').children(toggle,name,count));
 const advanced=c.item('advanced','Advanced').children(c.item('network','Network').children(c.checkbox('proxy','Use proxy')));
 const settings=c.settings('preferences').visual_size(760,400).children(general,advanced).on_change(e=>pages.push(e));
 app.mount(c.column().children(settings));assert.throws(()=>settings.settings_page());app.step();assert.equal(settings.settings_page(),general);
 press(app,settings.settings_field_bounds(toggle));assert.equal(toggle.is_checked(),false);assert.equal(fields.at(-1).checked,false);
 press(app,settings.settings_field_bounds(name));app.text_input('é').step();assert.ok(name.current_text().includes('é'));assert.equal(fields.at(-1).text,name.current_text());
 count.value(7);settings.settings_select(advanced);settings.settings_select(general);app.step();assert.equal(pages.length,2);assert.deepEqual(pages.map(e=>app.nodes.get(`${e.selectedHandle.slot}:${e.selectedHandle.generation}`)),[advanced,general]);
 assert.notDeepEqual(pages[0].selectedHandle,pages[1].selectedHandle);assert.equal(settings.settings_page(),general);
 fields.length=0;settings.settings_reset(general);app.step();assert.equal(toggle.is_checked(),true);assert.equal(name.current_text(),'Alex');assert.equal(count.current_value(),3);assert.equal(fields.length,3);
 assert.throws(()=>general.label('Frozen'));assert.equal(general.textValue,'General');assert.throws(()=>settings.settings_query('é'.repeat(129)));
 settings.settings_query('Advanced');app.step();assert.throws(()=>settings.settings_field_bounds(toggle));
 settings.settings_query('').settings_select(general);app.step();toggle.disabled();fields.length=0;press(app,settings.settings_field_bounds(toggle));assert.equal(fields.length,0);
 assert.throws(()=>settings.disabled());app.dispose();assert.throws(()=>settings.settings_page());
}
