import {App, button, column, label, rating} from '../../node/index.cjs';
import type {RatingConfig} from '../../node/index.cjs';
const options: RatingConfig={halfSteps:true,hoverPreview:true,stepDownFilled:true,clearable:true,designSize:'lg',width:360,fillRgba:0xce4275ff};
const status=label('Your score: 1.5');
const score=rating('delivery','Delivery').value(1.5).on_change(event=>status.label(`Your score: ${event.value}`));
const app=new App({width:460,height:260});
try {
  const supported=app.supports_rating_options();
  if(supported)score.configure(options);else score.value(2);
  app.mount(column().gap(12).children(label('Rate your delivery'),status,score,button('Restore whole stars').on_click(()=>supported?score.reset_config():score.value(0)))).step();
  const b=score.numeric_part_bounds(13),x=b.x+b.width/4,y=b.y+b.height/2;
  app.pointer('down',x,y).pointer('up',x,y).step();
  if(score.current_value()!==(supported?3.5:4))throw new Error('fractional rating callback');
  if(supported)score.reset_config();
  if(score.get_config()!==undefined)throw new Error('rating reset');
}finally{app.dispose();}
