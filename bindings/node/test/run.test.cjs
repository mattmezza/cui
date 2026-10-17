'use strict';
const assert = require('node:assert/strict');
const {App,row} = require('..');
const app = new App();
try {
  app.mount(row());
  assert.throws(()=>app.run(),/unavailable/);
  assert.equal(app.running,false);
  assert.equal(app.step(),0);
} finally {app.dispose();}
