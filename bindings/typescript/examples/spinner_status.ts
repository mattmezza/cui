import cui from '../../node/index.cjs';
import type {IconResource, SpinnerConfigInput, SpinnerPresentation} from '../../node/index.cjs';

const {App, button, column, row, label, rotating_spinner, IconSource} = cui;
// Pass a font path to open the UI; omission runs a headless interaction check.
declare const process: {argv: string[]};
const font = process.argv[2];
const style: SpinnerConfigInput = {size:'medium', easing:'quadratic'};
const loading = rotating_spinner('loading', style);
const status = label('Syncing workspace…');
const finish = button('finish').primary().label('Finish sync');
let completed = false;
let artwork: IconResource | undefined;
finish.on_click(() => {
  if (artwork) {
    loading.set_icon_resource(null);
    artwork.close();
  }
  loading.remove(); status.label('Workspace is up to date');
  finish.disabled().label('Sync complete'); completed = true;
});
const app = new App({width:480,height:180,backend:font?'sdl':'headless',fontPath:font});
try {
  app.mount(column().gap(12).children(row().gap(8).children(loading,status),finish));
  if (app.supports_spinner_icon_resource()) {
    artwork = app.icon_resource(IconSource.svg(
      '<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16">' +
      '<path d="M8 1a7 7 0 0 1 7 7h-3a4 4 0 0 0-4-4z"/></svg>'));
    loading.set_icon_resource(artwork);
  }
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
