import {App, button, notification, column} from '../../typescript';

const action = button('save').label('Save').on_click(event => event.node.label('Saved'));
const notice = notification('ready', 'Ready').open().action(action);
const app = new App();
if (app.supports_notification_action()) app.mount(column().children(notice));

// @ts-expect-error Notification actions require Buttons
notification('invalid').action(column());
// @ts-expect-error Only Notification exposes action
column().action(action);
