'use strict';
// Complete authored UI. Default is windowless; --native uses CUI_FONT_PATH.
const c=require('../index.cjs');
const reject=process.argv.includes('--reject'),single=process.argv.includes('--single'),vertical=process.argv.includes('--vertical');
let owner=single?c.slider_single(60):c.slider_range(20,60),accept=!reject;
const status=c.label('Ready');
const control=c.slider_model('selection',c.slider_config({orientation:vertical?'vertical':'horizontal',sourceGeneration:0xfedcba9876543210n}),owner).on_request(request=>{
 if(request.phase===1&&accept){app.accept_slider_request(request.receipt,request.requested);owner=request.requested;}
 status.label(`phase=${request.phase} requested=${JSON.stringify(request.requested)} owner=${JSON.stringify(owner)}`);
});
const toggle=c.button('policy').label('Toggle acceptance').on_click(()=>{accept=!accept;status.label(accept?'Accepting':'Rejecting');});
const native=process.argv.includes('--native');
const app=new c.App({backend:native?'sdl':'headless',fontPath:native?process.env.CUI_FONT_PATH:undefined,width:480,height:400});
try {
 app.mount(c.column().children(c.label('Controlled selection'),c.row().children(c.column().children(control)),toggle,status));
 if(native)app.run();
 else {
  app.step_only();const b=control.part_bounds(4);if(!b)throw new Error('missing actual thumb');
  app.pointer('down',b.x+b.width/2,b.y+b.height/2).pointer('up',b.x+b.width/2,b.y+b.height/2).step();
  const before=owner;app.key('down',vertical?'up':'right').step();
  if(accept&&owner===before)throw new Error('owner did not accept');
  if(!accept&&owner!==before)throw new Error('rejection changed owner');
  console.log('PASS Node-authored controlled UI: '+status.textValue);
 }
}finally{app.dispose();}
