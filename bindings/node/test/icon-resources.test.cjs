// SPDX-License-Identifier: MIT
'use strict';

const assert = require('node:assert/strict');
const test = require('node:test');
const {App, IconAvailability, IconError, IconSource, badge, column, icon} = require('../index.cjs');
const native = require(process.env.CUI_NODE_ADDON);
const SQUARE = '<svg xmlns="http://www.w3.org/2000/svg" width="12" height="12"><path fill="red" d="M1 1h10v10H1z"/></svg>';
const CORNER = SQUARE.replace('M1 1h10v10H1z', 'M1 1h4v4H1z');

test('the Icon resource capability is independent of legacy widget construction', () => {
  const app = new App();
  try {
    const supported = process.env.CUI_ICON_UNSUPPORTED !== '1';
    assert.equal(app.supports_icon_resources(), supported);
    app.mount(column().children(icon('legacy', 'check'), badge('info', '8')));
    assert.equal(app.step(), 0);
    if (!supported) assert.throws(() => app.icon_resource(), /complete, compatible bridge/);
  } finally {
    app.dispose();
  }
});

if (process.env.CUI_ICON_UNSUPPORTED !== '1') {
  test('catalog, defaults and zero-normalized limits come from the loaded C provider', () => {
    const app = new App();
    try {
      assert.deepEqual(app.default_icon_limits(), {
        maxSvgBytes: 256 << 10, maxPixels: 1 << 20, maxCacheBytes: 8 << 20, maxVersions: 16,
      });
      assert.ok(app.icon_catalog_count() >= 123);
      const entry = app.icon_catalog_entry(0);
      assert.ok(entry.name && entry.path);
      assert.equal(typeof entry.defaultEmbedded, 'boolean');
      assert.equal(app.query_icon('check'), IconAvailability.EMBEDDED);
      assert.equal(app.query_icon('outside-catalog-in-cui-tests'), IconAvailability.UNKNOWN);
      assert.ok(app.copy_icon_asset('check').includes('<svg'));
      assert.equal(app.copy_icon_asset('outside-catalog-in-cui-tests'), undefined);
      const resource = app.icon_resource(IconSource.named('check'), {maxVersions: 2});
      assert.deepEqual(resource.limits(), {...app.default_icon_limits(), maxVersions: 2});
      assert.deepEqual(resource.source(), IconSource.named('check'));
      assert.equal(resource.prepare(16, 16), IconError.OK);
      assert.equal(resource.info().parseCount, 1n);
      resource.close();
      resource.close();
    } finally {
      app.dispose();
    }
  });

  test('owned asset map, bitmap copy, cache collection and failure preserve C semantics', () => {
    const app = new App();
    try {
      const resource = app.icon_resource();
      assert.deepEqual(resource.source(), IconSource.empty());
      assert.deepEqual(resource.bitmap(), {
        width: 0, height: 0, stride: 0, paint: {x: 0, y: 0, width: 0, height: 0},
        revision: 0n, rgba: Buffer.alloc(0),
      });
      resource.set_source(IconSource.path('art/tile'));
      assert.equal(resource.prepare(12, 12), IconError.NOT_FOUND);
      const data = Buffer.from(SQUARE);
      resource.put_asset('art/tile', data);
      data.fill(0);
      assert.equal(resource.prepare(12, 12), IconError.OK);
      const square = resource.bitmap();
      assert.equal(square.width, 12);
      assert.equal(square.stride, 48);
      assert.equal(square.rgba.length, 576);
      const squarePixels = Buffer.from(square.rgba);
      square.rgba.fill(0);
      assert.deepEqual(resource.bitmap().rgba, squarePixels);
      assert.ok(squarePixels.some((byte, index) => index % 4 === 3 && byte !== 0));

      const before = resource.info();
      resource.put_asset('art/tile', CORNER).reload();
      assert.equal(resource.prepare(12, 12, [1, 0, 0, 1, 0, 0]), IconError.OK);
      const corner = resource.bitmap();
      const innerAlpha = (y, x) => (y * square.width + x) * 4 + 3;
      assert.ok(squarePixels[innerAlpha(8, 8)] > 0);
      assert.equal(corner.rgba[innerAlpha(8, 8)], 0);
      assert.ok(corner.rgba[innerAlpha(3, 3)] > 0);
      assert.ok(resource.info().sourceRevision > before.sourceRevision);
      assert.ok(resource.info().cacheVersions > 0n);
      resource.collect();
      assert.equal(resource.info().cacheVersions, 1n);

      resource.put_asset('art/tile', '<broken');
      assert.equal(resource.prepare(12, 12), IconError.PARSE);
      assert.deepEqual(resource.bitmap().rgba, corner.rgba);
      resource.remove_asset('art/tile');
      assert.equal(resource.prepare(12, 12), IconError.NOT_FOUND);
      resource.put_asset('art/tile', SQUARE).clear_assets();
      assert.equal(resource.prepare(12, 12), IconError.NOT_FOUND);
      resource.set_source(IconSource.svg(SQUARE)).set_limits({maxPixels: 64});
      assert.equal(resource.prepare(12, 12), IconError.LIMIT);
      assert.deepEqual(resource.source(), IconSource.svg(SQUARE));
    } finally {
      app.dispose();
    }
  });

  test('reject malformed inputs before changing the current source', () => {
    const app = new App();
    try {
      const resource = app.icon_resource(IconSource.svg(SQUARE));
      assert.throws(() => resource.set_source({kind: 'unknown', text: SQUARE}), TypeError);
      assert.throws(() => IconSource.svg(Uint8Array.of(0xc0, 0xaf)), TypeError);
      assert.throws(() => IconSource.path('invalid\0path'), /NUL/);
      assert.throws(() => IconSource.named('\ud801'), /surrogate/);
      assert.throws(() => IconSource.named('é'.repeat(2049)), RangeError);
      assert.throws(() => resource.set_limits({maxVersions: 129}), RangeError);
      assert.throws(() => resource.prepare(1, 1, [1, 0, 0, 1, , 0]), RangeError);
      assert.throws(() => resource.prepare(1e-50, 2), RangeError);
      assert.deepEqual(resource.source(), IconSource.svg(SQUARE));
      for (let attempt = 0; attempt < 130; attempt++)
        assert.throws(() => app.icon_resource(IconSource.svg(SQUARE), {maxSvgBytes: 1}));
      const next = app.icon_resource(IconSource.svg(CORNER));
      assert.equal(next.prepare(12, 12), IconError.OK);
    } finally {
      app.dispose();
    }
  });

  test('native externals reject foreign, closed and forged handles', () => {
    const app = new App();
    const other = new App();
    try {
      const handle = native.iconResource(app.handle, 6, 0, 0, 0, 0);
      assert.throws(() => native.iconResource(other.handle, 8, handle), /another app/);
      assert.throws(() => native.iconResource(app.handle, 8, app.handle), /another app/);
      native.iconResource(app.handle, 7, handle);
      assert.throws(() => native.iconResource(app.handle, 8, handle), /closed/);
      const resource = app.icon_resource();
      app.close();
      assert.throws(() => resource.info(), /closed/);
      resource.close();
      resource.close();
    } finally {
      app.dispose();
      other.dispose();
    }
  });
}
