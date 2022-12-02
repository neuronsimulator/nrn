from neuron import h

if not "NRN_ENABLE_MUSIC=ON" in h.nrnversion(6):
    print("skip music_tests; NRN_ENABLE_MUSIC is not ON")
    quit()

import subprocess

# the test system copies the files in the source folder to
# .../build/test/nrnmusic/music_tests/test/music_tests
# but launches python in .../build/test/nrnmusic/musictests
# there must be a way to avoid that, but for now I'm just
# going to hit it with a sledghammer.

import os

try:
    a = os.getcwd() + "/test/music_tests"
    os.chdir(a)
except:
    pass

# Also try to find path to music binary
my_env = os.environ.copy()
try:
    my_env["PATH"] = os.getenv("MUSIC_LIBDIR") + "/../bin:" + my_env["PATH"]
except:
    pass


def run(cmd):
    result = subprocess.run(cmd, shell=True, env=my_env, capture_output=True, text=True)
    result.check_returncode()
    return result


out2 = "numprocs=1\nRank 0: Event (0, 0.001175) detected at 0\nRank 0: Event (0, 0.013825) detected at 0\n"

out3 = "numprocs=1\nnumprocs=1\ngroup= 1  rank= 0\n1 0  call stdinit\n1 0  call psolve\n1 0  done\ngroup= 2  rank= 0\n2 0  call stdinit\n2 0  call psolve\n2 0  done\n"


def chkresult(r, std):
    # lines can be out of order
    rlines = sorted(result.stdout.split("\n"))
    stdlines = sorted(std.split("\n"))
    if rlines != stdlines:
        print("standard", std, "result", r)
    assert rlines == stdlines


result = run("mpirun -np 2 music test2.music")
chkresult(result, out2)

result = run("mpirun -np 2 music test3.music")
chkresult(result, out3)
