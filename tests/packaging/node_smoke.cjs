// SPDX-License-Identifier: MIT
'use strict';
const assert = require('node:assert/strict');
const {spawnSync} = require('node:child_process');
const path = require('node:path');
assert(!process.env.CUI_NODE_ADDON);
const cui = require('@cui/native');
let clicks = 0;
// Unicode fallback is covered separately; this probe accepts a supplied Latin font.
const widget = cui.button('save').label('Save changes').on_click(() => ++clicks);
const options = process.env.CUI_PACKAGE_SMOKE_BACKEND === 'sdl'
  ? {backend:'sdl', fontPath:process.env.CUI_PACKAGE_SMOKE_FONT} : {};
const app = new cui.App(options);
app.mount(cui.row().children(widget)); app.step();
const bounds = widget.bounds();
assert(bounds.width > 0 && bounds.height > 0);
app.pointer('down', bounds.x + bounds.width/2, bounds.y + bounds.height/2);
app.pointer('up', bounds.x + bounds.width/2, bounds.y + bounds.height/2);
app.step(); assert.equal(clicks, 1); app.dispose();
const root = path.dirname(require.resolve('@cui/native'));
for (const [file, success] of [[path.join(root,'native/cui.node'),true], ['/missing-cui-artifact.node',false]]) {
  const result = spawnSync(process.execPath,['-e',"const {App}=require('@cui/native');const a=new App();a.dispose()"],
    {env:{...process.env,CUI_NODE_ADDON:file}});
  assert.equal(result.status === 0, success);
}
const fs = require('node:fs');
const addon = path.join(root,'native/cui.node');
fs.renameSync(addon, addon + '.hidden');
try {
  const result = spawnSync(process.execPath,['-e',"require('@cui/native')"],{env:process.env});
  assert.notEqual(result.status, 0);
  assert.match(result.stderr.toString(), /native.*cui\.node/);
} finally { fs.renameSync(addon + '.hidden',addon); }
console.log('installed Node bridge smoke passed');
