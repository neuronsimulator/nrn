# BEFORE/AFTER must run, including a POINT_PROCESS POINTER.
# Counts are for one segment after h.finitialize(-65) and one h.fadvance().
# finitialize: BEFORE INITIAL, AFTER INITIAL, one BEFORE BREAKPOINT, one BEFORE STEP.
# fadvance adds one BEFORE BREAKPOINT, one AFTER SOLVE, and one BEFORE STEP.
# The point process INITIAL block zeros n, and BEFORE STEP runs after that.

from neuron import h


def test_beforestep():
    s = h.Section("soma")
    s.nseg = 1
    s.insert("befstep")
    mech = s(0.5).befstep

    pp = h.BefstepPP(s(0.5))
    watched = h.Vector(1)
    h.setpointer(watched._ref_x[0], "p", pp)

    h.finitialize(-65)

    assert mech.nbi == 1, mech.nbi
    assert mech.nai == 1, mech.nai
    assert mech.nbb == 1, mech.nbb
    assert mech.nas == 0, mech.nas
    assert mech.nstep == 1, mech.nstep
    assert mech.vsave == -65, mech.vsave
    assert mech.tsave == h.t, (mech.tsave, h.t)
    assert pp.n == 1, pp.n
    assert pp.nverb == 1, pp.nverb
    assert watched.x[0] == 1, watched.x[0]

    h.fadvance()

    assert mech.nbi == 1, mech.nbi
    assert mech.nai == 1, mech.nai
    assert mech.nbb == 2, mech.nbb
    assert mech.nas == 1, mech.nas
    assert mech.nstep == 2, mech.nstep
    assert mech.vsave == -65, mech.vsave
    assert mech.tsave == h.t, (mech.tsave, h.t)
    assert pp.n == 2, pp.n
    assert pp.nverb == 2, pp.nverb
    assert watched.x[0] == 2, watched.x[0]


if __name__ == "__main__":
    test_beforestep()
