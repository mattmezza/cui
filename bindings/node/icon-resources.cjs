// SPDX-License-Identifier: MIT
'use strict';

const {TextDecoder} = require('node:util');
const UTF8 = new TextDecoder('utf-8', {fatal: true, ignoreBOM: true});
const TEXT_MAX = 4096;
const SVG_MAX = 1 << 20;
const IDENTITY = Object.freeze([1, 0, 0, 1, 0, 0]);
const SOURCE_KINDS = ['empty', 'name', 'path', 'data'];
const IconError = Object.freeze({OK: 0, INVALID: 1, NOT_FOUND: 2, PARSE: 3, LIMIT: 4, ALLOCATION: 5});
const IconAvailability = Object.freeze({UNKNOWN: 0, CATALOG_NOT_EMBEDDED: 1, EMBEDDED: 2});
const OP = Object.freeze({
  PROBE: 0, DEFAULT_LIMITS: 1, CATALOG_COUNT: 2, CATALOG_ENTRY: 3, QUERY: 4, COPY_ASSET: 5,
  CREATE: 6, DESTROY: 7, INFO: 8, GET_LIMITS: 9, SET_LIMITS: 10, GET_SOURCE: 11, SET_SOURCE: 12,
  PUT_ASSET: 13, REMOVE_ASSET: 14, CLEAR_ASSETS: 15, RELOAD: 16, COLLECT: 17, PREPARE: 18, BITMAP: 19,
});

function textBytes(value, maximum, allowBytes = false) {
  if (allowBytes && value instanceof Uint8Array) {
    if (value.byteLength > maximum) throw new RangeError('Icon data exceeds byte limit');
    const owned = Buffer.from(value);
    if (owned.includes(0)) throw new TypeError('Icon text cannot contain NUL');
    UTF8.decode(owned);
    return owned;
  }
  if (typeof value !== 'string') throw new TypeError('expected Icon text');
  if (value.length > maximum || Buffer.byteLength(value) > maximum)
    throw new RangeError('Icon text exceeds UTF-8 byte limit');
  if (value.includes('\0')) throw new TypeError('Icon text cannot contain NUL');
  for (const character of value) {
    const code = character.codePointAt(0);
    if (code >= 0xd800 && code <= 0xdfff) throw new TypeError('unpaired UTF-16 surrogate');
  }
  return Buffer.from(value);
}

function sourceInput(kind, text) {
  const tag = SOURCE_KINDS.indexOf(kind);
  if (tag < 0 || (kind === 'empty' && text !== '')) throw new TypeError('invalid Icon source');
  const bytes = textBytes(text, kind === 'data' ? SVG_MAX : TEXT_MAX, kind === 'data');
  return {tag, bytes};
}

function sourceValue(kind, value) {
  return Object.freeze({kind, text: sourceInput(kind, value).bytes.toString('utf8')});
}

const IconSource = Object.freeze({
  empty: () => sourceValue('empty', ''),
  named: name => sourceValue('name', name),
  path: path => sourceValue('path', path),
  svg: svg => sourceValue('data', svg),
});

function uint(value, max, name) {
  if (!Number.isInteger(value) || value < 0 || value > max)
    throw new RangeError(`${name} must be an integer in [0, ${max}]`);
  return value;
}

function limitsInput(limits) {
  if (!limits || typeof limits !== 'object' || Array.isArray(limits))
    throw new TypeError('expected Icon limits');
  const {maxSvgBytes = 0, maxPixels = 0, maxCacheBytes = 0, maxVersions = 0} = limits;
  return [uint(maxSvgBytes, SVG_MAX, 'maxSvgBytes'), uint(maxPixels, 16 << 20, 'maxPixels'),
    uint(maxCacheBytes, 64 << 20, 'maxCacheBytes'), uint(maxVersions, 128, 'maxVersions')];
}

function bridgeFloat(value, min, max, name) {
  const rounded = Math.fround(typeof value === 'number' ? value : NaN);
  if (!Number.isFinite(rounded) || (value !== 0 && rounded === 0) || value < min || value > max)
    throw new RangeError(`invalid bridge float for ${name}`);
  return rounded;
}

function transformInput(transform) {
  if (!Array.isArray(transform) || transform.length !== 6)
    throw new TypeError('Icon transform requires six coefficients');
  return Array.from(transform, (value, index) =>
    bridgeFloat(value, -Infinity, Infinity, `transform[${index}]`));
}

module.exports = function iconResources(native) {
  const states = new WeakMap();

  function appAlive(app) {
    app._alive();
    if (app.closed) throw new Error('app is closed');
  }

  function supports(app) {
    appAlive(app);
    return typeof native.iconResource === 'function' && native.iconResource(app.handle, OP.PROBE);
  }

  function call(app, op, ...values) {
    appAlive(app);
    if (typeof native.iconResource !== 'function')
      throw new Error('owned Icon V1 requires a complete, compatible bridge symbol group');
    return native.iconResource(app.handle, op, ...values);
  }

  function liveHandle(resource, app) {
    const state = states.get(resource);
    if (!state || state.app !== app) throw new TypeError('Icon resource belongs to a different App');
    appAlive(app);
    if (!state.handle) throw new Error('Icon resource is closed');
    return state.handle;
  }

  function resourceCall(resource, method, ...args) {
    const state = states.get(resource);
    if (!state) throw new TypeError('expected an IconResource');
    return call(state.app, method, liveHandle(resource, state.app), ...args);
  }

  class IconResource {
    constructor(app, source = IconSource.empty(), limits = {}) {
      const {tag, bytes} = sourceInput(source.kind, source.text);
      const handle = call(app, OP.CREATE, ...limitsInput(limits));
      states.set(this, {app, handle});
      if (tag !== 0) {
        try {
          resourceCall(this, OP.SET_SOURCE, tag, bytes);
        } catch (error) {
          this.close();
          throw error;
        }
      }
    }

    set_source(source) {
      const {tag, bytes} = sourceInput(source.kind, source.text);
      resourceCall(this, OP.SET_SOURCE, tag, bytes);
      return this;
    }

    source() {
      const {kind, text} = resourceCall(this, OP.GET_SOURCE);
      return Object.freeze({kind: SOURCE_KINDS[kind], text});
    }

    info() {
      const {sourceKind, ...info} = resourceCall(this, OP.INFO);
      return Object.freeze({...info, sourceKind: SOURCE_KINDS[sourceKind]});
    }

    limits() { return Object.freeze(resourceCall(this, OP.GET_LIMITS)); }

    set_limits(limits) {
      resourceCall(this, OP.SET_LIMITS, ...limitsInput(limits));
      return this;
    }

    put_asset(path, svg) {
      resourceCall(this, OP.PUT_ASSET, textBytes(path, TEXT_MAX), textBytes(svg, SVG_MAX, true));
      return this;
    }

    remove_asset(path) {
      resourceCall(this, OP.REMOVE_ASSET, textBytes(path, TEXT_MAX));
      return this;
    }

    clear_assets() { resourceCall(this, OP.CLEAR_ASSETS); return this; }
    reload() { resourceCall(this, OP.RELOAD); return this; }
    collect() { resourceCall(this, OP.COLLECT); return this; }

    prepare(width, height, transform = IDENTITY) {
      return resourceCall(this, OP.PREPARE, bridgeFloat(width, 0, 4096, 'width'),
        bridgeFloat(height, 0, 4096, 'height'), ...transformInput(transform));
    }

    bitmap() {
      const bitmap = resourceCall(this, OP.BITMAP);
      return Object.freeze({...bitmap, paint: Object.freeze(bitmap.paint)});
    }

    close() {
      const state = states.get(this);
      if (!state.handle) return;
      if (!state.app.closed && !state.app.disposed) resourceCall(this, OP.DESTROY);
      state.handle = null;
    }
  }

  return {
    exports: {IconResource, IconSource, IconError, IconAvailability},
    supports, appAlive, liveHandle, IconResource, textBytes, bridgeFloat, uint,
    defaultLimits: app => Object.freeze(call(app, OP.DEFAULT_LIMITS)),
    catalogCount: app => call(app, OP.CATALOG_COUNT),
    catalogEntry: (app, i) => Object.freeze(call(app, OP.CATALOG_ENTRY, uint(i, 0xffffffff, 'catalog index'))),
    query: (app, name) => call(app, OP.QUERY, textBytes(name, TEXT_MAX)),
    copyAsset: (app, name) => call(app, OP.COPY_ASSET, textBytes(name, TEXT_MAX)),
  };
};
