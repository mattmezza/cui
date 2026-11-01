/* SPDX-License-Identifier: MIT */
// TEST ONLY: execute the exact linked probe, never a constant-owner replacement.
const fs = require('node:fs');
const assert = require('node:assert/strict');
const {WASI} = require('node:wasi');
(async () => {
  const file = process.argv[2];
  if (!file) throw new Error('expected actual linked probe.wasm');
  const module = await WebAssembly.compile(fs.readFileSync(file));
  const wasi = new WASI({version: 'preview1', args: [], env: {}});
  const imports = WebAssembly.Module.imports(module);
  const unsupportedShared = process.argv[3] === '--unsupported-shared';
  assert(imports.every(i => i.module === 'wasi_snapshot_preview1' ||
    unsupportedShared && i.module === 'env' && i.name === 'memory'), imports);
  const suppliedMemory = unsupportedShared ? new WebAssembly.Memory({initial:257, maximum:257, shared:true}) : undefined;
  const instance = await WebAssembly.instantiate(module, {wasi_snapshot_preview1: wasi.wasiImport,
    env: {memory:suppliedMemory}});
  const e = instance.exports;
  if (unsupportedShared) {
    assert(e.memory.buffer instanceof SharedArrayBuffer);
    assert.equal(e.probe_supported(), 0);
    assert.equal(e.probe_feature_epoch(), 0n);
    console.log(JSON.stringify({status:'passed', imports,
      exports:WebAssembly.Module.exports(module),memoryShared:true,policySupported:0,
      featureEpochRejected:true,limitation:'No raw shared TLS calls or Worker owner enforcement claimed.'},null,2));
    return;
  }
  wasi.initialize(instance);
  assert.equal(e.probe_supported(), 1);
  const owner = e.probe_owner() >>> 0;
  assert.notEqual(owner, 0);
  assert.equal(e.probe_matches(owner), 1);
  assert.equal(e.probe_owner() >>> 0, owner);
  assert.equal(e.probe_matches((owner + 1) >>> 0), 0);
  for (let i = 1n; i <= 4n; ++i) assert.equal(e.probe_epoch(), i);
  e.probe_seed_near_end();
  assert.equal(BigInt.asUintN(64, e.probe_epoch()), (1n << 64n) - 2n);
  assert.equal(BigInt.asUintN(64, e.probe_epoch()), (1n << 64n) - 1n);
  assert.equal(e.probe_epoch(), 0n);
  const memory = e.memory;
  assert(memory instanceof WebAssembly.Memory);
  const shared = memory.buffer instanceof SharedArrayBuffer;
  console.log(JSON.stringify({status: 'passed', imports,
    exports: WebAssembly.Module.exports(module), memoryShared: shared,
    policySupported: e.probe_supported(), ownerStable: true, wrongOwnerRejected: true, saturatingEpoch: true,
    limitation: 'This run proves one instance; no multi-thread TLS initialization claim.'}, null, 2));
})().catch(error => { console.error(error); process.exitCode = 1; });
