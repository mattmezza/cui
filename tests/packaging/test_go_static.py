#!/usr/bin/env python3
"""Build a real source-module consumer, then run it without checkout or tools."""
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[2]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--artifacts', type=Path, required=True)
    parser.add_argument('--native', action='store_true')
    parser.add_argument('--font', type=Path)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='cui-go-static-consumer-') as temp:
        clean = Path(temp)
        env = dict(os.environ)
        for key in ('CUI_BRIDGE_LIBRARY','CUI_BRIDGE_LIB_DIR','LD_LIBRARY_PATH','GOPRIVATE','GONOPROXY','GONOSUMDB','CUI_PACKAGE_SMOKE_BACKEND','DISPLAY','WAYLAND_DISPLAY','SDL_VIDEODRIVER','MIR_SOCKET','XDG_RUNTIME_DIR'):
            env.pop(key, None)
        env.update(GOPROXY=args.artifacts.resolve().as_uri(), GOSUMDB='off', CGO_ENABLED='1',
                   GOMODCACHE=str(clean/'modules'), GOCACHE=str(clean/'build-cache'), XDG_CACHE_HOME=str(clean/'cache'))
        (clean/'go.mod').write_text('module example.local/cui-counter\n\ngo 1.23\n\nrequire github.com/mattmezza/cui/bindings/go v0.1.0-alpha.3\n')
        shutil.copy2(ROOT/'tests/packaging/go_smoke.go', clean/'main.go')
        subprocess.run(['go','mod','tidy'],cwd=clean,env=env,check=True)
        tags = 'cui_native' if args.native else ''
        subprocess.run(['go','build','-tags',tags,'-o','app','.'],cwd=clean,env=env,check=True)
        # Compile an incompatible ABI fixture exporting the complete loader table.
        module = clean/'modules/github.com/mattmezza/cui/bindings/go@v0.1.0-alpha.3'
        table = (module/'cui/native.go').read_text().split('#define API(X, V)',1)[1].split('#define DECL',1)[0]
        functions = re.findall(r'[XV]\((\w+), ([^,]+), (\([^\n]*?\)), \([^\n]*?\)\)',table)
        source = '#include "cui/cui_bridge.h"\n'
        for name, ret, params in functions:
            body = 'return;' if ret == 'void' else 'return 999;' if name == 'abi_version' else 'return sizeof(cui_bridge_options);' if name == 'options_size' else f'{ret} fixture_return_value = {{0}}; return fixture_return_value;'
            source += f'{ret} cui_bridge_{name}{params} {{ {body} }}\n'
        (clean/'wrong-abi.c').write_text(source)
        subprocess.run(['cc','-std=c11','-shared','-fPIC','-I'+str(module/'sdk/source/include'),str(clean/'wrong-abi.c'),'-o',str(clean/'wrong-abi.so')],env=env,check=True)
        (clean/'modules').rename(clean/'unavailable-modules')
        (clean/'build-cache').rename(clean/'unavailable-build-cache')
        blocked = clean/'blocked'; blocked.mkdir()
        for name in ('cmake','cc','gcc','clang','make','ninja','pkg-config'):
            tool=blocked/name;tool.write_text('#!/bin/sh\necho forbidden runtime compiler >&2\nexit 99\n');tool.chmod(0o755)
        env['PATH']=str(blocked)+os.pathsep+env['PATH']
        runtime = clean/'runtime'; runtime.mkdir(mode=0o700)
        env['XDG_RUNTIME_DIR'] = str(runtime)
        command = [str(clean/'app')]
        if args.native:
            if not args.font or not args.font.is_file(): raise SystemExit('--native requires an existing --font')
            env.update(CUI_PACKAGE_SMOKE_BACKEND='sdl', CUI_PACKAGE_SMOKE_FONT=str(args.font.resolve()), SDL_VIDEODRIVER='x11')
            command=['xvfb-run','-a','env','-u','WAYLAND_DISPLAY',*command]
        subprocess.run(command,cwd=clean,env=env,check=True)
        if (clean/'cache/cui-bridge').exists(): raise SystemExit('Static App unexpectedly created a runtime build/library cache')
        for path, diagnostic in ((clean/'missing.so',b'missing.so'),(clean/'wrong-abi.so',b'ABI mismatch')):
            bad=dict(env,CUI_BRIDGE_LIBRARY=str(path))
            failed=subprocess.run(command,cwd=clean,env=bad,capture_output=True)
            if failed.returncode==0 or diagnostic not in failed.stderr: raise SystemExit('Explicit override failure was hidden: '+failed.stderr.decode())
        print('source-module static Go runtime and override/ABI checks passed')
if __name__=='__main__': main()
