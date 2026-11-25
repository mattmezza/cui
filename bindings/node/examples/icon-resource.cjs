// SPDX-License-Identifier: MIT
'use strict';
const assert = require('node:assert/strict');
const {App, IconError, IconSource} = require('../index.cjs');

const svg = '<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24"><path d="M2 12l7 7L22 5" fill="none" stroke="#268056" stroke-width="3"/></svg>';
const app = new App();
try {
  const artwork = app.icon_resource(IconSource.path('workspace/ready.svg'));
  try {
    artwork.put_asset('workspace/ready.svg', svg);
    assert.equal(artwork.prepare(24, 24), IconError.OK);
    const {width, height, rgba} = artwork.bitmap();
    assert.equal(rgba.length, width * height * 4);
    assert.ok(rgba.some((value, index) => index % 4 === 3 && value !== 0));
    console.log(`${width}×${height}, ${rgba.length} bytes of copied RGBA8`);
  } finally {
    artwork.close();
  }
} finally {
  app.dispose();
}
