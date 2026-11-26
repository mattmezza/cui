import {App, attachment, column, label} from '../../node/index.cjs';
import type {AttachmentActionNotificationV1} from '../../node/index.cjs';

const app = new App({backend: 'headless', width: 420, height: 280});
try {
  if (!app.supports_attachment_actions()) throw new Error('This bridge lacks Attachment actions');
  const report = attachment('report', 'Report.pdf').detail('Ready for review')
    .attachment_status('complete').attachment_actions({open: true, remove: true, retry: true});
  const status = label('Select a file action');
  app.mount(column().children(report, status)).step_only().step_only();
  app.key('down', 'tab').key('up', 'tab').step_only();
  app.key('down', 'enter').key('up', 'enter').step_only();

  const event = app.poll_component_notification_v1();
  if (event?.kind !== 'attachment-action' || event.action !== 'open') {
    throw new Error('Expected the file to request Open');
  }
  showAction(event);

  function showAction(action: AttachmentActionNotificationV1): void {
    status.label(`Requested ${action.action} for Report.pdf`);
    console.log(`Received ${action.action} by ${action.source ?? 'unknown input'} at frame ${action.frame}`);
  }
} finally {
  app.dispose();
}
