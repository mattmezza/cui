import {App,column,stepper,item} from '../../node/index.cjs';
const first=item('first','First').image_rgba(new Uint8Array([255,255,255,255]),1,1);
const steps=stepper('steps').stepper_visual(true,false,'sm').design_size('lg').children(first,item('second','Second'));
const app=new App({backend:'headless',width:800,height:400});
try {app.mount(column().children(steps));app.step();if(steps.numeric_part_bounds(2).width<=0)throw new Error('missing step geometry');}finally {app.dispose();}
