// SPDX-License-Identifier: MIT
'use strict';
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const {spawnSync} = require('node:child_process');
if (Number(process.versions.napi) < 8) throw new Error('CUI alpha addon requires Node-API version 8 or newer');
if (process.platform !== 'linux') throw new Error('Alpha source package currently validates Linux only');
const root = path.resolve(__dirname, '..');
const source = path.join(root, '_source', 'bindings', 'node');
if (!fs.existsSync(path.join(source, 'CMakeLists.txt'))) throw new Error('Packaged CUI source tree is missing');
if (process.env.CUI_PACKAGE_NATIVE === '1') {
  const check = spawnSync('pkg-config', ['--exists','sdl3 >= 3.2','freetype2','harfbuzz']);
  if (check.error || check.status !== 0) throw new Error('Native package requires system SDL3 >=3.2, FreeType and HarfBuzz development packages');
}
const build = fs.mkdtempSync(path.join(os.tmpdir(), 'cui-npm-build-'));
function run(args) {
  const result = spawnSync('cmake', args, {stdio: 'inherit'});
  if (result.error) throw result.error;
  if (result.status !== 0) throw new Error(`CUI package CMake failed (${result.status})`);
}
try {
  run(['-S', source, '-B', build, '-DCUI_BUILD_NATIVE=' + (process.env.CUI_PACKAGE_NATIVE === '1' ? 'ON' : 'OFF'),
    '-DCMAKE_BUILD_TYPE=' + (process.env.CUI_PACKAGE_BUILD_TYPE || 'Release'), '-DCMAKE_BUILD_RPATH_USE_ORIGIN=ON', '-DCMAKE_BUILD_RPATH=$ORIGIN', '-DCMAKE_C_FLAGS=-DNAPI_VERSION=8']);
  run(['--build', build, '--target', 'cui_node_bindings', '-j2']);
  const output = path.join(root, 'native');
  fs.mkdirSync(output, {recursive: true});
  fs.copyFileSync(path.join(build, 'cui.node'), path.join(output, 'cui.node'));
  fs.copyFileSync(path.join(build, 'cui-bridge-build', 'libcui_bridge.so'), path.join(output, 'libcui_bridge.so'));
  const dependencies = spawnSync('ldd',[path.join(output,'libcui_bridge.so')],{encoding:'utf8'});
  if (dependencies.status !== 0) throw new Error('Cannot inspect built Linux runtime dependencies');
  fs.writeFileSync(path.join(output,'build-info.json'),JSON.stringify({alpha:true,
    native_requested:process.env.CUI_PACKAGE_NATIVE === '1', platform:process.platform,arch:process.arch,
    system_development_versions:process.env.CUI_PACKAGE_NATIVE === '1'
      ? spawnSync('pkg-config',['--modversion','sdl3','freetype2','harfbuzz'],{encoding:'utf8'}).stdout.trim().split('\n') : [],
    source_revision:fs.readFileSync(path.join(root,'_source','SOURCE_REVISION'),'utf8').trim(),
    system_runtime_dependencies:dependencies.stdout.trim().split('\n').map(line=>line.trim().split(/\s+/)[0])},null,2));
} finally {
  fs.rmSync(build, {recursive: true, force: true});
}
