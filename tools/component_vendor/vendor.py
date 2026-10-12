#!/usr/bin/env python3
"""Vendor the embeddable layers with their license/provenance. No CLI required."""
import argparse,json,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('destination',type=Path);p.add_argument('--layer',choices=['core','base','components'],default='components');a=p.parse_args()
headers=['cui.h','cui_renderer.h','cui_platform.h']
if a.layer in ['base','components']:headers+=['cui_edit.h','cui_plot.h','cui_dock.h','cui_virtual.h','cui_overlay.h','cui_base.h']
if a.layer=='components':headers+=['cui_theme.h','cui_content.h','cui_disclosure.h','cui_choice.h','cui_numeric.h','cui_picker.h','cui_tree.h','cui_components.h']
a.destination.mkdir(parents=True,exist_ok=True)
files=[(ROOT/'dist'/h,a.destination/h) for h in headers]
files += [(ROOT/'LICENSE',a.destination/'LICENSE-CUI'),(ROOT/'third_party/clay/LICENSE',a.destination/'LICENSE-Clay'),(ROOT/'third_party/dependencies.json',a.destination/'dependencies.json')]
# Preflight entire transaction: never overwrite a destination's unknown files.
for src,dst in files:
 if not src.exists():p.error(f'{src} missing; run python scripts/amalgamate.py')
 if dst.exists() and dst.read_bytes()!=src.read_bytes():p.error(f'{dst} differs; choose empty destination or review upgrade manually')
for src,dst in files:shutil.copyfile(src,dst)
print(f'Vendored {a.layer} dependency closure into {a.destination}; component layer currently includes all implemented controls, not source slicing.')
