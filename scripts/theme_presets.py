#!/usr/bin/env python3
"""Generate CUI theme presets and website palette data from pinned color sources."""
import argparse
import colorsys
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def generate():
    source = json.loads((ROOT / 'third_party/gpui_themes/palettes.json').read_text())
    named = source['named_colors']

    def rgba(value):
        if value.startswith('linear-gradient('):
            value = re.search(r'#[0-9a-fA-F]{6,8}', value).group(0)
        if value == 'transparent':
            return (0, 0, 0, 0)
        value, _, alpha = value.partition('/')
        value = named.get(value, value)
        if not value.startswith('#') or len(value) not in (7, 9):
            raise ValueError(f'Unsupported source color: {value}')
        result = tuple(bytes.fromhex(value[1:]))
        if len(result) == 3:
            result += (255,)
        return result[:3] + (round(255 * int(alpha) / 100),) if alpha else result

    def mix(a, b, weight):
        return tuple(round(x * (1 - weight) + y * weight) for x, y in zip(a, b))

    def darken(color, amount):
        h, l, s = colorsys.rgb_to_hls(*(v / 255 for v in color[:3]))
        return tuple(round(v * 255) for v in colorsys.hls_to_rgb(h, max(0, l * (1 - amount)), s)) + (color[3],)

    palettes, entries, records, chart_rows = [], [], [], []
    ordered = sorted(source['themes'], key=lambda t: (t['mode'] != 'light', not t['name'].startswith('Default '), t['name'].casefold()))
    for theme in ordered:
        raw = theme['colors']
        colors = {key: rgba(value) for key, value in raw.items()}
        blue = colors['base.blue']
        h, l, saturation = colorsys.rgb_to_hls(*(v / 255 for v in blue[:3]))
        fallback = [tuple(round(v * 255) for v in colorsys.hls_to_rgb(h, min(1, max(0, l * factor)), saturation)) + (blue[3],)
                    for factor in [1.4, 1.2, 1, .8, .6]]
        chart = [colors.get(f'chart.{i}', fallback[i - 1]) for i in range(1, 6)]
        chart += [colors.get('chart.bullish', colors['base.green']), colors.get('chart.bearish', colors['base.red'])]
        chart += [colors.get('chart.grid', colors['border'][:3] + (round(colors['border'][3] * .6),))]
        chart_rows.append('    {' + ', '.join('{' + ', '.join(map(str, color)) + '}' for color in chart) + '}, /* ' + theme['name'] + ' */')
        dark = theme['mode'] == 'dark'
        bg, fg = colors['background'], colors['foreground']
        border = colors.get('border', mix(bg, fg, .2))
        primary = colors.get('primary.background', colors.get('base.blue', fg))
        secondary = colors.get('secondary.background', mix(bg, border, .4))
        muted_bg = colors.get('muted.background', secondary)
        muted = colors.get('muted.foreground', mix(muted_bg, fg, .7))
        colors.setdefault('muted.foreground', muted)
        colors.setdefault('popover.background', bg)
        colors.setdefault('primary.foreground', bg)
        colors.setdefault('secondary.foreground', fg)
        colors.setdefault('accent.background', secondary)
        colors.setdefault('accent.foreground', fg)
        values = {'background': bg, 'surface': colors['popover.background'], 'text': fg,
                  'muted': muted, 'muted_background': muted_bg, 'primary': primary, 'on_primary': colors['primary.foreground'],
                  'border': border, 'focus': colors.get('ring', primary),
                  'input': colors.get('input.border', border),
                  'input_background': colors.get('input.background', bg),
                  'secondary': secondary, 'secondary_foreground': colors['secondary.foreground'],
                  'accent': colors['accent.background'], 'accent_foreground': colors['accent.foreground'],
                  'popover': colors['popover.background'], 'popover_foreground': colors.get('popover.foreground', fg)}
        for intent, base, fallback in [('danger', 'red', '#ef4444'), ('warning', 'yellow', '#eab308'),
                                       ('success', 'green', '#22c55e'), ('info', 'cyan', '#06b6d4')]:
            values[intent] = colors.get(intent + '.background', colors.get('base.' + base, rgba(fallback)))
            values['on_' + intent] = colors.get(intent + '.foreground', colors['primary.foreground'])
        for field, key, fallback in [
            ('link', 'link', primary), ('link_hover', 'link.hover', primary), ('link_active', 'link.active', primary),
            ('secondary_hover', 'secondary.hover.background', mix(bg, secondary, .9)),
            ('tab_foreground', 'tab.foreground', fg), ('tab_active_foreground', 'tab.active.foreground', fg),
            ('tab_active', 'tab.active.background', bg), ('tab_bar', 'tab_bar.background', secondary),
            ('tab_bar_segmented', 'tab_bar.segmented.background', secondary),
            ('switch_background', 'switch.background', secondary), ('switch_thumb', 'switch.thumb.background', bg),
            ('list_hover', 'list.hover.background', values['accent']), ('list_active', 'list.active.background', mix(bg, primary, .1)),
            ('selection', 'selection.background', primary[:3] + (76,)),
            ('description_list_label', 'description_list.label.background', mix(bg, border, .2)),
            ('description_list_label_foreground', 'description_list.label.foreground', muted),
            ('title_bar', 'title_bar.background', bg), ('title_bar_border', 'title_bar.border', border),
            ('status_bar', 'status_bar.background', colors.get('title_bar.background', bg)),
            ('status_bar_border', 'status_bar.border', colors.get('title_bar.border', border)),
            ('sidebar', 'sidebar.background', mix(bg, border, .15)), ('sidebar_foreground', 'sidebar.foreground', fg),
            ('sidebar_border', 'sidebar.border', border), ('sidebar_accent', 'sidebar.accent.background', values['accent']),
            ('sidebar_accent_foreground', 'sidebar.accent.foreground', values['accent_foreground']),
            ('caret', 'caret', primary), ('group_box', 'group_box.background', mix(bg, secondary, .3 if dark else .4)),
            ('group_box_foreground', 'group_box.foreground', fg)]:
            values[field] = colors.get(key, fallback)
        def opacity(color, amount):
            return color[:3] + (round(color[3] * amount),)

        for index, intent in enumerate(['button', 'primary', 'secondary', 'danger', 'warning', 'success', 'info']):
            prefix = 'button' if index == 0 else 'button.' + intent
            base = values['input'] if index == 0 else values[intent]
            base_hover = colors.get(intent + '.hover.background', mix(bg, base, .9))
            base_active = colors.get(intent + '.active.background', darken(base, .2 if dark else .1))
            if index == 0:
                normal = colors.get('button.background', opacity(base, .3) if dark else bg)
                hover = colors.get('button.hover.background', opacity(base, .5))
                active = colors.get('button.active.background', opacity(base, .7))
                foreground = colors.get('button.foreground', fg)
            elif index >= 3:
                normal = colors.get(prefix + '.background', opacity(base, .2))
                hover = colors.get(prefix + '.hover.background', opacity(base, .3))
                active = colors.get(prefix + '.active.background', opacity(base, .4))
                foreground = colors.get(prefix + '.foreground', base)
            else:
                normal = colors.get(prefix + '.background', base)
                hover = colors.get(prefix + '.hover.background', base_hover)
                active = colors.get(prefix + '.active.background', base_active)
                foreground = colors.get(prefix + '.foreground', values['secondary_foreground'] if index == 2 else values['on_primary'])
            edge = base if index == 0 else border if index == 2 else normal if index >= 3 else base
            for field, value in {'normal': normal, 'hover': hover, 'active': active, 'foreground': foreground,
                                 'outline_normal': values['input_background'] if index == 0 else opacity(base, .1),
                                 'outline_hover': opacity(base if index == 0 else base_hover, .5 if index == 0 else .2),
                                 'outline_active': opacity(base if index == 0 else base_active, .7 if index == 0 else .4),
                                 'border': edge,
                                 'outline_border': opacity(base, .4) if index >= 3 else edge,
                                 'outline_foreground': fg if index == 0 else values['secondary_foreground'] if index == 2 else base}.items():
                values[f'buttons[{index}].{field}'] = value
        begin = len(entries)
        entries.extend(values.items())
        records.append((theme['name'], dark, begin, len(values)))
        palettes.append({'id': re.sub(r'[^a-z0-9]+', '-', theme['name'].lower()).strip('-'),
                         'name': theme['name'], 'mode': theme['mode'], 'author': theme['author'], 'url': theme['url'],
                         'colors': {key: '#' + bytes(value).hex() for key, value in colors.items()},
                         'adaptations': [key for key, value in raw.items() if value.startswith('linear-gradient(')]})
    fields = sorted({field for field, _ in entries})
    lines = ['/* Generated by scripts/theme_presets.py; source attribution in third_party/gpui_themes. */',
             'typedef struct { unsigned field; cui_color color; } cui_theme_preset_color;',
             'static const cui_theme_preset_color cui_theme_preset_colors[] = {']
    lines += ['    {' + str(fields.index(field)) + ', {' + ', '.join(map(str, color)) + '}},' for field, color in entries]
    lines += ['};', 'typedef struct { const char *name; bool dark; size_t begin, count; } cui_theme_preset_record;',
              'static const cui_theme_preset_record cui_theme_presets[] = {']
    lines += [f'    {{{json.dumps(name)}, {str(dark).lower()}, {begin}, {count}}},' for name, dark, begin, count in records]
    lines += ['};', 'static void cui_theme_preset_assign(cui_theme *theme, unsigned field, cui_color color) {', '    switch (field) {']
    lines += [f'    case {index}: theme->{field} = color; break;' for index, field in enumerate(fields)]
    lines += ['    default: break;', '    }', '}', '']
    lines += ['/* BEGIN CHART PRESETS: generated by scripts/theme_presets.py */',
              'static const cui_color cui_theme_chart_presets[38][8] = {', *chart_rows, '};',
              '/* END CHART PRESETS */', '']
    return '\n'.join(lines), json.dumps(palettes, indent=2, ensure_ascii=False) + '\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    generated = generate()
    for path, content in zip(['src/theme/cui_theme_presets.inc', 'third_party/gpui_themes/presets.json'], generated):
        target = ROOT / path
        if args.check:
            if not target.exists() or target.read_text() != content:
                raise SystemExit(f'Stale generated theme data: {path}')
        else:
            target.write_text(content, encoding='utf-8')


if __name__ == '__main__':
    main()
