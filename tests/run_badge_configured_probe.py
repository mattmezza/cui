# SPDX-License-Identifier: MIT
# Focused compilation only; runtime must use scripts/test_local.py.
import subprocess,pathlib,hashlib,argparse
parser=argparse.ArgumentParser()
parser.add_argument('--build-dir', required=True)
parser.add_argument('--libraries', required=True)
args=parser.parse_args()
out=pathlib.Path(args.build_dir)/"badge-configured"
out.mkdir(parents=True,exist_ok=True)
root=pathlib.Path(__file__).resolve().parents[1]
libs=pathlib.Path(args.libraries)
flags=['cc','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Wpedantic','-Werror','-DCUI_HAS_TEXT=1','-I'+str(root/'include')]
objects=[]
for i,src in enumerate(['src/components/cui_components.c','tests/unit/test_badge_configured_icon.c','tests/reference/badge_b4cd6a8f.c']):
 obj=out/(str(i)+'.o'); subprocess.run(flags+['-c',str(root/src),'-o',str(obj)],check=True);objects.append(str(obj))
archives=[libs/x for x in ['libcui.a','libcui_software.a','libcui_text.a','libcui.a','libcui_bidi.a']]
for a in archives: print(a,hashlib.sha256(a.read_bytes()).hexdigest(),flush=True)
subprocess.run(['cc']+objects+[str(x) for x in archives]+['-lm','/usr/lib/libharfbuzz.so','/usr/lib/libfreetype.so','-o',str(out/'test_badge')],check=True)
(out/'CTestTestfile.cmake').write_text('add_test(badge_configured_icon "'+str(out/'test_badge')+'")\n')
