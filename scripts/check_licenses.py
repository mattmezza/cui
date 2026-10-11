#!/usr/bin/env python3
import json,hashlib
from pathlib import Path
r=Path(__file__).resolve().parents[1]
m=json.loads((r/'third_party/dependencies.json').read_text())
for e in m:
    for p,h in e.get('sha256',{}).items():
        assert hashlib.sha256((r/p).read_bytes()).hexdigest()==h,p
    assert (r/e['license_file']).is_file(),e['name']
print('Pinned dependency hashes and license files match')
