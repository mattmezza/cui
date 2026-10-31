"""Run real depth/composition regressions with a bounded stack budget."""
import argparse
import resource
import subprocess


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--stack-kib', type=int, default=1024)
parser.add_argument('executables', nargs='+')
args = parser.parse_args()
if not 64 <= args.stack_kib <= 1024:
    parser.error('--stack-kib must be between 64 and 1024')


def limit_stack():
    _, hard = resource.getrlimit(resource.RLIMIT_STACK)
    resource.setrlimit(resource.RLIMIT_STACK, (args.stack_kib * 1024, hard))


for executable in args.executables:
    subprocess.run([executable], preexec_fn=limit_stack, check=True)
print(f'PASS bridge depth and interaction tests with a {args.stack_kib} KiB stack')
