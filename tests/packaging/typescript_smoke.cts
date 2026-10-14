import {App, button, row} from '@cui/native';
const app = new App({backend:'headless',width:480,height:240});
const save = button('save').label('Save').on_click(event => event.node.label('Saved'));
app.mount(row().children(save));
const delivered: number = app.step(1/60);
const width: number = save.bounds().width;
void delivered; void width;
app.dispose();
