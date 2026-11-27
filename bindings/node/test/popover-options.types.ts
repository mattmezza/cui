import {App,PopoverOptions,column,button,popover} from '..';
const options:PopoverOptions={anchor:5,focus_content:true,hide_close:true,offset:8};
const app=new App(),trigger=button('Account');
const p=popover('menu').anchor(trigger).popover_options(options).popover_options(null);
const supported:boolean=app.supports_popover_options();void supported;
app.mount(column().children(trigger,p));
// @ts-expect-error invalid anchor
p.popover_options({anchor:8});
// @ts-expect-error booleans are strict
p.popover_options({arrow:1});
