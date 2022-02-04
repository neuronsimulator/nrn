from neuron import h

pc = h.ParallelContext()
cvode = h.CVode()

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
            sec.nseg = 3 if i > 0 else 1
            sec.L = 20 if i > 0 else 5
            sec.diam = 1 if i > 0 else 5
            sec.insert("pas")
            sec.g_pas = 0.0001 * r.uniform(1.0, 1.1)
            sec.e_pas = -65 * r.uniform(1.0, 1.1)

        # IClamp at d2(0)
        # As i_membane_ does not include ELECTRODE_CURRENT, comparison with
        # im_axial should be done after ic completes.
        ic = h.IClamp(self.secs[2](0))
        ic.delay = 0.0
        ic.dur = 0.9
        ic.amp = 0.001

        # axial at every internal segment, a unique AxialPP at root
        # and at every sec(1) zero area node.
        self.axialpps = [h.AxialPP(self.secs[0](0))]
        for sec in self.secs:
            sec.insert("axial")
            self.axialpps.append(h.AxialPP(sec(1)))
        # update pointers and ri later after all cells are created

    def get_axial(self, seg):
        if seg.x > 0.0 and seg.x < 1.0:
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
            mech.ri = 0.0  # indicates pointes not to be used
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
        print("update_pointers")
        for cell in self.cells:
            cell.update_pointers()


def test_axial():
    m = Model(1, 5)
    cvode.use_fast_imem(1)

    def result(m):
        v = []
        ia = []
        im = []
        imem = []

        def add(cell, seg):  # for axial and AxialPP
            mech = cell.get_axial(seg)
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
        assert sum([abs(imem[i] - x) for i, x in enumerate(im)]) < 1e-12
        return v, ia, im, imem

    def chk(std, result):
        assert std == result

    def run(tstop):
        pc.set_maxstep(10)
        h.finitialize(-65)
        pc.psolve(tstop)
        return result(m)

    std = run(1)

    cvode.cache_efficient(1)

    from neuron import coreneuron

    coreneuron.verbose = 0
    coreneuron.cell_permute = 0
    for coreneuron.enable in [False, True]:
        print("coreneuron.enable = ", coreneuron.enable)
        chk(std, run(1))


if __name__ == "__main__":
    test_axial()
