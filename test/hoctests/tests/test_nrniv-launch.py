from sys import platform

if platform == "win32":  # skip the test.
    # Cannot get subprocess.run to feed stdin to nrniv
    quit()

import subprocess


def srun(cmd, inp):
    print(cmd, inp)
    cp = subprocess.run(
        cmd, shell=True, input=inp, capture_output=True, text=True, timeout=5
    )
    return cp.returncode, cp.stderr, cp.stdout


r = srun(
    'nrniv -isatty -c "a=5" -',
    r"""
func square() {
  return $1*$1
}
print "a = ", a
print "square(a)=", square(a)
quit()
""",
)

assert r[0] == 0
print(r[2])
assert "square(a)=25" in r[2]
if platform != "darwin":  # Mac does not print the "oc>" prompt
    assert "oc>quit()" in r[2]
