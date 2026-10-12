#!/usr/bin/env python3
"""Build the static browser gallery with pinned Zig 0.16.0. No Node build toolchain."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ['src/core/cui.c', 'src/base/cui_base.c', 'src/base/cui_models.c',
           'src/base/cui_overlay.c', 'src/base/cui_virtual.c', 'src/base/cui_dock.c', 'src/base/cui_plot.c', 'src/base/cui_edit.c',
           'src/base/cui_scroll.c', 'src/components/cui_scroll.c',
           'src/base/cui_form.c', 'src/components/cui_form.c',
           'src/base/cui_surface.c', 'src/components/cui_surface.c',
           'src/base/cui_table.c', 'src/components/cui_table.c',
           'src/base/cui_tree.c', 'src/components/cui_tree.c',
           'src/base/cui_picker.c', 'src/components/cui_picker.c',
           'src/base/cui_numeric.c', 'src/components/cui_numeric.c',
           'src/base/cui_content.c', 'src/components/cui_content.c',
           'src/base/cui_disclosure.c', 'src/components/cui_disclosure.c',
           'src/base/cui_choice.c', 'src/components/cui_choice.c',
           'src/theme/cui_theme.c', 'src/components/cui_components.c', 'web/gallery.c']
EXPORTS = ['gallery_init', 'gallery_error', 'gallery_scroll', 'gallery_table_pin', 'gallery_table_mode', 'gallery_table_selection_mode', 'gallery_form_mode', 'gallery_form_submissions', 'gallery_surface_variant', 'gallery_surface_open', 'gallery_alert_attempts', 'gallery_input_mode', 'gallery_input_selection', 'gallery_event', 'gallery_text', 'gallery_focus', 'gallery_frame',
           'gallery_clipboard_request', 'gallery_clipboard_result', 'gallery_selection_mode', 'gallery_density', 'gallery_modal', 'gallery_time', 'gallery_redraw', 'gallery_hover_state', 'gallery_count', 'gallery_command', 'gallery_command_text', 'gallery_command_length',
           'gallery_semantic_id', 'gallery_semantic_parent', 'gallery_semantic_controls', 'gallery_semantic_labelled_by',
           'gallery_semantic_count', 'gallery_semantic', 'gallery_semantic_name', 'gallery_semantic_description', 'gallery_semantic_description_length',
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
    for name in ['index.html', 'gallery.css']:
        shutil.copyfile(ROOT / 'web' / name, out / name)
    wasm_version = hashlib.sha256((out / 'gallery.wasm').read_bytes()).hexdigest()[:16]
    javascript = (ROOT / 'web/gallery.js').read_text(encoding='utf-8').replace('__CUI_WASM_VERSION__', wasm_version)
    (out / 'gallery.js').write_text(javascript, encoding='utf-8', newline='\n')
    manifest = json.loads((ROOT / 'docs/gpui-parity.json').read_text(encoding='utf-8'))
    (out / 'catalog.json').write_text(json.dumps(manifest, indent=2)+'\n')
    notices = (ROOT / 'LICENSE').read_text(encoding='utf-8') + '\n\nClay layout engine\n==================\n' + (ROOT / 'third_party/clay/LICENSE').read_text(encoding='utf-8') + '\n\n' + (ROOT / 'web/TOOLCHAIN_NOTICES.txt').read_text(encoding='utf-8')
    (out / 'LICENSE.txt').write_text(notices, encoding='utf-8')
    (out / '.nojekyll').touch()
    print(f'Gallery built at {out}; serve over HTTP, e.g. python -m http.server -d {out}')

if __name__ == '__main__':
    main()
