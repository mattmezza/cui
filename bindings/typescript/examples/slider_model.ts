import {App,column,row,label,button,slider_model,slider_config,slider_range,SliderValue} from '../../node/index.cjs';
// Complete windowless TS UI; same N-API runtime and explicit full-pair acceptance.
let owner:SliderValue=slider_range(20,60),accept=true;
const status=label('Ready');
const control=slider_model('selection',slider_config({sourceGeneration:0xfedcba9876543210n}),owner).on_request(request=>{
 if(request.phase===1&&accept){app.accept_slider_request(request.receipt,request.requested);owner=request.requested;}
 status.label(`phase=${request.phase} owner=${JSON.stringify(owner)}`);
});
const toggle=button('accept').label('Toggle acceptance').on_click(()=>{accept=!accept;});
const app=new App({backend:'headless',width:480,height:360});
try {
 app.mount(column().children(label('Controlled TS selection'),row().children(control),toggle,status));app.step_only();
 const b=control.part_bounds(4);if(!b)throw new Error('missing thumb');
 app.pointer('down',b.x+b.width/2,b.y+b.height/2).pointer('up',b.x+b.width/2,b.y+b.height/2).step();
 const before=owner;app.key('down','right').step();if(owner===before)throw new Error('owner did not accept');
 accept=false;const accepted=owner;app.key('down','left').step();if(owner!==accepted)throw new Error('rejection mutated owner');
 console.log('PASS TS-authored controlled UI: '+JSON.stringify(owner));
}finally{app.dispose();}
