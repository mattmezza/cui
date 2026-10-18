'use strict';
const assert = require('node:assert/strict');
const c = require('..');
const native = require(process.env.CUI_NODE_ADDON);
function click(app, node) {
  const r = node.bounds();
  app.pointer('down', r.x+r.width/2, r.y+r.height/2).pointer('up', r.x+r.width/2, r.y+r.height/2);
  app.step_only();
}
const app = new c.App({width: 1200});
try {
  let calls = 0;
  const check = c.checkbox('check', 'Check').on_change(() => calls++);
  const action = c.button('action').label('Action');
  const field = c.input('text','');
  const table = c.data_table('table').table_data(['A','B','C'], [['x','y','z']]).selection_mode('cell');
  app.mount(c.column().children(check, action, field, table)); app.step_only();
  click(app, check);
  const event = app.poll_notification_v1();
  assert.equal(event.kind, 'boolean-change'); assert.equal(event.value, true);
  assert.equal(typeof event.frame, 'bigint'); assert.equal(event.legacy.checkedRaw, 1);
  assert.equal(calls, 0); assert(Object.isFrozen(event)); assert(Object.isFrozen(event.legacy));
  click(app, check);
  assert.equal(app._dispatch(), 1); assert.equal(calls, 1);
  assert.equal(app.poll_notification_v1(), undefined);
  const r = table.table_cell_bounds(0,2);
  app.pointer('down',r.x+r.width/2,r.y+r.height/2).pointer('up',r.x+r.width/2,r.y+r.height/2).step_only();
  const selected = app.poll_notification_v1();
  assert.equal(selected.kind,'table-change'); assert.equal(selected.column,3);
  assert.equal(selected.legacy.checkedRaw,3); assert.match(selected.legacy.text,/"column":3/);
  click(app, action);
  const activation = app.poll_notification_v1(); assert.equal(activation.kind,'activation');
  action.remove(); assert.equal(event.value,true);
  click(app,field); while(app.poll_notification_v1() !== undefined) {}
  app.text_input('héllo ✓').step_only();
  const textEvent=app.poll_notification_v1();
  assert.equal(textEvent.kind,'legacy'); assert.equal(textEvent.legacy.text,'héllo ✓');
  app.text_input('later').step_only(); assert.equal(textEvent.legacy.text,'héllo ✓');
  if (typeof native.testNotificationFrame === 'function') {
    for (const value of [(1n<<53n)-1n,1n<<53n,(1n<<53n)+1n,(1n<<64n)-1n]) {
      // Conversion-only uint64 fixture uses the same native BigInt conversion as polling.
      assert.equal(native.testNotificationFrame(value),value);
    }
    assert.throws(()=>native.testNotificationFrame(1n<<64n),/uint64/);
  }
} finally { app.dispose(); }
console.log('manual V1 notifications: real queue, table integers, lifetime, uint64 BigInt PASS');
