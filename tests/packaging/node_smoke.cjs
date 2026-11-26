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
assert.equal(app.supports_radio_tab_index(), true, 'installed bridge lacks Radio ordering');
assert.equal(app.supports_radio_paint(), true, 'installed bridge lacks Radio paint');
assert.equal(app.supports_rating_options(), true, 'installed bridge lacks Rating options');
assert.equal(app.supports_stepper_fill(), true, 'installed bridge lacks Stepper Fill');
assert.equal(app.supports_disclosure_presentation(), true, 'installed bridge lacks disclosure presentation');
assert.equal(app.supports_disclosure_heading(), true, 'installed bridge lacks disclosure headings');
assert.equal(app.supports_disclosure_divider(), true, 'installed bridge lacks disclosure dividers');
assert.equal(app.supportsVirtualListRows(), true, 'installed bridge lacks VirtualList live rows');
assert.equal(app.supports_carousel_basis(), true, 'installed bridge lacks fractional Carousel slides');
assert.equal(app.supports_carousel_navigation(), true, 'installed bridge lacks Carousel navigation');
const changes = [];
const overview = cui.item('overview', 'Overview').carousel_basis(0.5);
const activity = cui.item('activity', 'Activity').carousel_basis(0.5);
const slides = cui.carousel('slides').visual_size(240, 100).children(overview, activity)
  .on_change(event => changes.push(event.node));
app.mount(cui.column().children(widget, slides)); app.step();
assert.equal(activity.bounds().x - overview.bounds().x, 128);
const bounds = widget.bounds();
assert(bounds.width > 0 && bounds.height > 0);
app.pointer('down', bounds.x + bounds.width/2, bounds.y + bounds.height/2);
app.pointer('up', bounds.x + bounds.width/2, bounds.y + bounds.height/2);
app.step(); assert.equal(clicks, 1);
assert.equal(slides.carousel_adjacent(), activity);
assert.equal(slides.selected(), overview);
assert.equal(slides.carousel_step(), activity);
assert.equal(changes.length, 0);
app.step();
assert.deepEqual(changes, [slides]);
assert.equal(slides.selected(), activity);
app.dispose();
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
