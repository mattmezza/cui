"""Local deterministic files only: no device, network or downloads."""
import pathlib, struct, subprocess, sys
version = subprocess.check_output(['ffmpeg', '-version'], text=True).splitlines()[0]
if not version.startswith('ffmpeg version n9.0.2 '):
    raise SystemExit('fixture requires existing pinned FFmpeg n9.0.2')
out = pathlib.Path(sys.argv[1]); out.mkdir(parents=True, exist_ok=True)
def run(*args):
    subprocess.run(['ffmpeg', '-nostdin', '-v', 'error', '-y', *map(str,args)], check=True)
video = out/'video.raw'; video.write_bytes(bytes([80, 120, 180])*32*16*10)
for rate in (48000,44100):
    raw = out/f'{rate}.raw'
    samples = [(12000 if i in (0,rate//10,rate//4,rate//2-64) else 0,
                -8000 if i in (0,rate//10,rate//4,rate//2-64) else 0) for i in range(rate//2)]
    raw.write_bytes(b''.join(struct.pack('<hh',*x) for x in samples))
    for offset in ((0,125) if rate==48000 else (0,)):
        target=out/f'{rate}-{offset}.mkv'
        run('-copyts','-f','rawvideo','-pixel_format','rgb24','-video_size','32x16','-framerate','20','-i',video,
            '-f','s16le','-ar',rate,'-ac',2,'-i',raw,
            '-af',f'asetpts=PTS+{offset}/1000/TB','-c:v','ffv1','-c:a','pcm_s16le',target)
    run('-f','s16le','-ar',rate,'-ac',2,'-i',raw,'-ar',48000,'-ac',2,'-f','f32le',out/f'{rate}.f32')
(out/'broken.mkv').write_bytes(b'not a media file')
with (out/'dependency-audit.txt').open('w') as log:
    for args in (['ffmpeg','-version'],['ffmpeg','-buildconf'],['ffmpeg','-L'],['pkg-config','--modversion','libavformat','libavcodec','libavutil','libswresample']):
        subprocess.run(args, stdout=log, stderr=log, check=True)
for name, codec, af in [('compressed','flac','anull'), ('late','pcm_s16le','asetpts=PTS+1/TB'), ('discontinuous','pcm_s16le','asetpts=PTS+if(gte(T\\,0.2)\\,0.1/TB\\,0)')]:
    run('-copyts','-f','rawvideo','-pixel_format','rgb24','-video_size','32x16','-framerate',20,'-i',video,
        '-f','s16le','-ar',48000,'-ac',2,'-i',out/'48000.raw','-af',af,'-c:v','ffv1','-c:a',codec,out/f'{name}.mkv')
