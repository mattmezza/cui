import {App, attachment, AttachmentActionNotificationV1, ComponentNotificationV1, ValueNotificationV1} from '../../typescript/index.js';
const app = new App();
attachment('file').attachment_actions({open: true, remove: false, retry: true}).attachment_actions();
const old: ValueNotificationV1 | undefined = app.poll_value_notification_v1();
const event: ComponentNotificationV1 | undefined = app.poll_component_notification_v1();
if (event?.kind === 'attachment-action') {
 const action: AttachmentActionNotificationV1 = event;
 const source: 'pointer'|'keyboard'|'accessibility'|undefined = action.source;
 const frame: bigint = action.frame;
 void source; void frame;
}
// @ts-expect-error independent choices must be booleans
attachment().attachment_actions({open: 1});
void old;
