#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""A corrupt manifest cannot reach archive extraction or installer execution."""
from pathlib import Path
import subprocess
import sys
import tempfile
ROOT = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='cui-regex-toolchain-guard-') as directory:
    work = Path(directory)
    archives = work / 'archives'
    archives.mkdir()
    (archives / 'channel.toml').write_text('corrupt pinned manifest')
    cache = work / 'cache'
    result = subprocess.run([sys.executable, str(ROOT / 'scripts/prepare_regex_toolchain.py'), '--cache', str(cache), '--archive-dir', str(archives)], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    assert result.returncode and 'Pinned Rust checksum mismatch: channel.toml' in result.stdout, result.stdout
    assert not list(cache.iterdir()), 'Unverified manifest caused cache writes/extraction/install'
print('PASS corrupt official manifest rejected before writes/extraction/installer')
