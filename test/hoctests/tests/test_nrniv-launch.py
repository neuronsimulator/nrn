import os
import shutil
import subprocess
from sys import platform

if platform == "win32":  # skip the test.
    # Cannot get subprocess.run to feed stdin to nrniv
    quit()


def nrniv(args, input):
    print(args, input)
    # nrniv is a NEURON executable, so it will be linked against any sanitizer
    # runtimes that are required => LD_PRELOAD is not needed. Delete LD_PRELOAD
    # if it is present, because if dynamic Python is enabled then nrniv will
    # run a bash subprocess, and bash + LD_PRELOAD=/path/to/libtsan.so crashes.
    env = os.environ.copy()
    try:
        del env["LD_PRELOAD"]
        print("Unset LD_PRELOAD before running nrniv")
    except KeyError:
        pass
    return subprocess.run(
        [shutil.which("nrniv")] + args,
        env=env,
        shell=False,
        input=input,
        capture_output=True,
        text=True,
        timeout=5,
    )


r = nrniv(
    args=["-isatty", "-nobanner", "-nogui", "-c", "a=5", "-"],
    input=r"""
func square() {
  return $1*$1
}
print "a = ", a
print "square(a)=", square(a)
quit()
""",
)

print("status={}".format(r.returncode))
print("stdout\n----\n{}----".format(r.stdout))
print("stderr\n----\n{}----".format(r.stderr))
assert r.returncode == 0
assert "square(a)=25" in r.stdout
if platform != "darwin":  # Mac does not print the "oc>" prompt
    assert "oc>quit()" in r.stdout
assert len(r.stderr) == 0
