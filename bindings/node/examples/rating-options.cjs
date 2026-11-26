'use strict';
const assert=require('node:assert/strict');
const {App,button,column,label,rating}=require('../index.cjs');
// No window by default; pass a TrueType font path to run the native UI.
const fontPath=process.argv[2];
const status=label('Your score: 1.5');
const score=rating('score','Delivery').total(5).value(1.5).on_change(e=>status.label(`Your score: ${e.value}`));
const app=new App({backend:fontPath?'sdl':'headless',fontPath,width:460,height:260});
try {
  const supported=app.supports_rating_options();
  if(supported)score.configure({halfSteps:true,hoverPreview:true,stepDownFilled:true,clearable:true,designSize:'lg',width:360,fillRgba:0xce4275ff,previewRgba:0x245cb6ff});
  else score.value(2);
  const reset=button('Restore whole stars').on_click(()=>supported?score.reset_config():score.value(0));
  app.mount(column().gap(12).children(label('Rate your delivery'),status,score,reset));
  if(fontPath)app.run();
  else {
    app.step();
    const b=score.numeric_part_bounds(13),x=b.x+b.width/4,y=b.y+b.height/2;
    app.pointer('down',x,y).pointer('up',x,y).step();
    assert.equal(score.current_value(),supported?3.5:4);
    if(supported)score.reset_config();assert.equal(score.get_config(),undefined);
  }
} finally {app.dispose();}
