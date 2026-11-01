#!/usr/bin/env python3
"""Opt-in Linux dynamic TimeField acceptance; existing tools/libraries only.

Invoke through scripts/test_local.py/CTest. No native desktop, engine build,
SDK generation, dependency installation or fabricated provider is used.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]


def parser():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--language', choices=('go', 'rust'), required=True)
    p.add_argument('--build-dir', type=Path, required=True)
    p.add_argument('--library', type=Path, required=True)
    p.add_argument('--old-library', type=Path, required=True)
    p.add_argument('--partial-library', type=Path, required=True)
    p.add_argument('--target-dir', type=Path)
    p.add_argument('--go', default='go')
    p.add_argument('--cargo', default='cargo')
    return p


def validate(a, environ=None, platform=None):
    environ = os.environ if environ is None else environ
    platform = sys.platform if platform is None else platform
    if platform != 'linux':
        raise ValueError('this dynamic provider acceptance is validated on Linux only')
    if environ.get('CUI_TEST_VIRTUAL_DISPLAY') != '1':
        raise ValueError('run through scripts/test_local.py private display wrapper')
    a.build_dir = a.build_dir.resolve()
    if not a.build_dir.is_dir():
        raise ValueError('reuse an existing build directory')
    for name in ('library', 'old_library', 'partial_library'):
        value = getattr(a, name).resolve()
        if not value.is_file():
            raise ValueError('actual existing provider required: ' + name)
        setattr(a, name, value)
    if len({a.library, a.old_library, a.partial_library}) != 3:
        raise ValueError('current, actual-old and partial providers must be distinct')
    if a.language == 'rust':
        if a.target_dir is None or not a.target_dir.resolve().is_dir():
            raise ValueError('Rust requires an existing reusable --target-dir')
        a.target_dir = a.target_dir.resolve()
    return a


def environment(a, inherited=None):
    env = dict(os.environ if inherited is None else inherited)
    for name in ('LD_PRELOAD', 'DYLD_INSERT_LIBRARIES', 'CUI_TEST_TIME_PROVIDER',
                 'CUI_BRIDGE_LIBRARY', 'CUI_BRIDGE_LIB_DIR', 'RUSTFLAGS',
                 'CARGO_ENCODED_RUSTFLAGS', 'GOFLAGS'):
        env.pop(name, None)
    env.update(GOMAXPROCS='1', GOFLAGS='-p=1', GOPROXY='off', GOSUMDB='off',
               GOTOOLCHAIN='local', CARGO_BUILD_JOBS='1', CARGO_NET_OFFLINE='true',
               CUI_BRIDGE_LIB_DIR=str(a.library.parent),
               CUI_BRIDGE_LIBRARY=str(a.library), LD_LIBRARY_PATH=str(a.library.parent))
    if a.target_dir is not None:
        env['CARGO_TARGET_DIR'] = str(a.target_dir)
    return env


def invoke(command, cwd, env, capture=False):
    command = [str(x) for x in command]
    print('RUN', json.dumps(command), flush=True)
    return subprocess.run(command, cwd=cwd, env=env, check=True, text=True,
                          stdout=subprocess.PIPE if capture else None)


def artifact(stdout, target_dir):
    found = set()
    for line in stdout.splitlines():
        try:
            item = json.loads(line)
        except json.JSONDecodeError:
            print(line, flush=True)
            continue
        if item.get('reason') == 'compiler-message':
            print(item.get('message', {}).get('rendered', ''), end='', flush=True)
        if (item.get('reason') == 'compiler-artifact'
                and item.get('target', {}).get('name') == 'time_localized'
                and 'test' in item.get('target', {}).get('kind', [])
                and item.get('executable')):
            path = Path(item['executable']).resolve()
            if not path.is_relative_to(target_dir.resolve()):
                raise ValueError('Cargo executable escaped the requested reused cache')
            found.add(path)
    if len(found) != 1:
        raise ValueError('Cargo must report exactly one time_localized test executable')
    path = found.pop()
    if not path.is_file():
        raise ValueError('reported executable is missing')
    return path


def provider_dir(a, name, provider):
    # Rust links libcui_bridge.so; the actual partial DSO has a different filename.
    # A tiny symlink directory selects that exact artifact, never a copied engine.
    out = a.build_dir / 'time-localized-providers' / name
    out.mkdir(parents=True, exist_ok=True)
    link = out / 'libcui_bridge.so'
    if link.is_symlink():
        if link.resolve() != provider:
            raise ValueError('provider symlink already selects another artifact')
    elif link.exists():
        raise ValueError('refusing to replace an existing provider file')
    else:
        link.symlink_to(provider)
    return out


def run_suite(a, run=invoke):
    env = environment(a)
    if a.language == 'go':
        cwd = ROOT / 'bindings/go'
        base = [a.go, 'test', '-tags', 'cui_dynamic', './cui', '-count=1', '-v']
        run(base + ['-run', '^TestLocalizedTime'], cwd, env)
        for name, provider in (('old', a.old_library), ('partial', a.partial_library)):
            other = dict(env, CUI_TEST_TIME_PROVIDER=name, CUI_BRIDGE_LIBRARY=str(provider))
            run(base + ['-run', '^TestLocalizedTimeOptionalProvider$'], cwd, other)
        run([a.go, 'run', '-tags', 'cui_dynamic', './examples/time_field_localized'], cwd, env)
        return
    cwd = ROOT / 'bindings/rust'
    base = [a.cargo, 'test', '--offline', '--locked', '--no-default-features', '-j1']
    compiled = run(base + ['--test', 'time_localized', '--no-run', '--message-format=json'],
                   cwd, env, capture=True)
    binary = artifact(compiled.stdout, a.target_dir)
    run([binary, '--test-threads=1', '--nocapture'], cwd, env)
    for name, provider in (('old', a.old_library), ('partial', a.partial_library)):
        other = dict(env, CUI_TEST_TIME_PROVIDER=name,
                     LD_LIBRARY_PATH=str(provider_dir(a, name, provider)))
        run([binary, '--exact', 'optional_provider_preserves_scalar_default', '--nocapture'],
            cwd, other)
    run(base + ['--lib', 'time_localized::', '--', '--test-threads=1'], cwd, env)
    run([a.cargo, 'run', '--offline', '--locked', '--no-default-features', '-j1',
         '--example', 'time_field_localized'], cwd, env)


def main():
    p = parser()
    a = p.parse_args()
    try:
        validate(a)
    except ValueError as error:
        p.error(str(error))
    run_suite(a)


if __name__ == '__main__':
    main()
