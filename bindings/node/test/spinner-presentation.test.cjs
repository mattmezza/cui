'use strict';
const assert = require('node:assert/strict');
const {App, column, rotating_spinner, spinner, IconSource} = require('../index.cjs');
const node = rotating_spinner('loading');
const shared = rotating_spinner('shared');
assert.throws(() => node.set_icon_resource(null), /not mounted/);
const app = new App({backend:'headless'});
try {
  if (process.env.CUI_EXPECT_SPINNER_UNSUPPORTED === '1') {
    assert.equal(app.supports_spinner_presentation(), false);
    const root = column('root').children(node);
    assert.throws(() => app.mount(root), /compatible bridge symbol group/);
    const legacy = spinner('legacy').visual_size(22,22);
    app.mount(column('old').children(legacy)); app.step();
    assert.equal(legacy.bounds().width, 22);
  } else {
    assert.equal(app.supports_spinner_presentation(), true);
    assert.equal(app.supports_spinner_icon_resource(), process.env.CUI_EXPECT_SPINNER_ICON_UNSUPPORTED !== '1');
    const config = node.get_config();
    assert.ok(Object.isFrozen(config));
    assert.equal(config.size, 'medium'); assert.equal(config.icon, '');
    app.mount(column('root').children(node, shared));
    assert.deepEqual(node.get_config(), config); assert.equal(node.presentation(), undefined);
    app.step(.1); const first = node.presentation();
    assert.equal(first.phase,0); assert.ok(Object.isFrozen(first));
    assert.equal(node.bounds().width,16); assert.deepEqual(node.presentation(),first);
    node.configure({size:'custom',pixels:24,rgba:0,easing:'linear'});
    assert.equal(node.presentation(),undefined); app.step(.2);
    assert.ok(node.presentation().phase>0); assert.equal(node.bounds().width,24);
    const before=node.presentation();
    assert.throws(()=>node.configure({icon:'not-an-embedded-icon'}));
    assert.deepEqual(node.presentation(),before);
    if (process.env.CUI_EXPECT_SPINNER_ICON_UNSUPPORTED === '1') {
      assert.throws(() => node.set_icon_resource(null), /compatible bridge symbol group/);
    } else {
      const resource = app.icon_resource(IconSource.svg('<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16"><path d="M0 0h16v16H0z"/></svg>'));
      const other = new App({backend:'headless'});
      try {
        const foreign = other.icon_resource(IconSource.empty());
        assert.throws(() => node.set_icon_resource(foreign), /different App/);
        assert.throws(() => node.set_icon_resource({}), /different App/);
      } finally {other.dispose();}
      assert.deepEqual(node.presentation(), before);
      assert.equal(node.set_icon_resource(resource), node);
      shared.set_icon_resource(resource);
      assert.equal(node.presentation(), undefined); app.step(.1);
      assert.ok(node.presentation()); assert.ok(resource.bitmap().width > 0);
      assert.throws(() => resource.close(), /invalid state/);
      app.step(.1); assert.ok(node.presentation());
      assert.equal(node.set_icon_resource(null), node);
      assert.throws(() => resource.close(), /invalid state/);
      shared.set_icon_resource(null); resource.close();
      assert.throws(() => node.set_icon_resource(resource), /closed/);
      assert.equal(node.presentation(), undefined); app.step(.1); assert.ok(node.presentation());
    }
    node.configure({reducedMotion:true}); app.step(.1);
    assert.equal(node.presentation().nextWakeSeconds,0);
    assert.throws(()=>node.visual_size(1,1),/configuration/);
  }
  for (const bad of [{size:'bad'},{size:'custom'},{pixels:1},{duration:NaN},{duration:1e-100},
      {duration:61},{rgba:true},{icon:'x'.repeat(64)},{icon:'x\0y'},{size:'large',remSize:4096},
      {reducedMotion:1}]) assert.throws(()=>rotating_spinner('bad',bad));
} finally {app.dispose();}
