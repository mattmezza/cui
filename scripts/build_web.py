#!/usr/bin/env python3
"""Build the static browser gallery with pinned Zig 0.16.0. No Node build toolchain."""
import argparse
import json
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ['src/core/cui.c', 'src/base/cui_base.c', 'src/base/cui_models.c',
           'src/base/cui_overlay.c', 'src/base/cui_virtual.c', 'src/base/cui_dock.c', 'src/base/cui_plot.c', 'src/base/cui_edit.c',
           'src/base/cui_numeric.c', 'src/components/cui_numeric.c',
           'src/base/cui_content.c', 'src/components/cui_content.c',
           'src/base/cui_disclosure.c', 'src/components/cui_disclosure.c',
           'src/base/cui_choice.c', 'src/components/cui_choice.c',
           'src/theme/cui_theme.c', 'src/components/cui_components.c', 'web/gallery.c']
EXPORTS = ['gallery_init', 'gallery_event', 'gallery_text', 'gallery_focus', 'gallery_frame',
           'gallery_modal', 'gallery_time', 'gallery_redraw', 'gallery_count', 'gallery_command', 'gallery_command_text', 'gallery_command_length',
           'gallery_semantic_count', 'gallery_semantic', 'gallery_semantic_name',
           'gallery_semantic_length', 'gallery_clicks', 'malloc', 'free']

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', type=Path, default=ROOT / 'build/web')
    p.add_argument('--zig', default='zig')
    args = p.parse_args()
    version = subprocess.check_output([args.zig, 'version'], text=True).strip()
    if version != '0.16.0':
        raise RuntimeError(f'Expected Zig 0.16.0, found {version}')
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    subprocess.run([args.zig, 'cc', '-target', 'wasm32-wasi', '-mexec-model=reactor',
        '-std=c11', '-O2', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
        '-I', str(ROOT / 'include'), '-I', str(ROOT / 'third_party/clay'),
        *[str(ROOT / s) for s in SOURCES], '-lm', '-Wl,-z,stack-size=8388608',
        *[f'-Wl,--export={s}' for s in EXPORTS], '-o', str(out / 'gallery.wasm')], check=True)
    for name in ['index.html', 'gallery.js', 'gallery.css']:
        shutil.copyfile(ROOT / 'web' / name, out / name)
    manifest = json.loads((ROOT / 'docs/gpui-parity.json').read_text())
    (out / 'catalog.json').write_text(json.dumps(manifest, indent=2)+'\n')
    (out / '.nojekyll').touch()
    print(f'Gallery built at {out}; serve over HTTP, e.g. python -m http.server -d {out}')

if __name__ == '__main__':
    main()
