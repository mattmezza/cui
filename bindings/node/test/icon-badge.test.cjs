// SPDX-License-Identifier: MIT
'use strict';

const assert = require('node:assert/strict');
const test = require('node:test');
const {App, IconError, IconSource, badge, button, collapsible, column, configured_badge,
  configured_icon, icon, icon_rotation} = require('../index.cjs');
const native = require(process.env.CUI_NODE_ADDON);

const SVG = '<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16"><path d="M1 7l5 5L15 2" fill="none" stroke="white" stroke-width="3"/></svg>';
const provider = process.env.CUI_ICON_BADGE_PROVIDER ?? 'current';
const COUNT_MAX = ['ia32', 'arm'].includes(process.arch) ? 0xffffffffn : 0xffffffffffffffffn;

function withApp(options, run) {
  const app = new App(options);
  try {
    run(app);
  } finally {
    app.dispose();
  }
}

function click(app, child) {
  const box = child.bounds();
  const x = box.x + box.width / 2, y = box.y + box.height / 2;
  return app.pointer('move', x, y).pointer('down', x, y).pointer('up', x, y).step();
}

test('optional feature probes preserve the existing provider subsets', () => {
  withApp({}, app => {
    const iconAvailable = ['current', 'partial-badge', 'invalid-result', 'invalid-copy'].includes(provider);
    const badgeAvailable = ['current', 'invalid-result', 'invalid-copy'].includes(provider);
    assert.equal(app.supports_icon_resources(), true);
    assert.equal(app.supports_icon_presentation(), iconAvailable);
    assert.equal(app.supports_configured_badge(), badgeAvailable);
    const artwork = app.icon_resource(IconSource.svg(SVG));
    if (!iconAvailable)
      assert.throws(() => app.mount(column().children(configured_icon('invalid', artwork))), /Icon presentation V1/);
    if (!badgeAvailable)
      assert.throws(() => app.mount(column().children(configured_badge('invalid'))), /configured Badge V1/);
    const tree = column().children(icon('catalog', 'check'), badge('legacy', '5'));
    if (provider === 'partial-badge') tree.children(configured_icon('owned', artwork));
    app.mount(tree);
    app.step();
    if (provider === 'partial-badge') assert.equal(artwork.info().references, 1);
  });
});

if (provider === 'invalid-copy' || provider === 'invalid-result') {
  test('refuse a provider that violates the copied ABI', () => {
    withApp({}, app => {
      const resource = app.icon_resource(IconSource.svg(SVG));
      const glyph = configured_icon('icon', resource);
      const badgeNode = configured_badge('badge', {variant: 'icon', artwork: resource})
        .children(button('child').label('OK'));
      app.mount(column().children(glyph, badgeNode)).step();
      const query = provider === 'invalid-copy' ? 'get_config' : 'result';
      assert.throws(() => glyph[query](), /invalid copied Icon/);
      assert.throws(() => badgeNode[query](), /invalid copied Badge|invalid copied Icon or Badge/);
    });
  });
}

if (provider === 'current') {
  test('copied Icon options preserve explicit zeros, all nested fields, and fallback', () => {
    withApp({}, app => {
      const resource = app.icon_resource(IconSource.svg(SVG));
      const style = {
        width: -1, height: 12, padding: 3, gap: 1, radius: 2, backgroundRgba: 0x112233ff,
        borderRgba: 0x004400ff, borderWidth: 1, alignX: 'center', alignY: 'end', offsetX: -2,
        offsetY: 3, corners: [1, 2, 3, 4], borderEdges: [2, 1, 4, 3], maxWidth: 30,
        maxHeight: 29, opacity: 0, paddingEdges: [4, 0, 2, 3], row: true, clip: true,
        fixedWidth: true, fixedHeight: true, layoutOnly: true, inert: true, widthPercent: true,
        heightPercent: true,
      };
      const rotation = [1, 0, 0, 1, 2, -3];
      const options = {
        size: 'custom', customSize: 24, designSize: '2xl', inheritedTextSize: 15,
        width: 0, height: 22, remSize: 16, opacity: 0, colorRgba: 0, inheritedColorRgba: 0,
        transform: rotation, style, constraints: {minWidth: 11, minHeight: 13, maxWidth: 0, maxHeight: 24},
        decorative: true, debugPlaceholder: true,
      };
      const node = configured_icon('retained', resource, options, {name: 'Inbox ✓', fallback: 'check'});
      const draft = node.get_config();
      assert.equal(draft.resource, resource);
      assert.deepEqual(draft.options, options);
      assert.ok(Object.isFrozen(draft.options.style.corners));
      rotation[4] = 200;
      style.paddingEdges[0] = 99;
      app.mount(column().children(node));
      assert.equal(node.result(), undefined);
      assert.deepEqual(node.get_config(), draft);
      assert.equal(draft.options.transform[4], 2);
      assert.equal(draft.options.style.paddingEdges[0], 4);
      app.step();
      assert.equal(node.result().iconError, IconError.OK);
      assert.equal(resource.info().references, 1);
      assert.throws(() => resource.close());
      assert.throws(() => node.configure(resource, {width: -1}), /width/);
      assert.deepEqual(node.get_config(), draft);
      node.configure(resource, {width: 24, height: 24, transform: icon_rotation(Math.PI / 8)});
      assert.equal(node.result(), undefined);
      app.step();
      assert.equal(node.result().iconError, IconError.OK);
      node.reset();
      assert.equal(node.get_config(), undefined);
      assert.equal(node.result(), undefined);
      resource.close();
      app.step();
      assert.ok(node.bounds().width > 0);
    });
  });

  test('Badge composes children without layout shift, batches no events and updates in callbacks', () => {
    withApp({width: 520, height: 300}, app => {
      const resource = app.icon_resource(IconSource.svg(SVG));
      const art = configured_icon('image', resource, {width: 20, height: 20});
      const first = configured_badge('unread', {count: COUNT_MAX, maximum: 0n,
        colorRgba: 0, size: 'xsmall', designSize: 'base', name: 'Unread'});
      const flags = configured_badge('flags', {variant: 'icon', name: 'Synced',
        artwork: resource, artworkOptions: {width: 12, height: 12, opacity: 0}});
      const dot = configured_badge('dot', {variant: 'dot', size: 'large'}).children(button('settings').label('Settings'));
      let deliveries = 0;
      const child = button('ack').label('Clear unread').primary().on_click(() => {
        ++deliveries;
        first.configure({...first.get_config(), count: 0n});
      });
      first.children(flags.children(child));
      assert.equal(first.get_config().maximum, 0n);
      assert.equal(first.get_config().colorRgba, 0);
      app.mount(column().gap(10).children(art, first, dot));
      assert.equal(first.result(), undefined);
      app.step();
      assert.equal(first.get_config().count, COUNT_MAX);
      const bounds = child.bounds();
      assert.ok(first.indicator_bounds().width > 0);
      assert.ok(dot.indicator_bounds().height > 0);
      assert.deepEqual(flags.get_config().artworkOptions.width, 12);
      assert.equal(flags.result().iconError, IconError.OK);
      assert.equal(resource.info().references, 2);
      assert.equal(click(app, child), 1);
      assert.equal(deliveries, 1);
      app.step();
      assert.equal(first.indicator_bounds(), undefined);
      assert.deepEqual(child.bounds(), bounds);
      assert.equal(first.result().iconError, null);
      assert.equal(first.result().submitted, true);
      assert.equal(deliveries, 1);

      first.disabled();
      app.step();
      assert.equal(click(app, child), 0);
      assert.equal(deliveries, 1);
      first.disabled(false);
      app.step();
      child.on_click(() => { first.remove(); ++deliveries; });
      assert.equal(click(app, child), 1);
      assert.equal(deliveries, 2);
      assert.throws(() => flags.result(), /removed/);
      assert.equal(resource.info().references, 1);
      art.remove();
      assert.equal(resource.info().references, 0);
      resource.close();
    });
  });

  test('an artwork failure keeps content and omission clears subsequent frame evidence', () => {
    withApp({}, app => {
      const resource = app.icon_resource(IconSource.path('missing.svg'));
      const child = button('keep').label('Keep');
      const broken = configured_badge('broken', {variant: 'icon', artwork: resource}).children(child);
      const named = configured_badge('named', {variant: 'icon', namedIcon: 'not-in-the-catalog'});
      const panel = collapsible('panel', 'Details').open().children(broken, named);
      app.mount(column().children(panel));
      app.step();
      assert.equal(broken.result().submitted, true);
      assert.equal(broken.result().iconError, IconError.NOT_FOUND);
      assert.equal(broken.result().frameStatus, 0);
      assert.ok(child.bounds().width > 0);
      assert.equal(named.result().iconError, null);
      panel.open(false);
      app.step();
      assert.equal(broken.result(), undefined);
      assert.equal(broken.indicator_bounds(), undefined);
      resource.put_asset('missing.svg', SVG);
      named.configure({variant: 'icon', namedIcon: 'check'});
      panel.open();
      app.step();
      assert.equal(broken.result().iconError, IconError.OK);
      assert.ok(named.indicator_bounds().width > 0);
    });
  });

  test('foreign owners and invalid synchronous updates leave current config intact', () => {
    withApp({}, owner => withApp({}, consumer => {
      const foreign = owner.icon_resource(IconSource.svg(SVG));
      const own = consumer.icon_resource(IconSource.svg(SVG));
      const tree = column().children(configured_icon('own', own), configured_badge('foreign', {
        variant: 'icon', artwork: foreign,
      }));
      assert.throws(() => consumer.mount(tree), /different App/);
      const badgeNode = configured_badge('show', {variant: 'icon', artwork: own});
      consumer.mount(column().children(badgeNode)).step();
      assert.throws(() => badgeNode.configure({variant: 'icon', artwork: foreign}), /different App/);
      assert.throws(() => badgeNode.configure({count: Number.MAX_SAFE_INTEGER + 1}), /safe integer/);
      assert.throws(() => badgeNode.configure({maximum: 1n << 64n}), RangeError);
      assert.throws(() => badgeNode.configure({variant: 'icon', artwork: own, namedIcon: 'check'}));
      assert.throws(() => badgeNode.configure({name: 'invalid\ud803'}), /surrogate/);
      assert.equal(badgeNode.get_config().artwork, own);
      assert.equal(badgeNode.result().iconError, IconError.OK);
      assert.equal(own.info().references, 1);
      assert.equal(foreign.info().references, 0);

      // The low-level entry point must reject a foreign resource even if a JS
      // caller bypasses builder preflight.
      const handleKey = Object.getOwnPropertySymbols(badgeNode).find(key => key.description === 'CUI node');
      const childHandle = badgeNode[handleKey];
      assert.ok(childHandle);
      const foreignHandle = native.iconResource(owner.handle, 6, 0, 0, 0, 0);
      assert.throws(() => native.configuredBadge(consumer.handle, 3, childHandle, foreignHandle), /another app/);
      native.iconResource(owner.handle, 7, foreignHandle);
    }));
  });
}
