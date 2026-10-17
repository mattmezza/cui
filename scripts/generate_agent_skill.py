#!/usr/bin/env python3
"""Generate the self-contained agent skill's data and shared exporter; --check in CI."""
import argparse
import json
from pathlib import Path
import sys
from agent_resources import collect

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    outputs = {
        ROOT / 'skills/cui/data/resources.json': (json.dumps(collect(ROOT), indent=2, ensure_ascii=False) + '\n').encode(),
        ROOT / 'skills/cui/scripts/agent_resources.py': (ROOT / 'scripts/agent_resources.py').read_bytes(),
    }
    stale = []
    for path, data in outputs.items():
        if args.check:
            if not path.is_file() or path.read_bytes() != data:
                stale.append(path.relative_to(ROOT).as_posix())
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
    if stale:
        print('Stale agent resources: ' + ', '.join(stale) + '\nRun python scripts/generate_agent_skill.py', file=sys.stderr)
        return 1
    print('Agent skill resources are current.' if args.check else 'Generated agent skill resources.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
