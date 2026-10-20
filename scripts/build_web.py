#!/usr/bin/env python3
"""Build the static browser gallery with pinned Zig 0.16.0. No Node build toolchain."""
from site_language import website_data
from icon_assets import embedded_names
import argparse
import hashlib
import json
import html
import re
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ["src/base/cui_otp_events.c", "src/base/cui_number_events.c", "src/base/cui_numeric_selection_events.c", "src/base/cui_tabs_events.c", "src/components/cui_text_overflow.c", "web/modules/cui_shadow_bridge.c", "src/base/cui_button_group.c", 'examples/composed_apps/app.c', 'src/effects/cui_effects.c', 'src/base/cui_questionnaire.c', 'src/components/cui_questionnaire.c', 'src/base/cui_speech.c', 'src/components/cui_speech.c',
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
           'src/base/cui_video.c', 'src/components/cui_video_controls.c', 'src/base/cui_media.c', 'src/components/cui_media.c', 'src/base/cui_message.c', 'src/components/cui_message.c', 'src/core/cui.c', 'src/base/cui_base.c', 'src/base/cui_models.c',
           'src/base/cui_overlay.c', 'src/base/cui_virtual.c', 'src/base/cui_virtual_scroll.c', 'src/components/cui_virtual.c', 'src/base/cui_dock.c', 'src/components/cui_dock.c', 'src/base/cui_plot.c', 'src/components/cui_plot.c', 'src/components/cui_plot_motion.c', 'src/base/cui_edit.c', 'src/base/cui_editor.c', 'src/components/cui_editor.c',
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
           'src/base/cui_radio_group.c', 'src/components/cui_radio_group.c',
           'src/theme/cui_theme.c', 'src/theme/cui_theme_registry.c', 'src/theme/cui_theme_motion.c', 'src/components/cui_components.c', 'src/components/cui_label.c', 'web/gallery.c']
EXPORTS = ['gallery_avatar_options', 'gallery_avatar_metric', 'gallery_video_session', 'gallery_video_sync', 'gallery_video_buffer', 'gallery_video_commit', 'gallery_video_request_count', 'gallery_video_request', 'gallery_video_drain', 'gallery_video_overflow', 'gallery_spinner_options', 'gallery_regex_available', 'cui_canvas_inset_shadow_query', 'cui_canvas_inset_shadow_prepare', 'cui_canvas_inset_shadow_element_bound', 'gallery_shadow_value', 'cui_canvas_shadow_values', 'cui_canvas_shadow_query', 'cui_canvas_shadow_prepare', 'cui_canvas_shadow_width', 'cui_canvas_shadow_height', 'cui_canvas_shadow_bound', 'cui_canvas_shadow_layer_bound', 'cui_canvas_shadow_pixels', 'gallery_button_collection', 'gallery_button_collection_stat', 'gallery_text_config', 'gallery_composition_bytes', 'gallery_radio_stat', 'gallery_scrollbar_options', 'gallery_date_locale', 'gallery_date_value', 'gallery_touch_page_cancels', 'gallery_touch_page_available', 'gallery_touch', 'gallery_dock_panel_region', 'gallery_plot_mode', 'gallery_dock_stat', 'gallery_editor_stat', 'gallery_virtual_mode', 'gallery_virtual_window_stat', 'gallery_text_view_motion', 'gallery_text_view_motion_stat', 'gallery_typography_options', 'gallery_command_family', 'gallery_effects_options', 'gallery_effect_pixel', 'gallery_app_stat', 'gallery_design_options', 'gallery_sheet_motion', 'gallery_screen_color', 'gallery_color_rgba', 'gallery_progress_mode', 'gallery_progress_shape', 'gallery_renderer_fixture', 'gallery_pointer_stats', 'gallery_font_metrics', 'gallery_keyboard_event', 'gallery_key_identity', 'gallery_key_character', 'gallery_key_stats', 'gallery_questionnaire_options', 'gallery_questionnaire_stats', 'gallery_speech_options', 'gallery_speech_stats', 'gallery_speech_text', 'gallery_chart_options', 'gallery_chart_content', 'gallery_chart_appearance', 'gallery_chart_stats', 'gallery_text_view_options', 'gallery_text_view_stats', 'gallery_root_stats', 'gallery_command_palette_options', 'gallery_command_palette_stats', 'gallery_settings_options', 'gallery_settings_stats', 'gallery_carousel_options', 'gallery_carousel_stats', 'gallery_theme_preset', 'gallery_theme_channel', 'gallery_sidebar_options', 'gallery_sidebar_stats', 'gallery_textarea_options', 'gallery_textarea_stats', 'gallery_textarea_text', 'gallery_title_options', 'gallery_title_action', 'gallery_toolbar_options', 'gallery_toolbar_activations', 'gallery_toolbar_cursor', 'gallery_description_options', 'gallery_status_options', 'gallery_resizable_options', 'gallery_resizable_size', 'gallery_icon_options', 'gallery_icon_stats', 'gallery_cursor', 'gallery_control_options', 'gallery_control_stats', 'gallery_image_options', 'gallery_image_stats', 'gallery_attachment_options', 'gallery_attachment_stats', 'gallery_checkbox_state', 'gallery_message_options', 'gallery_message_actions', 'gallery_conversation', 'gallery_dropdown_options', 'gallery_tabs_options', 'gallery_tabs_scroll', 'gallery_shimmer_mode', 'gallery_shimmer_phase', 'gallery_init', 'gallery_error', 'gallery_scroll', 'gallery_scroll_pixels', 'gallery_table_pin', 'gallery_table_mode', 'gallery_table_selection_mode', 'gallery_disclosure_options', 'gallery_disclosure_progress', 'gallery_disclosure_actions', 'gallery_notification_options', 'gallery_notification_stats', 'gallery_notification_entry', 'gallery_tag_options', 'gallery_marker_options', 'gallery_otp_options', 'gallery_otp_completions', 'gallery_otp_cursor', 'gallery_next_wake', 'gallery_group_mode', 'gallery_group_actions', 'gallery_form_mode', 'gallery_form_submissions', 'gallery_surface_variant', 'gallery_surface_open', 'gallery_alert_attempts', 'gallery_input_mode', 'gallery_input_selection', 'gallery_event', 'gallery_text', 'gallery_composition', 'gallery_ime_rect', 'gallery_focus', 'gallery_frame', 'gallery_plot_wheel', 'gallery_plot_domain',
           'gallery_clipboard_request', 'gallery_clipboard_result', 'gallery_selection_mode', 'gallery_density', 'gallery_modal', 'gallery_time', 'gallery_redraw', 'gallery_hover_state', 'gallery_count', 'gallery_command', 'gallery_command_text', 'gallery_command_length', 'gallery_bitmap_pixels', 'gallery_bitmap_revision', 'gallery_bitmap',
           'gallery_semantic_id', 'gallery_semantic_parent', 'gallery_semantic_controls', 'gallery_semantic_labelled_by',
           'gallery_semantic_count', 'gallery_semantic', 'gallery_semantic_name', 'gallery_semantic_description', 'gallery_semantic_description_length',
           'gallery_semantic_length', 'gallery_clicks', 'malloc', 'free']

def public_version(header):
    """Read exactly one public string version; never substitute the CMake ABI version."""
    values = re.findall(r'^\s*#\s*define\s+CUI_VERSION\s+"([^"\n]+)"\s*$', header, re.MULTILINE)
    definitions = re.findall(r'^\s*#\s*define\s+CUI_VERSION\b', header, re.MULTILINE)
    if len(values) != 1 or len(definitions) != 1:
        raise ValueError('Expected exactly one public CUI_VERSION string definition')
    return values[0]

def versioned_index(template, header):
    if template.count('__CUI_VERSION__') != 1:
        raise ValueError('Expected one web version placeholder')
    return template.replace('__CUI_VERSION__', html.escape(public_version(header), quote=True))

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', type=Path, default=ROOT / 'build/web')
    p.add_argument('--zig', default='zig')
    p.add_argument('--regex-library', type=Path, help='Optional compiled wasm32-wasip1 libcui_regex.a; default needs no Rust/archive')
    args = p.parse_args()
    version = subprocess.check_output([args.zig, 'version'], text=True).strip()
    if version != '0.16.0':
        raise RuntimeError(f'Expected Zig 0.16.0, found {version}')
    regex_link = []
    if args.regex_library:
        library = args.regex_library.resolve(strict=True)
        if not library.is_file():
            raise ValueError('regex library must be a compiled archive file')
        regex_link = ['-DCUI_GALLERY_REGEX=1', str(library)]
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    subprocess.run([args.zig, 'cc', '-target', 'wasm32-wasi', '-mexec-model=reactor',
        '-std=c11', '-O2', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
        '-I', str(ROOT / 'include'), '-I', str(ROOT / 'third_party/clay'),
        *[str(ROOT / s) for s in SOURCES], *regex_link, '-lm', '-Wl,-z,stack-size=8388608',
        *[f'-Wl,--export={s}' for s in EXPORTS], '-o', str(out / 'gallery.wasm')], check=True)
    shutil.copytree(ROOT / 'web/modules', out / 'modules', dirs_exist_ok=True)
    shutil.copytree(ROOT / 'web/assets', out / 'assets', dirs_exist_ok=True)
    for name in ['index.html', 'gallery.css', 'secure_input.js']:
        shutil.copyfile(ROOT / 'web' / name, out / name)
    index = (ROOT / 'web/index.html').read_text(encoding='utf-8')
    index = versioned_index(index, (ROOT / 'include/cui/cui.h').read_text(encoding='utf-8'))
    (out / 'index.html').write_text(index, encoding='utf-8')
    wasm_version = hashlib.sha256((out / 'gallery.wasm').read_bytes()).hexdigest()[:16]
    javascript = (ROOT / 'web/gallery.js').read_text(encoding='utf-8').replace('__CUI_WASM_VERSION__', wasm_version)
    (out / 'gallery.js').write_text(javascript, encoding='utf-8', newline='\n')
    shutil.copyfile(ROOT / 'third_party/gpui_themes/presets.json', out / 'themes.json')
    subprocess.run(['uv', 'run', str(ROOT / 'scripts/build_docs.py'), '--output', str(out), '--profile', 'web-default-icons' + ('-regex' if args.regex_library else '')], check=True)
    icon_archive = json.loads((ROOT / 'third_party/gpui_icons/icons.json').read_text())
    defaults = embedded_names(icon_archive)
    icons = [{'name': name, 'embedded': name in defaults} for name in sorted(icon_archive['icons'])]
    (out / 'icons.json').write_text(json.dumps({'source_commit': icon_archive['source_commit'],
        'icons': icons}, indent=2) + '\n')
    manifest = website_data(json.loads((ROOT / 'docs/gpui-parity.json').read_text(encoding='utf-8')))
    usage = json.loads((ROOT / 'docs/component-usage.json').read_text(encoding='utf-8'))['components']
    if [c['name'] for c in usage] != [c['name'] for c in manifest['components']]:
        raise ValueError('Component usage must match all 78 manifest families')
    for component, explanation in zip(manifest['components'], usage):
        component['usage'] = explanation
    manifest['extensions'] = [{
        'name': 'Video', 'category': 'extensions', 'status': 'extension', 'public_header': 'include/cui/cui_video_controls.h',
        'source_commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
        'features': [{'name': 'Browser-decoded local sample with CUI player controls', 'status': 'tested'}],
        'limitations': ['Extension outside the frozen 78-family GPUI baseline.',
            'Silent sample; browser codec and fullscreen support vary. Safari is not validated.',
            'Seek cancel restores the requested position; decoder completion is asynchronous.'],
        'usage': {'composition': 'cui_video_player surface plus optional CUI controls',
            'state_ownership': 'Browser host owns media, timeline confirmation and bounded RGBA staging storage.',
            'interaction': 'Typed requests are drained after rendering; confirmed HTMLVideoElement state is synchronized back.',
            'usage': 'Play, pause, scrub, skip two seconds and switch 1x / 1.5x. The sample is muted and loops.',
            'docs': ['docs/VIDEO_PLAYER.md', 'docs/VIDEO_GUIDE.md']}}]
    (out / 'catalog.json').write_text(json.dumps(manifest, indent=2)+'\n')
    notices = (ROOT / 'LICENSE').read_text(encoding='utf-8') + '\n\nClay layout engine\n==================\n' + (ROOT / 'third_party/clay/LICENSE').read_text(encoding='utf-8') + '\n\nNanoSVG\n=======\n' + (ROOT / 'third_party/nanosvg/LICENSE').read_text(encoding='utf-8') + '\n\nGPUI Kit icons\n==============\n' + (ROOT / 'third_party/gpui_icons/LICENSE').read_text(encoding='utf-8') + '\n\n' + (ROOT / 'web/TOOLCHAIN_NOTICES.txt').read_text(encoding='utf-8')
    notices += '\n\nGPUI Kit theme presets\n======================\n' + (ROOT / 'third_party/gpui_themes/LICENSE').read_text(encoding='utf-8')
    notices += '\n\nUnicode 17 grapheme data\n========================\n' + (ROOT / 'third_party/unicode/17.0.0/LICENSE.txt').read_text(encoding='utf-8')
    (out / 'LICENSE.txt').write_text(notices, encoding='utf-8')
    downloads = out / 'downloads'
    downloads.mkdir(exist_ok=True)
    archive = downloads / 'cui-native.tar.gz'
    subprocess.run([sys.executable, str(ROOT / 'scripts/native_bundle.py'), '--output', str(archive)], check=True)
    checksum = hashlib.sha256(archive.read_bytes()).hexdigest()
    (downloads / 'cui-native.tar.gz.sha256').write_text(checksum + '  cui-native.tar.gz\n')
    info = json.loads((out / 'build-info.json').read_text())
    info['build_options'] = {'CUI_ICON_ALL_ASSETS': False, 'CUI_GALLERY_REGEX': bool(args.regex_library),
                             'target': 'wasm32-wasi', 'zig': version}
    info['downloads'] = {'downloads/cui-native.tar.gz': {'sha256': checksum, 'bytes': archive.stat().st_size}}
    (out / 'build-info.json').write_text(json.dumps(info, indent=2) + '\n')
    subprocess.run([sys.executable, str(ROOT / 'scripts/build_sharing.py'), '--output', str(out)], check=True)
    (out / '.nojekyll').touch()
    print(f'Gallery built at {out}; serve over HTTP, e.g. python -m http.server -d {out}')

if __name__ == '__main__':
    main()
