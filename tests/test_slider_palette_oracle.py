#!/usr/bin/env python3
"""Independent pinned-token oracle against actual C resolver output; no imports of its generator."""
import argparse
import colorsys
import hashlib
import json
from pathlib import Path
import re
import subprocess
ROOT = Path(__file__).resolve().parents[1]


def expected_palettes():
    data = (ROOT / 'third_party/gpui_themes/palettes.json').read_bytes()
    assert hashlib.sha256(data).hexdigest() == '03de5a483daab83ed983495963a560d70f8ea7db3fd079bf4a69a4df0e2f77c0'
    source = json.loads(data)
    def source_color(token):
        text = source['named_colors'].get(token, token)
        assert re.fullmatch('#[0-9a-fA-F]{6}([0-9a-fA-F]{2})?', text), text
        channels = [int(text[n:n+2], 16) / 255 for n in range(1, len(text), 2)]
        if len(channels) == 3: channels.append(1)
        # Independent path: actual source parses encoded channels to HSLA; the
        # shader evaluates its HSL formula but treats RGB as linear (no decode).
        h, l, s = colorsys.rgb_to_hls(*channels[:3])
        return [*colorsys.hls_to_rgb(h, l, s), channels[3]]
    def brush(token):
        if not token.startswith('linear-gradient('):
            return {'kind': 0, 'solid': source_color(token), 'space': 0, 'projection': 0,
                    'angle': 0, 'stops': [[0, [0]*4], [0, [0]*4]]}
        fields = [field.strip() for field in token[len('linear-gradient('):-1].split(',')]
        assert len(fields) == 3 and fields[0].endswith('deg')
        return {'kind': 1, 'solid': [0]*4, 'space': 0, 'projection': 0,
                'angle': float(fields[0][:-3]),
                'stops': [[0, source_color(fields[1])], [1, source_color(fields[2])]]}
    result = []
    themes = sorted(source['themes'], key=lambda t: (t['mode'] != 'light',
                    not t['name'].startswith('Default '), t['name'].casefold()))
    for theme in themes:
        raw = theme['colors']
        assert all(k in raw for k in ('primary.background', 'primary.foreground', 'base.blue'))
        primary = brush(raw['primary.background'])
        primary_color = primary['solid'] if primary['kind'] == 0 else primary['stops'][0][1]
        bar = source_color(raw['slider.background']) if 'slider.background' in raw else primary_color
        thumb = brush(raw['slider.thumb.background']) if 'slider.thumb.background' in raw else brush(raw['primary.foreground'])
        result.append(dict(name=theme['name'], bar=bar, ring=source_color(raw.get('ring', raw['base.blue'])), **thumb))
    assert len(result) == 38
    # Independent literal anchors from pinned schema/defaults/Aurora source.
    assert result[0]['name'] == 'Default Light' and abs(result[0]['bar'][0] - 23/255) < 1e-12
    aurora = next(p for p in result if p['name'] == 'Aurora Light')
    assert aurora['kind'] == 1 and aurora['angle'] == 180 and aurora['bar'] == source_color('#0F172A')
    assert aurora['stops'] == [[0, source_color('#FFFFFF')], [1, source_color('#F8FAFC')]]
    assert sum(p['kind'] == 1 for p in result) == 1
    return result


def compare(actual, expected, path='palette'):
    if isinstance(expected, dict):
        assert actual.keys() == expected.keys(), path
        for key in expected: compare(actual[key], expected[key], path+'.'+key)
    elif isinstance(expected, list):
        assert len(actual) == len(expected), path
        for i, value in enumerate(expected): compare(actual[i], value, path+f'[{i}]')
    elif isinstance(expected, (float, int)):
        assert abs(actual - expected) < 2e-6, (path, actual, expected)
    else:
        assert actual == expected, (path, actual, expected)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--probe', type=Path)
    args = parser.parse_args()
    expected = expected_palettes()
    if args.probe:
        actual = json.loads(subprocess.check_output([str(args.probe.resolve()), '--dump'], text=True))
        compare(actual, expected)
        print('PASS actual C values against independent pinned oracle: all38 palettes and owned Aurora brush')
    else:
        print('PASS static independent pinned oracle:38 entries (C runtime not checked without --probe)')


if __name__ == '__main__': main()
