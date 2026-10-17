#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Generate an actual lossless encoded red/blue video, never a placeholder."""
import argparse
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    frames = bytes((255, 0, 0, 255)) * (64 * 32 * 10)
    frames += bytes((0, 0, 255, 255)) * (64 * 32 * 10)
    subprocess.run(['ffmpeg', '-nostdin', '-v', 'error', '-y', '-f', 'rawvideo',
                    '-pixel_format', 'rgba', '-video_size', '64x32', '-framerate', '10',
                    '-i', 'pipe:0', '-an', '-c:v', 'ffv1', '-pix_fmt', 'bgr0',
                    str(args.output)], input=frames, check=True)


if __name__ == '__main__':
    main()
