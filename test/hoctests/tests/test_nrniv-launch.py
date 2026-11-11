import os
import shutil
import subprocess
from sys import platform

import pytest


def nrniv(args, input):
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


@pytest.mark.skipif(
    platform == "win32",
    reason="Cannot get subprocess.run to feed stdin to nrniv",
)
def test_nrniv():
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
    assert len(r.stderr) == 0
    # GNU readline echoes the typed line, including the oc> prompt. libedit
    # (macOS, and static-libedit wheels) runs the same hoc and does not copy
    # that line into stdout. Require the echo only when a prompt is present.
    if "oc>" in r.stdout:
        assert "oc>quit()" in r.stdout


if __name__ == "__main__":
    test_nrniv()
