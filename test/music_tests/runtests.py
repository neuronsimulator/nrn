from neuron import h

if not "NRN_ENABLE_MUSIC=ON" in h.nrnversion(6):
    print("skip music_tests; NRN_ENABLE_MUSIC is not ON")

import subprocess
from shutil import which

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

my_env = os.environ.copy()


def run(cmd):
    result = subprocess.run(cmd, env=my_env, shell=True, capture_output=True, text=True)
    print("PATH:", my_env["PATH"])
    if result.returncode != 0:
        print(result.stderr)
        print(result.stdout)
    result.check_returncode()
    return result


# try to find path to music binary based on MUSIC_LIBDIR
if os.getenv("MUSIC_LIBDIR") is not None:
    music_libdir = os.getenv("MUSIC_LIBDIR")
    print("MUSIC_LIBDIR:", music_libdir)
    # assume bin is in same dir as lib
    musicpath = str(os.path.dirname(music_libdir))
    print("MUSIC located in:", music_libdir)
    music_bin_path = musicpath + "/bin:"
    print("MUSIC_BINPATH:", music_bin_path)
    my_env["PATH"] = music_bin_path + my_env["PATH"]
else:
    result = which("music", path=my_env["PATH"])
    if result is not None:
        print("music exe:", result)
        musicpath = "/".join(result.strip().split("/")[:-2])
    else:
        raise Exception("music not found")

# need NRN_LIBMUSIC_PATH if mpi dynamic
if "NRN_ENABLE_MPI_DYNAMIC=ON" in h.nrnversion(6):
    print("NRN_ENABLE_MPI_DYNAMIC=ON")
    if os.getenv("NRN_LIBMUSIC_PATH") is None:
        from os.path import exists

        prefix = os.getenv("MUSIC_LIBDIR")
        name = ""
        for suffix in ["so", "dylib", "dll", None]:
            name = musicpath + "/lib/libmusic." + suffix
            if exists(name):
                break
        my_env["NRN_LIBMUSIC_PATH"] = name
    print("NRN_LIBMUSIC_PATH={}".format(my_env["NRN_LIBMUSIC_PATH"]))

out2 = "numprocs=1\nRank 0: Event (0, 0.001175) detected at 0\nRank 0: Event (0, 0.013825) detected at 0\n"

out3 = "numprocs=1\nnumprocs=1\ngroup= 1  rank= 0\n1 0  call stdinit\n1 0  call psolve\n1 0  done\ngroup= 2  rank= 0\n2 0  call stdinit\n2 0  call psolve\n2 0  done\n"


def chkresult(r, std):
    # lines can be out of order
    rlines = sorted(result.stdout.split("\n"))
    stdlines = sorted(std.split("\n"))
    if rlines != stdlines:
        print("standard", std, "result", r)
    assert rlines == stdlines


# retrieve mpiexec from environment (ctest), otherwise use musicrun
mpiexec = os.getenv("MPIEXEC_COMMAND", "musicrun 2")
result = run("{} test2.music".format(mpiexec))
chkresult(result, out2)

result = run("{} test3.music".format(mpiexec))
chkresult(result, out3)
