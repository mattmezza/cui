import {App, button, column, row, label, rotating_spinner} from '../../node/index.cjs';
import type {SpinnerConfigInput, SpinnerPresentation} from '../../node/index.cjs';

// Pass a font path to open the UI; omission runs a headless interaction check.
declare const process: {argv: string[]};
const font = process.argv[2];
const style: SpinnerConfigInput = {size:'medium', easing:'quadratic'};
const loading = rotating_spinner('loading', style);
const status = label('Syncing workspace…');
const finish = button('finish').primary().label('Finish sync');
let completed = false;
finish.on_click(() => {
  loading.remove(); status.label('Workspace is up to date');
  finish.disabled().label('Sync complete'); completed = true;
});
const app = new App({width:480,height:180,backend:font?'sdl':'headless',fontPath:font});
try {
  app.mount(column().gap(12).children(row().gap(8).children(loading,status),finish));
  if (font) {
    app.run();
  } else {
    app.step(.1);
    const sample: SpinnerPresentation | undefined = loading.presentation();
    if (!sample || !sample.nextWakeSeconds) throw new Error('Expected rotating indicator');
    const {x, y, width, height} = finish.bounds();
    app.pointer('down', x + width / 2, y + height / 2)
      .pointer('up', x + width / 2, y + height / 2).step();
    app.step();
    if (!completed) throw new Error('Expected sync completion');
  }
} finally {app.dispose();}
