from neuron import h
import subprocess
from subprocess import PIPE

pc = h.ParallelContext()
cvode = h.CVode()


def sortspikes(spiketime, gidvec):
    return sorted(zip(spiketime, gidvec))


# Set globally so we can ensure the IClamp duration is shorter
tstop = 1

# Passive cell random tree so there is some inhomogeneity of ia and im
class Cell:
    def __init__(self, id, nsec):
        r = h.Random()
        r.Random123(id, 0, 0)
        nsec += int(r.discunif(0, 4))  # for nontrivial cell_permute=1

        self.id = id
        self.secs = [h.Section(name="d" + str(i), cell=self) for i in range(nsec)]

        # somewhat random tree, d0 plays role of soma with child connections to
        # d0(.5) and all others to  1.0
        for i in range(1, nsec):
            iparent = int(r.discunif(0, i - 1))
            x = 0.5 if iparent == 0 else 1.0
            self.secs[i].connect(self.secs[iparent](x))

        # uniform L and diam but somewhat random passive g and e
        for i, sec in enumerate(self.secs):
            sec.nseg = int(r.discunif(3, 5)) if i > 0 else 1
            sec.L = 200 if i > 0 else 5
            sec.diam = 1 if i > 0 else 5
            sec.insert("pas")
            sec.g_pas = 0.0001 * r.uniform(1.0, 1.1)
            sec.e_pas = -65 * r.uniform(1.0, 1.1)
        self.secs[0].insert("hh")

        # IClamp at d2(0)
        # As i_membane_ does not include ELECTRODE_CURRENT, comparison with
        # im_axial should be done after ic completes.
        ic = h.IClamp(self.secs[2](0))
        ic.delay = 0.0
        ic.dur = min(0.2, tstop / 2)
        ic.amp = 5.0
        self.ic = ic

        # axial at every internal segment, a unique AxialPP at root
        # and at every sec(1) zero area node.
        self.axialpps = [h.AxialPP(self.secs[0](0))]
        for sec in self.secs:
            sec.insert("axial")
            self.axialpps.append(h.AxialPP(sec(1)))
        # update pointers and ri later after all cells are created

    def get_axial(self, seg):
        if 0.0 < seg.x < 1.0:
            mech = seg.axial
        else:
            mech = None
            pps = seg.point_processes()
            for pp in pps:
                if type(pp) == type(self.axialpps[0]):
                    mech = pp
                    break
        assert mech is not None
        return mech

    def __str__(self):
        return "Cell" + str(self.id)

    def ptr_helper(self, seg, mech, pseg, pmech):
        # mech and pmech for both axial and AxialPP have same rangevars.
        if pmech is None:
            mech.ri = 0.0  # indicates pointers not to be used
            # POINTER unused but must not be NULL for CoreNEURON so point to self
            mech._ref_pv = seg._ref_v
            mech._ref_pia = mech._ref_ia
            mech._ref_pim = mech._ref_im
        else:
            mech.ri = seg.ri()
            mech._ref_pv = pseg._ref_v
            mech._ref_pia = pmech._ref_ia
            mech._ref_pim = pmech._ref_im

    def update_pointers(self):
        # Probably works if every section has default orientation, i.e.
        # arc position 0 at proximal (toward the root) end.
        for i, sec in enumerate(self.secs):
            seg = sec(0)
            pseg = sec.trueparentseg()  # None only for root
            pmech = None
            if i == 0:
                assert pseg is None
                mech = self.axialpps[0]
                self.ptr_helper(seg, mech, pseg, pmech)
            else:
                # Already dealt with proximal end of section at some previous
                # iteration. But need to figure out mech
                assert pseg is not None
                mech = self.get_axial(pseg)

            pseg = seg
            pmech = mech
            for seg in sec:
                mech = seg.axial
                self.ptr_helper(seg, mech, pseg, pmech)
                pseg = seg
                pmech = mech

            # distal end of section. pseg and pmech are correct
            seg = sec(1)
            mech = self.axialpps[
                i + 1
            ]  # 0 is for rootnode, 1 is distal of root section
            self.ptr_helper(seg, mech, pseg, pmech)


class Model:
    def __init__(self, ncell, nsec):
        self.cells = [Cell(i, nsec) for i in range(ncell)]
        self.update_pointers()
        # Setup callback to update dipole POINTER for cache_efficiency
        # The PtrVector is used only to support the callback.
        self._callback_setup = h.PtrVector(1)
        self._callback_setup.ptr_update_callback(self.update_pointers)

    def update_pointers(self):
        for cell in self.cells:
            cell.update_pointers()


def srun(cmd):
    print("--------------------")
    print(cmd)
    r = subprocess.run(cmd, shell=True, stdout=PIPE, stderr=PIPE)
    if r.returncode != 0:
        print(r)
    r.check_returncode()


def runcn(args):
    import platform

    cpu = platform.machine()
    cmd = cpu + "/special-core " + args
    srun(cmd)


def test_axial():
    m = Model(5, 5)
    cvode.use_fast_imem(1)

    def result(m):
        v = []
        ia = []
        im = []
        imem = []
        loc = []

        def add(cell, seg):  # for axial and AxialPP
            mech = cell.get_axial(seg)
            loc.append((seg, mech, mech.im))
            v.append(seg.v)
            ia.append(mech.ia)
            im.append(mech.im)
            imem.append(seg.i_membrane_)

        for cell in m.cells:
            add(cell, cell.secs[0](0))
            for sec in cell.secs:
                for seg in sec:
                    add(cell, seg)
                add(cell, sec(1))

        # im == imem
        mx = max([abs(y) for y in imem])
        mx = mx if mx > 0 else 1.0
        x = sum([abs(imem[i] - y) for i, y in enumerate(im)])
        assert x / mx < 1e-9
        return v, ia, im, imem, loc

    def chk(std, result):
        # cell_permute = 1 --- im and imem addition is not associative
        # assert std == result
        for i in [0, 1, 2, 3]:
            mx = max([abs(x) for x in std[i]])
            mx = mx if mx > 0 else 1.0
            x = sum([abs(std[i][j] - x) for j, x in enumerate(result[i])])
            assert x / mx < 1e-9

    def run(tstop):
        pc.set_maxstep(10)
        h.finitialize(-65)
        pc.psolve(tstop)
        return result(m)

    std = run(tstop)

    cvode.cache_efficient(1)
    chk(std, run(tstop))

    from neuron import coreneuron

    coreneuron.verbose = 0
    coreneuron.enable = True
    coreneuron.cell_permute = 0
    chk(std, run(tstop))
    coreneuron.cell_permute = 1
    chk(std, run(tstop))
    coreneuron.enable = False

    m._callback_setup = None  # get rid of the callback first.
    del m


def test_checkpoint():
    if pc.nhost() > 1:
        return

    # clear out the old
    srun("rm -r -f coredat")

    m = Model(5, 5)
    # file mode CoreNEURON real cells need gids
    for i, cell in enumerate(m.cells):
        pc.set_gid2node(i, pc.id())
        sec = cell.secs[0]
        pc.cell(i, h.NetCon(sec(0.5)._ref_v, None, sec=sec))

    # "integrate" fabs(ia) in each axial_pp and use that as a source of spikes
    # for actually testing that the checkpoint is working with POINTER.
    # Would be much better if I knew how to get file mode coreneuron to
    # print trajectories.
    for i, p in enumerate(h.List("AxialPP")):
        pc.set_gid2node(i + 100, pc.id())
        pc.cell(i + 100, h.NetCon(p, None))

    spktime = h.Vector()
    spkgid = h.Vector()
    pc.spike_record(-1, spktime, spkgid)
    cvode.cache_efficient(1)
    pc.set_maxstep(10)
    h.finitialize(-65)
    pc.nrncore_write("coredat")

    # do a NEURON run to record spikes
    def run(tstop):
        pc.set_maxstep(10)
        h.finitialize(-65)
        pc.psolve(tstop)

    run(10)
    spikes_std = sortspikes(spktime, spkgid)

    # Does it work in direct mode?
    from neuron import coreneuron

    coreneuron.enable = True
    for perm in [0, 1]:
        coreneuron.cell_permute = perm
        run(5)
        pc.psolve(10)
        spikes = sortspikes(spktime, spkgid)
        assert spikes_std == spikes
    coreneuron.enable = False

    # standard to compare with checkpoint series
    tpnts = [5.0, 10.0]
    for perm in [0, 1]:
        print("\n\ncell_permute ", perm)
        common = "-d coredat --voltage 1000 --verbose 0 --cell-permute %d" % (perm,)
        # standard full run
        runcn(common + " --tstop %g" % float(tpnts[-1]) + " -o coredat")
        # sequence of checkpoints
        for i, tpnt in enumerate(tpnts):
            tend = tpnt
            restore = " --restore coredat/chkpnt%d" % (i,) if i > 0 else ""
            checkpoint = " --checkpoint coredat/chkpnt%d" % (i + 1,)
            outpath = " -o coredat/chkpnt%d" % (i + 1,)
            runcn(
                common + " --tstop %g" % (float(tend),) + outpath + restore + checkpoint
            )

        # compare spikes
        cmp_spks(
            spikes_std, "coredat", ["coredat/chkpnt%d" % (i,) for i in range(1, 3)]
        )

    m._callback_setup = None
    pc.gid_clear()
    del m


def cmp_spks(spikes, dir, chkpntdirs):
    # sorted nrn standard spikes into dir/out.spk
    f = open(dir + "/temp", "w")
    for spike in spikes:
        f.write("%.8g\t%d\n" % (spike[0], int(spike[1])))
    f.close()
    # sometimes roundoff to %.8g gives different sort.
    srun("sortspike {}/temp {}/nrn.spk".format(dir, dir))

    srun("sortspike {}/out.dat {}/out.spk".format(dir, dir))
    srun("cmp {}/out.spk {}/nrn.spk".format(dir, dir))

    cmd = "cat"
    for i in chkpntdirs:
        cmd = cmd + " " + i + "/out.dat"
    srun(cmd + " > " + dir + "/temp")
    srun("sortspike {}/temp {}/chkptout.spk".format(dir, dir))
    srun("cmp {}/out.spk {}/chkptout.spk".format(dir, dir))


if __name__ == "__main__":
    test_axial()
    test_checkpoint()
