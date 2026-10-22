#!/usr/bin/env python3
"""Delegate to canonical pinned WASI registrations."""
import runpy
from pathlib import Path
if __name__ == '__main__':
    runpy.run_path(str(Path(__file__).resolve().parents[3] / 'scripts/test_wasi.py'), run_name='__main__')
