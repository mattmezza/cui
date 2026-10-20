#!/usr/bin/env python3
"""Rebuild original browser video/audio fixtures with local FFmpeg, without downloads."""
import hashlib
import json
import math
from pathlib import Path
import struct
import subprocess

root = Path(__file__).resolve().parent
version = subprocess.check_output(['ffmpeg', '-version'], text=True).splitlines()[0]
commands = []
files = {}
encodings = [
    ('sample.webm', ['-c:v', 'libvpx', '-b:v', '140k', '-deadline', 'best',
                     '-c:a', 'libopus', '-b:a', '24k']),
    ('sample.mp4', ['-c:v', 'libx264', '-crf', '30', '-movflags', '+faststart',
                    '-c:a', 'aac', '-b:a', '48k']),
]
for filename, codecs in encodings:
    # FFmpeg sine's default peak is 1/8 full scale; volume 1/4 gives about
    # -30 dBFS peak. Short fades avoid clicks at the beginning/end of the loop.
    command = ['ffmpeg', '-hide_banner', '-loglevel', 'error', '-y',
               '-f', 'lavfi', '-i', 'testsrc2=size=320x180:rate=15:duration=12',
               '-f', 'lavfi', '-i', 'sine=frequency=440:sample_rate=48000:duration=12',
               '-map', '0:v:0', '-map', '1:a:0',
               '-af', 'volume=0.25,afade=t=in:st=0:d=0.1,afade=t=out:st=11.9:d=0.1',
               '-t', '12', '-fflags', '+bitexact', '-flags:v', '+bitexact',
               '-flags:a', '+bitexact', '-map_metadata', '-1', '-pix_fmt', 'yuv420p',
               '-threads', '1', *codecs, filename]
    subprocess.run(command, cwd=root, check=True)
    commands.append(command)
    probe_command = ['ffprobe', '-v', 'error', '-show_entries',
                     'stream=codec_type,codec_name,width,height,sample_rate,channels,duration,nb_frames:format=duration',
                     '-of', 'json', filename]
    probe = json.loads(subprocess.check_output(probe_command, cwd=root))
    video = [s for s in probe['streams'] if s['codec_type'] == 'video']
    audio = [s for s in probe['streams'] if s['codec_type'] == 'audio']
    assert len(video) == len(audio) == 1
    assert (video[0]['width'], video[0]['height']) == (320, 180)
    assert audio[0]['codec_name'] == ('opus' if filename.endswith('webm') else 'aac')
    assert abs(float(probe['format']['duration']) - 12) < 0.1
    decode_command = ['ffmpeg', '-v', 'error', '-i', filename, '-map', '0:a:0',
                      '-ac', '1', '-ar', '48000', '-f', 's16le', '-']
    pcm = subprocess.check_output(decode_command, cwd=root)
    samples = [v[0] for v in struct.iter_unpack('<h', pcm)]
    peak = max(abs(v) for v in samples)
    rms = math.sqrt(sum(v*v for v in samples) / len(samples))
    assert 11.9 * 48000 <= len(samples) <= 12.1 * 48000
    assert 0 < rms < 2000 and 0 < peak < 4096
    data = (root / filename).read_bytes()
    assert len(data) <= 512 * 1024
    files[filename] = {
        'sha256': hashlib.sha256(data).hexdigest(), 'bytes': len(data),
        'probe_command': probe_command, 'probe': probe,
        'audio_decode_command': decode_command,
        'decoded_audio': {'format': 'mono signed 16-bit little-endian PCM, 48000 Hz',
                          'samples': len(samples), 'peak': peak, 'rms': round(rms, 6),
                          'sha256': hashlib.sha256(pcm).hexdigest()},
    }
(root / 'receipt.json').write_text(json.dumps({
    'origin': 'Original FFmpeg lavfi testsrc2 and quiet 440 Hz sine; no external media',
    'license': 'CC0-1.0', 'tool': version, 'commands': commands,
    'audio': {'source': '440 Hz mono sine, 48000 Hz, 12 seconds',
              'gain': 0.25, 'approximate_source_peak_dbfs': -30.1,
              'fade_in_seconds': 0.1, 'fade_out_seconds': 0.1,
              'note': 'Encoded streams may include codec delay/padding; decoded PCM verified nonzero.'},
    'files': files}, indent=2) + '\n')
