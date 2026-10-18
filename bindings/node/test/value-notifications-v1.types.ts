import {App, ValueNotificationV1, NotificationV1} from '../../typescript/index.js';
const app = new App();
const old: NotificationV1 | undefined = app.poll_notification_v1();
const value: ValueNotificationV1 | undefined = app.poll_value_notification_v1();
if (value) {
 const frame: bigint = value.frame; void frame;
 if (value.kind === 'scalar-change') {const n: number = value.value; void n;}
 if (value.kind === 'text-change') {const s: string = value.text; void s;}
 if (value.kind === 'selection-change') {
  const slot: number = value.selectedHandle.slot; void slot;
  const node = value.selectedNode; void node;
  // @ts-expect-error copied handles are readonly
  value.selectedHandle.slot = 1;
 }
 // @ts-expect-error each discriminated alternative has its own payload
 const guessed: string = value.text; void guessed;
}
if (old) {
 // @ts-expect-error the existing union does not acquire new value kinds
 const scalar: 'scalar-change' = old.kind; void scalar;
}
app.dispose();
