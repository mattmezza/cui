import {App,DialogOptions,DialogAction,DialogSlotRole,dialog,alert_dialog,dialog_slot,button,column} from '..';
const options: DialogOptions={centered:true,width:360};
const action: DialogAction='confirm',role: DialogSlotRole='footer';
const d=dialog('d').dialog_options(options).children(dialog_slot('f',role).children(button('OK').dialog_action(action)));
alert_dialog('a').dialog_options({show_close:true}).children(dialog_slot('body','body'));
const a=new App();const supported:boolean=a.supports_dialog_composition();column().children(d);void supported;
// @ts-expect-error unknown role
dialog_slot('x','unknown');
// @ts-expect-error typed action
button().dialog_action('delete');
// @ts-expect-error boolean flag
d.dialog_options({centered:1});
