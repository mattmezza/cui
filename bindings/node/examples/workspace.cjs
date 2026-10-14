'use strict';
const c=require('../index.cjs');
const headless=process.argv.includes('--headless');
const status=c.label('Ready');
const name=c.input('name','Alex Morgan').on_change(e=>status.label('Name: '+e.text));
const notifications=c.checkbox('notifications','Email notifications').checked();
const birthday=c.date_picker('birthday','Birthday').date_value(1980,1,1);
const confirmation=c.dialog('confirmation','Save account changes').detail('Confirm the new account details.')
 .children(c.label('Your settings stay in your application model.')).on_close(()=>status.label('Dialog closed'));
const save=c.button('save').label('Save changes').primary().on_click(()=>confirmation.open());
const account=c.item('account','Account').children(c.column().gap_units(2).children(name,birthday,save,status));
const settings=c.item('settings','Preferences').children(notifications);
const pages=c.tabs('pages').children(account,settings).select(account);
const app=new c.App({width:720,height:480,title:'CUI account workspace',backend:headless?'headless':'sdl',fontPath:headless?undefined:process.env.CUI_FONT_PATH});
app.mount(c.column().gap_units(3).children(c.label('Workspace'),pages,confirmation));
if(headless) {
 const assert=require('node:assert/strict');app.step();const {x,y,width,height}=save.bounds();
 app.pointer('down',x+width/2,y+height/2).pointer('up',x+width/2,y+height/2).step();app.step();assert.equal(confirmation.is_open(),true);
 confirmation.open(false);app.step();pages.select(settings);app.step();assert.ok(notifications.bounds().width>0);app.dispose();
 console.log('PASS account workspace: native button, dialog and page selection');
} else app.start();
