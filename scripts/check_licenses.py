#!/usr/bin/env python3
import json,hashlib
from pathlib import Path
r=Path(__file__).resolve().parents[1]
m=json.loads((r/'third_party/dependencies.json').read_text(encoding="utf-8"))
for e in m:
    for p,h in e.get('sha256',{}).items():
        assert hashlib.sha256((r/p).read_bytes()).hexdigest()==h,p
    assert (r/e['license_file']).is_file(),e['name']
receipt_path = r/'src/theme/cui_theme_palette.receipt.json'
if receipt_path.exists():
    receipt=json.loads(receipt_path.read_text(encoding='utf-8'))
    assert receipt['source_commit']=='87d10ae5e1299d1be18670a37570c5402f952e34'
    assert receipt['license']=='Apache-2.0' and (r/receipt['license_notice']).is_file()
    assert hashlib.sha256((r/receipt['derived_file']).read_bytes()).hexdigest()==receipt['derived_sha256']
regex_pin = json.loads((r/'backends/regex/pin.json').read_text())
for filename, key in [('cache.patch', 'cache_patch_sha256'), ('Cargo.lock', 'cargo_lock_sha256')]:
    assert hashlib.sha256((r/'backends/regex'/filename).read_bytes()).hexdigest() == regex_pin[key], filename
print('Pinned dependency hashes and license files match')
