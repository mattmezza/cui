// SPDX-License-Identifier: MIT
'use strict';
const c=require('..');
function appointments(options={}) {
  const app=new c.App({width:760,height:440,...options});
  let bounded=true;const applied=[];
  const summary=c.label('Edit either appointment; changes are already applied');
  const english=c.localized_time_field('english',{precision:'second',minimum:[8,0,0],maximum:[18,0,0]},'English 24-hour').time_value(9,30,0);
  const arabic=c.localized_time_field('arabic',{locale:c.time_locale_arabic(),cycle:'h12',precision:'second',names:['ساعة','دقيقة','ثانية','فترة']},'العربية 12-hour').time_value(13,45,0);
  const observe=event=>{applied.push(event);summary.label(`Applied ${event.value.join(':')}; frame ${event.frame}; status ${event.frameStatus}`);};
  english.on_localized_change(observe);arabic.on_localized_change(observe);
  const clear=c.button('clear').label('Clear validation').on_click(()=>{english.clear_validation();arabic.clear_validation();});
  const readonly=c.button('readonly').label('Toggle read-only').on_click(()=>english.readonly(!english.get_config().readonly));
  const disabled=c.button('disabled').label('Toggle disabled').on_click(()=>english.disabled(!english.get_config().disabled));
  const bounds=c.button('bounds').label('Toggle bounds').on_click(()=>{
    bounded=!bounded;const old=english.get_config();
    english.configure({...old,editGeneration:old.editGeneration+1n,minimum:bounded?[8,0,0]:null,maximum:bounded?[18,0,0]:null});
  });
  app.enable_localized_time_fields(16);
  app.mount(c.column().gap(12).children(summary,
    c.row().gap(12).children(c.column().children(c.label('Morning'),english),c.column().children(c.label('Afternoon'),arabic)),
    c.row().gap(8).children(clear,readonly,disabled,bounds)));
  return {app,english,arabic,summary,applied,controls:{clear,readonly,disabled,bounds}};
}
module.exports={appointments};
if(require.main===module){
  const native=process.argv.includes('--native');
  const ui=appointments(native?{backend:'sdl',fontPath:process.env.CUI_TEST_FONT,initiallyHidden:process.argv.includes('--hidden')}:{backend:'headless'});
  try{if(native)ui.app.run();else{ui.app.step();console.log('Authored English/Arabic localized TimeFields',ui.english.current_time(),ui.arabic.current_time());}}
  finally{ui.app.dispose();}
}
