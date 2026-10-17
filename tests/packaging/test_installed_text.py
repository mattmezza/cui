#!/usr/bin/env python3
"""Compare wheel/npm bundled native text with canonical text without opening a display."""
import argparse
import base64
import csv
import hashlib
import io
import json
import os
from pathlib import Path, PurePosixPath
import shlex
import stat
import subprocess
import tarfile
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]


def run(command, env):
    subprocess.run([str(item) for item in command], cwd=ROOT, env=env, check=True)


def extract(artifact, destination):
    """Accept only ordinary files/directories; never follow archive links."""
    seen = set()

    def target(name):
        path = PurePosixPath(name)
        if path.is_absolute() or '..' in path.parts or '\\' in name:
            raise ValueError(f'Unsafe archive path: {name}')
        result = destination.joinpath(*path.parts)
        if result in seen:
            raise ValueError(f'Duplicate archive path: {name}')
        seen.add(result)
        return result

    if artifact.suffix == '.whl':
        with zipfile.ZipFile(artifact) as archive:
            for member in archive.infolist():
                mode = member.external_attr >> 16
                if stat.S_IFMT(mode) not in (0, stat.S_IFREG, stat.S_IFDIR):
                    raise ValueError('Archive special files and links are forbidden')
                path = target(member.filename)
                if member.is_dir():
                    path.mkdir(parents=True, exist_ok=True)
                else:
                    path.parent.mkdir(parents=True, exist_ok=True)
                    path.write_bytes(archive.read(member))
    else:
        with tarfile.open(artifact, 'r:gz') as archive:
            for member in archive:
                if not (member.isfile() or member.isdir()):
                    raise ValueError('Archive special files and links are forbidden')
                path = target(member.name)
                if member.isdir():
                    path.mkdir(parents=True, exist_ok=True)
                else:
                    path.parent.mkdir(parents=True, exist_ok=True)
                    with archive.extractfile(member) as source:
                        path.write_bytes(source.read())


def check_metadata(package, library):
    digest = hashlib.sha256(library.read_bytes()).hexdigest()
    info_path = library.with_name('build-info.json')
    if info_path.exists():
        info = json.loads(info_path.read_text())
        if info.get('native_requested') is False:
            raise RuntimeError('Package metadata identifies a non-native bridge')
        expected = info.get('library_sha256')
        hashes = info.get('file_sha256', {})
        if hashes:
            expected = hashes[library.name]
        if expected is not None and digest != expected:
            raise RuntimeError('Bridge SHA256 differs from build-info.json')
    # Wheels carry the library hash in their mandatory RECORD even when their
    # build-info schema does not provide a dedicated library_sha256 field.
    for record in package.glob('*.dist-info/RECORD'):
        relative = library.relative_to(package).as_posix()
        rows = [row for row in csv.reader(io.StringIO(record.read_text())) if row[0] == relative]
        expected = 'sha256=' + base64.urlsafe_b64encode(bytes.fromhex(digest)).rstrip(b'=').decode()
        if len(rows) != 1 or rows[0][1:] != [expected, str(library.stat().st_size)]:
            raise RuntimeError('Bridge hash/size differs from wheel RECORD')
    print(f'{library.relative_to(package)} SHA256: {digest}', flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('artifacts', 'font', 'rtl-font', 'build-dir'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--reference-build', type=Path,
                        help='Existing canonical native build containing libcui_text.a, libcui.a, libcui_bidi.a')
    args = parser.parse_args()
    font, rtl_font = args.font.resolve(strict=True), args.rtl_font.resolve(strict=True)
    build = args.build_dir.resolve()
    build.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ)
    for key in ('DISPLAY', 'WAYLAND_DISPLAY', 'MIR_SOCKET', 'SDL_VIDEODRIVER', 'XDG_RUNTIME_DIR',
                'LD_LIBRARY_PATH', 'LD_PRELOAD', 'CUI_BRIDGE_LIBRARY', 'CUI_BRIDGE_LIB_DIR'):
        env.pop(key, None)
    env['SDL_VIDEODRIVER'] = 'dummy'
    reference = args.reference_build.resolve() if args.reference_build else build / 'canonical'
    if not args.reference_build:
        run(['cmake', '-S', ROOT, '-B', reference, '-G', 'Ninja', '-DCUI_BUILD_NATIVE=ON',
             '-DCUI_BUILD_BIDI=ON', '-DCMAKE_BUILD_TYPE=Debug'], env)
        run(['cmake', '--build', reference, '--target', 'cui_text', '-j2'], env)
    flags = shlex.split(subprocess.check_output(
        ['pkg-config', '--libs', 'freetype2', 'harfbuzz'], env=env, text=True))
    common = ['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-UNDEBUG']
    fixture = ROOT / 'tests/packaging/sdk_text_fixture.c'
    for label, path in (('primary-font', font), ('rtl-font', rtl_font)):
        print(f'{label} SHA256: {hashlib.sha256(path.read_bytes()).hexdigest()}', flush=True)
    with tempfile.TemporaryDirectory(prefix='installed-text-', dir=build) as temporary:
        work = Path(temporary)
        executable = work / 'canonical'
        run([*common, '-I' + str(ROOT / 'include'), fixture,
             reference / 'libcui_text.a', reference / 'libcui.a', reference / 'libcui_bidi.a',
             *flags, '-lm', '-o', executable], env)
        def probe(binary):
            return subprocess.check_output([str(binary), str(font), str(rtl_font)],
                                           cwd=work, env=env, text=True).strip()
        expected = probe(executable)
        print('canonical: ' + expected, flush=True)
        for kind, pattern, prefix in (('wheel', '*.whl', 'cui_bridge'), ('npm', '*.tgz', 'package')):
            artifacts = sorted(args.artifacts.resolve().glob(pattern))
            if len(artifacts) != 1:
                raise RuntimeError(f'Expected exactly one {kind} artifact, found {len(artifacts)}')
            artifact = artifacts[0]
            print(f'{kind} artifact SHA256: {hashlib.sha256(artifact.read_bytes()).hexdigest()}', flush=True)
            destination = work / kind
            destination.mkdir()
            extract(artifact, destination)
            package = destination / prefix
            library = package / ('_native' if kind == 'wheel' else 'native') / 'libcui_bridge.so'
            headers = package / '_source/include'
            if not library.is_file() or not (headers / 'cui/cui_text.h').is_file():
                raise RuntimeError(f'{kind} lacks bundled native library or text headers')
            check_metadata(destination if kind == 'wheel' else package, library)
            executable = work / (kind + '-fixture')
            run([*common, '-I' + str(headers), fixture, library,
                 '-Wl,-rpath,' + str(library.parent), '-lm', '-o', executable], env)
            actual = probe(executable)
            print(kind + ': ' + actual, flush=True)
            if actual != expected:
                raise RuntimeError(f'{kind} contextual bidi geometry/raster differs from canonical text')
    print('Canonical, wheel and prebuilt npm contextual text fixture PASS', flush=True)


if __name__ == '__main__':
    main()
