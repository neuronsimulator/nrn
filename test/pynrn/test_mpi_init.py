from neuron import h
import sys
import subprocess


def srun(cmd):
    print("--------------------")
    print(cmd)
    subprocess.run(cmd, shell=True).check_returncode()


# test various ways to launch nrniv and python that can MPI_INIT
def test_mpi_init():
    python = sys.executable + " "
    nrniv = "nrniv "
    nompi = "NRN_ENABLE_MPI=OFF" in h.nrnversion(6)
    mpiexec = "mpiexec -n 1 "
    env = "env NEURON_INIT_MPI=1 "
    if not nompi:
        srun(mpiexec + nrniv + "-mpi -c 'quit()'")
        srun(mpiexec + python + "-c 'from neuron import h; h.nrnmpi_init(); h.quit()'")
        srun(env + mpiexec + python + "-c 'from neuron import h; h.quit()'")
    srun(nrniv + "-mpi -c 'quit()'")
    srun(nrniv + "-c 'nrnmpi_init()' -c 'quit()'")
    srun(python + "-c 'from neuron import h; h.nrnmpi_init(); h.quit()'")
    srun(env + python + "-c 'from neuron import h; h.quit()'")


if __name__ == "__main__":
    test_mpi_init()
