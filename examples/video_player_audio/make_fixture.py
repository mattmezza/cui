#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Actual deterministic muxed video/audio, local files only."""
from pathlib import Path
import math
import struct
import subprocess
import sys

folder = Path(sys.argv[1]); folder.mkdir(parents=True, exist_ok=True)
if not subprocess.check_output(['ffmpeg', '-version'], text=True).startswith('ffmpeg version n9.0.2 '):
    raise SystemExit('Existing FFmpeg n9.0.2 required; no installation attempted')
video = folder / 'video.raw'
video.write_bytes(bytes((255, 0, 0, 255))*64*32*10 + bytes((0, 0, 255, 255))*64*32*10)
for rate in (48000, 44100):
    raw = folder / f'{rate}.raw'
    # Explicit generated 440Hz tone for human-run listening; automation is dummy.
    raw.write_bytes(b''.join(struct.pack('<hh', *([int(3000*math.sin(2*math.pi*440*i/rate))]*2))
                             for i in range(rate*2)))
    for name, af, vf in [('fixture', 'anull', 'null'), ('video-offset', 'anull', 'settb=1/1000,setpts=PTS+125'),
                         ('audio-short', 'atrim=end=1', 'null')]:
        if rate != 48000 and name != 'fixture':
            continue
        output = folder / (f'{name}.mkv' if rate == 48000 else 'resampled.mkv')
        subprocess.run(['ffmpeg', '-nostdin', '-v', 'error', '-y', '-copyts', '-f', 'rawvideo',
                        '-pixel_format', 'rgba', '-video_size', '64x32', '-framerate', '10', '-i', str(video),
                        '-f', 's16le', '-ar', str(rate), '-ac', '2', '-i', str(raw),
                        '-af', af, '-vf', vf, '-enc_time_base', '1/1000', '-fps_mode', 'passthrough', '-c:v', 'ffv1', '-pix_fmt', 'bgr0', '-c:a', 'pcm_s16le', str(output)], check=True)
with (folder / 'dependency-audit.txt').open('w') as log:
    for args in (['ffmpeg', '-version'], ['ffmpeg', '-buildconf'], ['ffmpeg', '-L'],
                 ['pkg-config', '--modversion', 'sdl3', 'libavformat', 'libavcodec', 'libavutil', 'libswscale', 'libswresample']):
        subprocess.run(args, stdout=log, stderr=log, check=True)
