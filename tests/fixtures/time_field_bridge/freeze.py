"""Freeze bounded pre-feature bridge sources for a same-engine legacy comparison.

No compilation, checkout, dependency access or candidate-derived expected values.
Run before the first candidate bridge build; keep generated receipt with build logs.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[3]
FILES = ('include/cui/cui_bridge.h', 'src/bridge/cui_bridge.c',
         'src/bridge/cui_bridge_internal.h', 'src/bridge/cui_bridge_slider_internal.h')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--revision', default='b4cd6a8f')
    args = parser.parse_args()
    revision = subprocess.check_output(['git', 'rev-parse', '--verify', args.revision + '^{commit}'], cwd=ROOT, text=True).strip()
    data = {name: subprocess.check_output(['git', 'show', revision + ':' + name], cwd=ROOT) for name in FILES}
    # Never overwrite an independently captured baseline.
    args.output.mkdir(parents=True, exist_ok=False)
    for name, contents in data.items():
        path = args.output / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(contents)
    receipt = {'source_revision': revision, 'files': {
        name: hashlib.sha256(contents).hexdigest() for name, contents in data.items()},
        'scope': 'Pre-localized-TimeField bridge sources only; link old/new separately against same engine. No runtime claim.'}
    (args.output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(receipt, indent=2))


if __name__ == '__main__':
    main()
