#!/usr/bin/env python3
"""Build the static browser gallery with pinned Zig 0.16.0. No Node build toolchain."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ['examples/composed_apps/app.c', 'src/effects/cui_effects.c', 'src/base/cui_questionnaire.c', 'src/components/cui_questionnaire.c', 'src/base/cui_speech.c', 'src/components/cui_speech.c',
           'src/base/cui_chart.c', 'src/components/cui_chart.c',
           'src/base/cui_text_view.c', 'src/components/cui_text_view.c',
           'src/base/cui_root.c', 'src/components/cui_root.c',
           'src/base/cui_command.c', 'src/components/cui_command.c',
           'src/base/cui_settings.c', 'src/components/cui_settings.c',
           'src/base/cui_carousel.c', 'src/components/cui_carousel.c',
           'src/base/cui_sidebar.c', 'src/components/cui_sidebar.c',
           'src/base/cui_textarea.c', 'src/components/cui_textarea.c',
           'src/base/cui_description.c', 'src/components/cui_description.c',
           'src/base/cui_chrome.c', 'src/components/cui_chrome.c',
           'src/base/cui_resizable.c', 'src/components/cui_resizable.c',
           'src/base/cui_icon.c', 'src/components/cui_icon.c',
           'src/base/cui_toolbar.c', 'src/components/cui_toolbar.c',
           'src/base/cui_media.c', 'src/components/cui_media.c', 'src/base/cui_message.c', 'src/components/cui_message.c', 'src/core/cui.c', 'src/base/cui_base.c', 'src/base/cui_models.c',
           'src/base/cui_overlay.c', 'src/base/cui_virtual.c', 'src/base/cui_dock.c', 'src/base/cui_plot.c', 'src/base/cui_edit.c',
           'src/base/cui_scroll.c', 'src/components/cui_scroll.c',
           'src/base/cui_status.c', 'src/components/cui_status.c',
           'src/base/cui_otp.c', 'src/components/cui_otp.c',
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
EXPORTS = ['gallery_text_view_motion', 'gallery_text_view_motion_stat', 'gallery_typography_options', 'gallery_command_family', 'gallery_effects_options', 'gallery_effect_pixel', 'gallery_app_stat', 'gallery_design_options', 'gallery_sheet_motion', 'gallery_screen_color', 'gallery_color_rgba', 'gallery_progress_mode', 'gallery_renderer_fixture', 'gallery_pointer_stats', 'gallery_font_metrics', 'gallery_keyboard_event', 'gallery_key_identity', 'gallery_key_character', 'gallery_key_stats', 'gallery_questionnaire_options', 'gallery_questionnaire_stats', 'gallery_speech_options', 'gallery_speech_stats', 'gallery_speech_text', 'gallery_chart_options', 'gallery_chart_stats', 'gallery_text_view_options', 'gallery_text_view_stats', 'gallery_root_stats', 'gallery_command_palette_options', 'gallery_command_palette_stats', 'gallery_settings_options', 'gallery_settings_stats', 'gallery_carousel_options', 'gallery_carousel_stats', 'gallery_theme_preset', 'gallery_theme_channel', 'gallery_sidebar_options', 'gallery_sidebar_stats', 'gallery_textarea_options', 'gallery_textarea_stats', 'gallery_textarea_text', 'gallery_title_options', 'gallery_title_action', 'gallery_toolbar_options', 'gallery_toolbar_activations', 'gallery_toolbar_cursor', 'gallery_description_options', 'gallery_status_options', 'gallery_resizable_options', 'gallery_resizable_size', 'gallery_icon_options', 'gallery_icon_stats', 'gallery_cursor', 'gallery_control_options', 'gallery_control_stats', 'gallery_image_options', 'gallery_image_stats', 'gallery_attachment_options', 'gallery_attachment_stats', 'gallery_checkbox_state', 'gallery_message_options', 'gallery_message_actions', 'gallery_conversation', 'gallery_dropdown_options', 'gallery_tabs_options', 'gallery_tabs_scroll', 'gallery_shimmer_mode', 'gallery_shimmer_phase', 'gallery_init', 'gallery_error', 'gallery_scroll', 'gallery_scroll_pixels', 'gallery_table_pin', 'gallery_table_mode', 'gallery_table_selection_mode', 'gallery_disclosure_options', 'gallery_disclosure_progress', 'gallery_disclosure_actions', 'gallery_notification_options', 'gallery_notification_stats', 'gallery_notification_entry', 'gallery_tag_options', 'gallery_marker_options', 'gallery_otp_options', 'gallery_otp_completions', 'gallery_otp_cursor', 'gallery_next_wake', 'gallery_group_mode', 'gallery_group_actions', 'gallery_form_mode', 'gallery_form_submissions', 'gallery_surface_variant', 'gallery_surface_open', 'gallery_alert_attempts', 'gallery_input_mode', 'gallery_input_selection', 'gallery_event', 'gallery_text', 'gallery_focus', 'gallery_frame',
           'gallery_clipboard_request', 'gallery_clipboard_result', 'gallery_selection_mode', 'gallery_density', 'gallery_modal', 'gallery_time', 'gallery_redraw', 'gallery_hover_state', 'gallery_count', 'gallery_command', 'gallery_command_text', 'gallery_command_length', 'gallery_bitmap_pixels', 'gallery_bitmap_revision', 'gallery_bitmap',
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
    shutil.copytree(ROOT / 'web/assets', out / 'assets', dirs_exist_ok=True)
    for name in ['index.html', 'gallery.css']:
        shutil.copyfile(ROOT / 'web' / name, out / name)
    wasm_version = hashlib.sha256((out / 'gallery.wasm').read_bytes()).hexdigest()[:16]
    javascript = (ROOT / 'web/gallery.js').read_text(encoding='utf-8').replace('__CUI_WASM_VERSION__', wasm_version)
    (out / 'gallery.js').write_text(javascript, encoding='utf-8', newline='\n')
    shutil.copyfile(ROOT / 'third_party/gpui_themes/presets.json', out / 'themes.json')
    subprocess.run(['uv', 'run', str(ROOT / 'scripts/build_docs.py'), '--output', str(out)], check=True)
    manifest = json.loads((ROOT / 'docs/gpui-parity.json').read_text(encoding='utf-8'))
    usage = json.loads((ROOT / 'docs/component-usage.json').read_text(encoding='utf-8'))['components']
    if [c['name'] for c in usage] != [c['name'] for c in manifest['components']]:
        raise ValueError('Component usage must match all 78 manifest families')
    for component, explanation in zip(manifest['components'], usage):
        component['usage'] = explanation
    (out / 'catalog.json').write_text(json.dumps(manifest, indent=2)+'\n')
    notices = (ROOT / 'LICENSE').read_text(encoding='utf-8') + '\n\nClay layout engine\n==================\n' + (ROOT / 'third_party/clay/LICENSE').read_text(encoding='utf-8') + '\n\nNanoSVG\n=======\n' + (ROOT / 'third_party/nanosvg/LICENSE').read_text(encoding='utf-8') + '\n\nGPUI Kit icons\n==============\n' + (ROOT / 'third_party/gpui_icons/LICENSE').read_text(encoding='utf-8') + '\n\n' + (ROOT / 'web/TOOLCHAIN_NOTICES.txt').read_text(encoding='utf-8')
    notices += '\n\nGPUI Kit theme presets\n======================\n' + (ROOT / 'third_party/gpui_themes/LICENSE').read_text(encoding='utf-8')
    (out / 'LICENSE.txt').write_text(notices, encoding='utf-8')
    subprocess.run([sys.executable, str(ROOT / 'scripts/build_sharing.py'), '--output', str(out)], check=True)
    (out / '.nojekyll').touch()
    print(f'Gallery built at {out}; serve over HTTP, e.g. python -m http.server -d {out}')

if __name__ == '__main__':
    main()
