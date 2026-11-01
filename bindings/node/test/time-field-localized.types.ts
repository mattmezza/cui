import {App,column,localized_time_field,localized_time_config,time_locale_arabic,LocalizedTimeChange,LocalizedTimeField} from '../../typescript';
const app=new App();const node=localized_time_field('time',localized_time_config({locale:time_locale_arabic(),cycle:'h12',precision:'second',editGeneration:0xffffffffffffffffn}));
const constructed=new LocalizedTimeField('another');void constructed;
node.time_value(13,45).readonly(false).on_localized_change((event:LocalizedTimeChange)=>{
 const frame:bigint=event.frame;const hour:number=event.value[0];void frame;void hour;
 // @ts-expect-error applied record is immutable
 event.value[0]=0;
 // @ts-expect-error frame identities never use Number
 const wrong:number=event.frame;void wrong;
});
app.mount(column().children(node));app.step_only();node.latest_result();app.poll_localized_time_change();app.dispose();
// @ts-expect-error generation is uint64 BigInt
localized_time_config({editGeneration:1});
// @ts-expect-error unsupported locale guessing
localized_time_config({cycle:'automatic'});
