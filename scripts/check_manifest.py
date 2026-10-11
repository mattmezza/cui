#!/usr/bin/env python3
"""Check full catalog and honest evidence requirements, not just entry count."""
import json,re
from pathlib import Path
r=Path(__file__).resolve().parents[1]
text=(r/'initial-prompt.md').read_text();names=re.findall(r'^\d+\. (.+)$',text,re.M);start=names.index('Accordion');expected=names[start:start+78]
m=json.loads((r/'docs/gpui-parity.json').read_text());entries=m['components'];actual=[e['name'] for e in entries]
assert len(actual)==78 and len(set(actual))==78 and set(actual)==set(expected),'catalog mismatch'
required={'name','reference_url','category','base_behavior','public_header','sample','tests','status','platforms','limitations','owner','last_verified_commit'}
for e in entries:
    assert required<=e.keys(),e['name']
    assert e['status'] in {'not_started','scaffolded','implemented','tested','verified'},e['name']
    if e['status']=='verified':
        assert e['sample'] and e['tests'] and e['public_header'] and e['last_verified_commit'],e['name']
        for p in [e['sample'],e['public_header'],*e['tests']]:assert (r/p).is_file(),(e['name'],p)
        assert e.get('evidence') and e.get('features') and all(f['status']=='verified' for f in e['features']),e['name']
counts={s:sum(e['status']==s for e in entries) for s in ['not_started','scaffolded','implemented','tested','verified']}
print('78 unique components:',counts)
