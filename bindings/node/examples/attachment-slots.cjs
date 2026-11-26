'use strict';
// Headless by default; --font /absolute/font.ttf opts into a visible SDL window.
const {App, column, label, button, attachment, attachment_slot} = require('..');
const argv = process.argv.slice(2);
if (argv.length && (argv.length !== 2 || argv[0] !== '--font'))
  throw new Error('Usage: attachment-slots.cjs [--font /absolute/font.ttf]');
const font = argv[1];
const app = new App({backend: font ? 'sdl' : 'headless', fontPath: font,
  width: 560, height: 360, title: 'Project files'});
try {
  if (!app.supports_attachment_slots()) throw new Error('Attachment slots require a current bridge');
  const summary = label('Select Inspect for file details.');
  let inspections = 0;
  const inspect = button('Inspect').on_click(() => {
    summary.label('Quarterly report.pdf · Finance team · 1.4 MB');
    inspections++;
  });
  const card = attachment('report', 'Quarterly report.pdf').detail('1.4 MB')
    .visual_size(460, 0).attachment_status('complete').children(
      attachment_slot('preview', 'media').children(label('PDF')),
      attachment_slot('metadata', 'metadata').children(label('Finance team')),
      attachment_slot('actions', 'actions', 120).gap(4).children(inspect));
  app.mount(column().gap(12).children(label('Project files'), card, summary));
  if (font) {
    app.run();
  } else {
    app.step();
    const {x, y, width, height} = inspect.bounds();
    if (width <= 0 || height <= 0) throw new Error('Inspect button has no bounds');
    app.pointer('down', x + width / 2, y + height / 2)
      .pointer('up', x + width / 2, y + height / 2).step();
    if (inspections !== 1) throw new Error('Expected one Inspect activation');
    console.log('Inspect updated the file summary once.');
  }
} finally {
  app.dispose();
}
