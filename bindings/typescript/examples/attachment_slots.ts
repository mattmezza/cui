import {App, column, label, button, attachment, attachment_slot} from '../../node/index.cjs';
const app = new App({backend: 'headless', width: 560, height: 360});
try {
  if (!app.supports_attachment_slots()) throw new Error('Attachment slots require a current bridge');
  const summary = label('No file inspected');
  let inspections = 0;
  const inspect = button('Inspect').on_click(() => {
    summary.label('Quarterly report.pdf · Reviewed today');
    inspections++;
  });
  const card = attachment('report', 'Quarterly report.pdf').detail('2.4 MB').visual_size(480, 0)
    .children(attachment_slot('metadata', 'metadata').children(label('Finance team')),
      attachment_slot('actions', 'actions', 120).gap(4).children(inspect));
  app.mount(column().children(card, summary)).step();
  const b = inspect.bounds();
  if (b.width <= 0 || b.height <= 0) throw new Error('Inspect button has no bounds');
  app.pointer('down', b.x + b.width / 2, b.y + b.height / 2)
    .pointer('up', b.x + b.width / 2, b.y + b.height / 2).step();
  if (inspections !== 1) throw new Error('Inspect did not update summary once');
  console.log('Inspect updated the file summary once.');
} finally { app.dispose(); }
