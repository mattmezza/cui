'use strict';
// CUI_NODE_ADDON points to the built addon. Use real keyboard input in a headless UI.
const {App, attachment, column, label} = require('..');
const app = new App({width: 480, height: 300});
try {
  const card = attachment('report', 'Quarterly report.pdf')
    .detail('Upload failed').attachment_status('failed');
  const status = label('Select a file action');
  const supported = app.supports_attachment_actions();
  if (supported) card.attachment_actions({open: true, remove: true, retry: true});
  app.mount(column().children(card, status)).step_only().step_only();
  if (!supported) {
    console.log('This bridge supports Attachment presentation only.');
  } else {
    for (let step = 0; step < 3; step++) {
      app.key('down', 'tab').key('up', 'tab').step_only();
      app.key('down', 'enter').key('up', 'enter').step_only();
      let event;
      while ((event = app.poll_component_notification_v1()) !== undefined) {
        if (event.kind === 'attachment-action') {
          status.label(`Requested ${event.action} for Quarterly report.pdf`);
          console.log(event.action, event.part, event.source, event.frame);
        }
      }
    }
  }
} finally {
  app.dispose();
}
