"""Run real depth/composition regressions with the Windows default stack budget."""
import resource
import subprocess
import sys


def limit_stack():
    _, hard = resource.getrlimit(resource.RLIMIT_STACK)
    resource.setrlimit(resource.RLIMIT_STACK, (1024 * 1024, hard))


for executable in sys.argv[1:]:
    subprocess.run([executable], preexec_fn=limit_stack, check=True)
print('PASS bridge depth and interaction tests with a 1 MB stack')
