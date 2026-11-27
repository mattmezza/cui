import {App,column,stepper,item} from '../../node/index.cjs';
const first=item('first','Set up your workspace').image_rgba(new Uint8Array([255,255,255,255]),1,1);
const steps=stepper('steps').stepper_visual(true,false,'sm').design_size('lg').children(first,item('second','Review your configuration'));
const app=new App({backend:'headless',width:800,height:400});
if (app.supports_stepper_centered_fill()) steps.stepper_centered_fill(false, 'sm', 360, 0);
else if (app.supports_stepper_wrapped_fill()) steps.stepper_wrapped_fill(false, 'sm', 360, 0);
else if (app.supports_stepper_fill()) steps.stepper_fill(false, 'sm', 360, 64);
try {app.mount(column().children(steps));app.step();if(steps.numeric_part_bounds(2).width<=0)throw new Error('missing step geometry');}finally {app.dispose();}
