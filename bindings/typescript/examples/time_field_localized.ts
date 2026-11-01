import {App,column,row,label,button,localized_time_field,time_locale_arabic,LocalizedTimeChange} from '../../node/index.cjs';
// Complete application-owned UI. C applies edits; callback return values are ignored.
const app=new App({backend:'headless',width:760,height:440});
const summary=label('Localized appointments');let last:LocalizedTimeChange|undefined;
const english=localized_time_field('english',{precision:'second',minimum:[8,0,0],maximum:[18,0,0]},'English').time_value(9,30,0);
const arabic=localized_time_field('arabic',{locale:time_locale_arabic(),cycle:'h12',precision:'second',names:['ساعة','دقيقة','ثانية','فترة']},'العربية').time_value(13,45,0);
function applied():LocalizedTimeChange|undefined{return last;}
const observe=(change:LocalizedTimeChange)=>{last=change;summary.label(`Applied ${change.value.join(':')} at ${change.frame}`);};
english.on_localized_change(observe);arabic.on_localized_change(observe);
const clear=button('clear').label('Clear validation').on_click(()=>{english.clear_validation();arabic.clear_validation();});
const readonly=button('readonly').label('Toggle read-only').on_click(()=>english.readonly(!english.get_config().readonly));
const disabled=button('disabled').label('Toggle disabled').on_click(()=>english.disabled(!english.get_config().disabled));
const bounds=button('bounds').label('Toggle bounds').on_click(()=>{const old=english.get_config();english.configure({...old,editGeneration:old.editGeneration+1n,minimum:old.minimum?null:[8,0,0],maximum:old.maximum?null:[18,0,0]});});
try{
 app.enable_localized_time_fields(16);app.mount(column().children(summary,row().children(column().children(english),column().children(arabic)),row().children(clear,readonly,disabled,bounds)));
 app.step_only();app.step_only();english.latest_result();const box=english.bounds();app.pointer('down',box.x+box.width/5,box.y+box.height/2).pointer('up',box.x+box.width/5,box.y+box.height/2).text_input('10:15:30').step();
 const observed=applied();
 if(!observed||observed.value[0]!==10||observed.value[1]!==15||observed.value[2]!==30)throw new Error('missing actual applied TimeField record');
 console.log(`PASS TS localized UI: ${observed.value.join(':')} frame ${observed.frame}`);
}finally{app.dispose();}
