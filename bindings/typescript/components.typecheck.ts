import * as c from './index';
const save=c.button('save').label('Save').primary().rounded_md().on_click(event => event.node.label('Saved'));
const calendar=c.calendar('calendar').date_value(1980,1,1).on_change(event=>console.log(event.text));
const parent=c.item('branch','Documents').step_status(0).checked(true);
const tree=c.tree('files').visual_size(240,180).children(parent,c.item('child','Notes').step_status(1));
const dialog=c.dialog('dialog','Edit').detail('Edit details').children(c.input('name','Alex'),save).on_close(event=>console.log(event.result));
const picker=c.date_picker('birthday').date_value(1980,1,1).open();
const app=new c.App({backend:'headless'}).mount(c.column().children(calendar,tree,dialog,picker,c.image('photo').image_rgba(new Uint8Array(16),2,2)));
app.scroll(5,5,0,-80).step();
// @ts-expect-error Static badges do not accept button-only styling.
c.badge('status','Ready').primary();
// @ts-expect-error AlertDialog uses its native actions rather than arbitrary children.
c.alert_dialog('confirm','Delete?').children(save);
// @ts-expect-error Only scrollable exposes scrolling state.
c.label('Text').scroll_extent(300,900);
// @ts-expect-error The picker has no Calendar-only day hit-test method.
picker.calendar_day_bounds(1980,1,1);
