import {App} from '../../typescript';
const app = new App({backend:'headless'});
const supported: boolean = app.supports_responsive_notifications();
if (supported) {
  const same: App = app.set_responsive_notifications(true);
  same.set_responsive_notifications(false);
}
// @ts-expect-error Requires a boolean, not a number.
app.set_responsive_notifications(1);
app.dispose();
