import {App, NotificationV1} from '../index.cjs';

const app = new App();
app.step_only();
const event: NotificationV1 | undefined = app.poll_notification_v1();
if (event) {
  const frame: bigint = event.frame;
  const rawColumn: number = event.legacy.checkedRaw;
  void frame; void rawColumn;
  if (event.kind === 'boolean-change') {
    const value: boolean = event.value;
    void value;
  } else if (event.kind === 'table-change') {
    const reason: 'selection' | 'sort' = event.reason;
    const column: number = event.column;
    void reason; void column;
  } else if (event.kind === 'legacy') {
    const selected = event.selectedNode;
    void selected;
  }
  // @ts-expect-error snapshots expose readonly payloads
  event.frame = 0n;
  // @ts-expect-error activation/legacy/surface do not infer a boolean value
  const guessed: boolean = event.value;
  void guessed;
}
app.dispose();
