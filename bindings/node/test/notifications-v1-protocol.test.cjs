'use strict';
// Run in a fresh process with the shared test-only C V1 provider preloaded.
// Synthetic boundary values are protocol proof, not real UI queue evidence.
const assert = require('node:assert/strict');
const c = require('..');
const app = new c.App();
try {
  app.mount(c.column().children(c.label('protocol fixture')));
  const mode = process.env.CUI_NOTIFICATION_FIXTURE;
  if (['limit','zero','repeat','size','unavailable'].includes(mode)) {
    assert.throws(() => app.poll_notification_v1(), /limit|capacity|layout|capability/);
    // The injected failure cannot break existing legacy polling/dispatch.
    assert.equal(app.step(), 0);
  } else {
    const events = [];
    let event;
    while ((event = app.poll_notification_v1()) !== undefined) events.push(event);
    assert.deepEqual(events.map(x => x.frame), [(1n<<53n)-1n,1n<<53n,(1n<<53n)+1n,(1n<<64n)-1n]);
    assert.deepEqual(events.map(x => x.kind), ['activation','boolean-change','table-change','surface-close']);
    assert(events.every(x => x.node === undefined && x.handle.slot === 0xffffffff));
    assert.equal(events[2].column,0xfffffffe);
    assert.equal(events[2].legacy.checkedRaw,0xffffffff);
    assert.equal(events[1].value,true);
    assert.equal(events[3].result,3);
    assert.equal(events[0].legacy.text.length,mode === 'capacity' ? 8192 : 3);
  }
} finally { app.dispose(); }
