'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const {App, column, collapsible, responsive_skeleton, skeleton} = require('..');
const native = require(process.env.CUI_NODE_ADDON);

function withApp(options, work) {
  const app = new App(options);
  try { work(app); }
  finally { app.dispose(); }
}

if (process.env.CUI_SKELETON_UNSUPPORTED === '1') {
  test('optional group absence leaves the app available for legacy mounting', () => {
    withApp({}, app => {
      const modern = responsive_skeleton('modern');
      assert.equal(app.supports_skeleton_presentation(), false);
      assert.throws(() => app.mount(column().children(modern)), /complete, compatible bridge/);
      assert.equal(app.root, null);
      assert.equal(app.nodes.size, 0);
      const legacy = skeleton('legacy').visual_size(72, 12);
      app.mount(column().children(legacy)).step();
      assert.deepEqual([...app.nodes.keys()], ['1:1', '2:1']);
      assert.deepEqual([legacy.bounds().width, legacy.bounds().height], [72, 12]);
    });
  });
} else {
  test('responsive geometry, frozen snapshots and legacy coexistence', () => {
    for (const width of [320, 480]) {
      withApp({width, height: 180}, app => {
        assert.equal(app.supports_skeleton_presentation(), true);
        const style = {width: .65, radius: 6, backgroundRgba: 0, secondary: true};
        const line = responsive_skeleton('line', style);
        const capped = responsive_skeleton('capped', {widthMode: 'grow', width: 0, maxWidth: 128});
        const circle = responsive_skeleton('circle', {widthMode: 'fixed', width: 40, height: 40});
        const legacy = skeleton('legacy').visual_size(72, 12);
        style.width = .2;
        const owned = line.get_config();
        assert.equal(owned.width, Math.fround(.65));
        assert.equal(owned.backgroundRgba, 0);
        assert.equal(owned.secondary, true);
        assert.ok(Object.isFrozen(owned));
        app.mount(column().children(line, capped, circle, legacy));
        assert.deepEqual(line.get_config(), owned);
        assert.equal(line.presentation(), undefined);
        assert.equal(app.step(.25), 0);
        assert.ok(Math.abs(line.bounds().width - (width - 32) * .65) < .001);
        assert.equal(capped.bounds().width, 128);
        assert.deepEqual([circle.bounds().width, circle.bounds().height], [40, 40]);
        assert.deepEqual([legacy.bounds().width, legacy.bounds().height], [72, 12]);
        assert.throws(() => line.visual_size(1, 1), /configuration/);
      });
    }
  });

  test('restyling, independent clocks and reduced motion', () => {
    withApp({}, app => {
      const line = responsive_skeleton('line');
      const other = responsive_skeleton('other', {reducedMotion: true});
      app.mount(column().children(line, other)).step(.25);
      const first = line.presentation();
      assert.equal(typeof first.frameSequence, 'bigint');
      assert.equal(first.phase, 0);
      assert.ok(Math.abs(first.nextWakeSeconds - first.coreTime - 1 / 60) < 1e-10);
      assert.deepEqual(line.presentation(), first);
      assert.ok(Object.isFrozen(first));
      line.configure({...line.get_config(), secondary: true, width: .5});
      assert.equal(line.presentation(), undefined);
      app.step(.5);
      const updated = line.presentation();
      assert.ok(updated.frameSequence > first.frameSequence);
      assert.equal(updated.phase, .5);
      assert.equal(updated.opacity, .75);
      assert.equal(first.phase, 0);
      assert.equal(other.presentation().nextWakeSeconds, 0);
      line.configure({...line.get_config(), reducedMotion: true});
      app.step(.25);
      assert.equal(line.presentation().opacity, 1);
      assert.equal(line.presentation().nextWakeSeconds, 0);
      line.configure({...line.get_config(), reducedMotion: false});
      app.step(.25);
      assert.equal(line.presentation().phase, 0);
    });
  });

  test('omitted frames, reopen and node removal', () => {
    withApp({}, app => {
      const line = responsive_skeleton('line');
      const panel = collapsible('panel', 'Profile').open().children(line);
      app.mount(column().children(panel)).step(.25);
      assert.equal(line.presentation().phase, 0);
      panel.open(false);
      app.step(.25);
      assert.equal(line.presentation(), undefined);
      panel.open(true);
      app.step(.25);
      assert.equal(line.presentation().phase, 0);
      line.remove();
      assert.throws(() => line.presentation(), /not mounted/);
      app.step();
    });
  });

  test('invalid options and atomic configuration validation', () => {
    const line = responsive_skeleton('line');
    const invalid = [{width: 0}, {width: NaN}, {width: 1.01}, {height: 0}, {duration: 1e-100},
      {duration: Infinity}, {maxWidth: 20}, {radius: -1}, {widthMode: 'percent'},
      {backgroundRgba: -1}, {secondary: 1}, {reducedMotion: null}, {widthMode: 'grow', width: 2}];
    withApp({}, app => {
      app.mount(column().children(line)).step();
      const before = line.presentation();
      for (const cfg of invalid) assert.throws(() => line.configure(cfg), /width|height|radius|duration|backgroundRgba|secondary/);
      assert.equal(line.get_config().width, 1);
      assert.deepEqual(line.presentation(), before);
    });
  });

  test('raw addon checks userdata and failed operations preserve native state', () => {
    const opts = {backend: 0, width: 320, height: 200, maxNodes: 4, maxEvents: 4};
    const first = native.create(opts);
    const second = native.create(opts);
    try {
      const root = native.makeNode(first, null, 2, 'root');
      const placeholder = native.makeNode(first, root, 35, 'placeholder');
      const configure = (...fields) => native.skeletonPresentation(first, 1, placeholder, ...fields);
      configure(6, 0, 40, 40, 0, 20, 2, 0x307AFFFF);
      assert.throws(() => native.skeletonPresentation(second, 2, placeholder), /another app/);
      assert.throws(() => configure(6, 0, 40, 40, 0, 20, -2, 0x307AFFFF), /invalid argument/);
      assert.throws(() => configure(6, 0, 40, 40, 0, 20, 1e-100, 0x307AFFFF), /underflows/);
      assert.throws(() => configure(6, 0, 40, 40), /missing argument/);
      assert.equal(native.skeletonPresentation(first, 2, placeholder).duration, 2);
      native.step(first, .25);
      assert.deepEqual(native.skeletonPresentation(first, 3, placeholder), {
        frameSequence: 1n, coreTime: .25, nextWakeSeconds: 0, phase: 0, opacity: 1,
      });
      native.removeNode(first, placeholder);
      assert.throws(() => native.skeletonPresentation(first, 3, placeholder), /stale handle/);
    } finally {
      native.dispose(first);
      native.dispose(second);
    }
  });
}
