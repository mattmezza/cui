import { App, notification, column } from '../../typescript/index.js';
const app = new App();
const notice = notification('n').auto_hide().auto_hide(false).open();
if (app.supports_notification_auto_hide()) app.mount(column().children(notice));
// @ts-expect-error strict boolean
notice.auto_hide(1);
// @ts-expect-error Notification only
column().auto_hide();
