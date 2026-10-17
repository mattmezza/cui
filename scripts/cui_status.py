#!/usr/bin/env python3
"""Read component status and limitations directly from the 78-entry manifest."""
import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def summary(entries):
    lines = ['# Component status', '',
             'Generated from [gpui-parity.json](gpui-parity.json). Status describes the tested slice,',
             'not complete parity. Run `python scripts/cui_status.py Input Icon Sidebar` for',
             'features, evidence, and full limitations. Regenerate with `--write-summary`.', '',
             '| Component | Status | Reference |', '| --- | --- | --- |']
    for entry in entries:
        header = entry['public_header']
        lines.append(f"| {entry['name']} | {entry['status']} | [{Path(header).name}](../{header}) |")
    return '\n'.join(lines) + '\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('components', nargs='*', help='Case-insensitive exact component names')
    parser.add_argument('--write-summary', action='store_true')
    parser.add_argument('--check', action='store_true', help='Check generated summary freshness')
    args = parser.parse_args()
    entries = json.loads((ROOT / 'docs/gpui-parity.json').read_text())['components']
    target = ROOT / 'docs/COMPONENT_STATUS.md'
    if args.write_summary:
        target.write_text(summary(entries), encoding='utf-8')
    if args.check:
        if not target.exists() or target.read_text(encoding='utf-8') != summary(entries):
            parser.exit(1, 'Component summary is stale; run scripts/cui_status.py --write-summary\n')
        return
    names = {entry['name'].casefold(): entry for entry in entries}
    unknown = [name for name in args.components if name.casefold() not in names]
    if unknown:
        parser.error('Unknown component(s): ' + ', '.join(unknown))
    if not args.components:
        if not args.write_summary:
            print(summary(entries), end='')
        return
    for name in args.components:
        entry = names[name.casefold()]
        print(f"{entry['name']}: {entry['status']} ({entry['public_header']})")
        for feature in entry['features']:
            print(f"  [{feature['status']}] {feature['name']}")
            for evidence in feature.get('evidence', []):
                print(f'    Evidence: {evidence}')
        for limitation in entry['limitations']:
            print(f'  Limitation: {limitation}')
        print()


if __name__ == '__main__':
    main()
