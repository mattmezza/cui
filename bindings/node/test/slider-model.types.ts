import {App,column,slider_model,slider_config,slider_range,SliderValue,SliderRequest} from '../../typescript';
const app=new App();let owner:SliderValue=slider_range(20,60);
const node=slider_model('typed',slider_config({sourceGeneration:0xffffffffffffffffn}),owner).on_request((r:SliderRequest)=>{
 const id:bigint=r.receipt.appEpoch;
 if(r.phase===1){app.accept_slider_request(r.receipt,r.requested);owner=r.requested;}
 if(r.requested.kind==='range'){const start:number=r.requested.start;void start;}
 // @ts-expect-error copied requests are immutable
 r.receipt.node.slot=1;
 // @ts-expect-error range is never scalarized
 app.accept_slider_request(r.receipt,5);
 void id;
});
app.mount(column().children(node));
// @ts-expect-error identities never use Number
slider_config({sourceGeneration:9007199254740993});
// @ts-expect-error Single cannot contain a range start
const bad:SliderValue={kind:'single',start:20,end:60};
void bad;app.dispose();
