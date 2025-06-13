# augmented to also checkpoint verify when RANDOM is permuted
from neuron.tests.utils.strtobool import strtobool
import os

from neuron import h, coreneuron

pc = h.ParallelContext()


class Cell:
    def __init__(self, gid):
        self.soma = h.Section(name="soma", cell=self)
        self.soma.insert("noisychan")
        if gid % 2 == 0:
            # CoreNEURON permutation not the identity if cell topology not homogeneous
            self.dend = h.Section(name="dend", cell=self)
            self.dend.connect(self.soma(0.5))
            self.dend.insert("noisychan")
        self.gid = gid
        pc.set_gid2node(gid, pc.id())
        pc.cell(gid, h.NetCon(self.soma(0.5)._ref_v, None, sec=self.soma))


def model():
    nslist = [h.NetStim() for _ in range(3)]
    cells = [Cell(gid) for gid in range(10, 15)]
    for gid, ns in enumerate(nslist):
        ns.start = 0
        ns.number = 1e9
        ns.interval = 1
        ns.noise = 1
        pc.set_gid2node(gid, pc.id())
        pc.cell(gid, h.NetCon(ns, None))
    spiketime = h.Vector()
    spikegid = h.Vector()
    pc.spike_record(-1, spiketime, spikegid)
    return nslist, spiketime, spikegid, cells


def pspike(m):
    print("spike raster")
    for i, t in enumerate(m[1]):
        print("%.5f %g" % (t, m[2][i]))


def run(tstop, m):
    pc.set_maxstep(10)
    h.finitialize(-65)
    pc.psolve(tstop)


def chk(std, m):
    assert std[0].eq(m[1])
    assert std[1].eq(m[2])


def test_embeded_run():
    m = model()
    run(5, m)
    std = [m[1].c(), m[2].c()]
    pc.psolve(7)
    std2 = [m[1].c(), m[2].c()]

    coreneuron.enable = True
    coreneuron.verbose = 0
    coreneuron.gpu = bool(strtobool(os.environ.get("CORENRN_ENABLE_GPU", "false")))
    run(5, m)
    chk(std, m)

    coreneuron.enable = False
    pc.psolve(7)
    chk(std2, m)


def sortspikes(spiketime, gidvec):
    return sorted(zip(spiketime, gidvec))


def test_chkpnt():
    import shutil, os, platform, subprocess

    # clear out the old if any exist
    shutil.rmtree("coredat", ignore_errors=True)

    m = model()

    # std spikes from 0-5 and 5-10
    run(5, m)
    std1 = [m[1].c(), m[2].c()]
    m[1].resize(0)
    m[2].resize(0)
    pc.psolve(10)
    std2 = [m[1].c(), m[2].c()]
    pspike(m)

    # Files for coreneuron runs
    h.finitialize(-65)
    pc.nrncore_write("coredat")

    def runcn(tstop, perm, args):
        exe = os.path.join(os.getcwd(), platform.machine(), "special-core")
        common = [
            "-d",
            "coredat",
            "--voltage",
            "1000",
            "--verbose",
            "0",
            "--cell-permute",
            str(perm),
        ]

        gpu_run = bool(strtobool(os.environ.get("CORENRN_ENABLE_GPU", "false")))
        if gpu_run:
            common.append("--gpu")

        cmd = [exe] + ["--tstop", "{:g}".format(tstop)] + common + args
        print(" ".join(cmd))
        subprocess.run(
            cmd,
            check=True,
            shell=False,
        )

    runcn(5, 1, ["--checkpoint", "coredat/chkpnt", "-o", "coredat"])
    runcn(10, 1, ["--restore", "coredat/chkpnt", "-o", "coredat/chkpnt"])
    cmp_spks(sortspikes(std2[0], std2[1]), "coredat")

    # cleanup
    shutil.rmtree("coredat", ignore_errors=True)


def cmp_spks(spikes, dir):  # modified from test_pointer.py
    import os, subprocess, shutil

    # sorted nrn standard spikes into dir/out.spk
    with open(os.path.join(dir, "temp"), "w") as f:
        for spike in spikes:
            f.write("{:.8g}\t{}\n".format(spike[0], int(spike[1])))

    # sometimes roundoff to %.8g gives different sort.
    def help(cmd, name_in, name_out):
        # `cmd` is some generic utility, which does not need to have a
        # sanitizer runtime pre-loaded. LD_PRELOAD=/path/to/libtsan.so can
        # cause problems for *nix utilities, so drop it if it was present.
        env = os.environ.copy()
        try:
            del env["LD_PRELOAD"]
        except KeyError:
            pass
        subprocess.run(
            [
                shutil.which(cmd),
                os.path.join(dir, name_in),
                os.path.join(dir, name_out),
            ],
            check=True,
            env=env,
            shell=False,
        )

    help("sortspike", "temp", "nrn.spk")
    help("sortspike", "chkpnt/out.dat", "chkpnt/out.spk")
    help("cmp", "chkpnt/out.spk", "nrn.spk")


if __name__ == "__main__":
    test_embeded_run()
    test_chkpnt()
