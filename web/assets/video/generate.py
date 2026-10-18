#!/usr/bin/env python3
"""Rebuild the original, silent browser video fixture with local FFmpeg (no downloads)."""
import hashlib
import json
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parent
version = subprocess.check_output(['ffmpeg', '-version'], text=True).splitlines()[0]
commands = []
for filename, codec in [('sample.webm', ['-c:v', 'libvpx', '-b:v', '140k', '-deadline', 'best']),
                        ('sample.mp4', ['-c:v', 'libx264', '-crf', '30', '-movflags', '+faststart'])]:
    command = ['ffmpeg', '-hide_banner', '-loglevel', 'error', '-y', '-f', 'lavfi', '-i',
               'testsrc2=size=320x180:rate=15:duration=12', '-an', '-fflags', '+bitexact', '-flags:v', '+bitexact', '-map_metadata', '-1', '-pix_fmt', 'yuv420p',
               '-threads', '1', *codec, filename]
    subprocess.run(command, cwd=root, check=True)
    commands.append(command)
(root / 'receipt.json').write_text(json.dumps({'origin': 'Original FFmpeg lavfi testsrc2; no external media; silent',
    'license': 'CC0-1.0', 'tool': version, 'commands': commands,
    'files': {name: {'sha256': hashlib.sha256((root/name).read_bytes()).hexdigest(),
                     'bytes': (root/name).stat().st_size} for name in ['sample.webm', 'sample.mp4']}}, indent=2)+'\n')
