from neuron.tests.utils.strtobool import strtobool
from neuron.expect_hocerr import expect_err
import os

from neuron import h

pc = h.ParallelContext()


def test_units():
    # should just emit warning
    h.nrnunit_use_legacy(0)

    # should generate an error
    expect_err("h.nrnunit_use_legacy(1)")

    s = h.Section()
    pp = h.UnitsTest(s(0.5))
    h.ion_style("na_ion", 1, 2, 1, 1, 0, sec=s)

    h.finitialize(-65)
    R_std = pp.gasconst
    erev_std = pp.erev
    ghk_std = pp.ghk

    from neuron import coreneuron

    coreneuron.enable = True
    coreneuron.gpu = bool(strtobool(os.environ.get("CORENRN_ENABLE_GPU", "false")))
    pc.set_maxstep(10)
    h.finitialize(-65)
    pc.psolve(h.dt)

    assert R_std == pp.gasconst  # mod2c needs nrnunits.lib.in
    assert abs(erev_std - pp.erev) <= (
        1e-13 if coreneuron.gpu else 0
    )  # GPU has tiny numerical differences
    assert ghk_std == pp.ghk


if __name__ == "__main__":
    test_units()
    h.quit()
