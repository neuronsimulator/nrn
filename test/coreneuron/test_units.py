import os
import traceback

enable_gpu = bool(os.environ.get("CORENRN_ENABLE_GPU", ""))

from neuron import h

pc = h.ParallelContext()


def test_units():
    s = h.Section()
    pp = h.UnitsTest(s(0.5))
    h.ion_style("na_ion", 1, 2, 1, 1, 0, sec=s)

    h.finitialize(-65)
    R_std = pp.gasconst
    erev_std = pp.erev
    ghk_std = pp.ghk

    from neuron import coreneuron

    h.CVode().cache_efficient(1)
    coreneuron.enable = True
    coreneuron.gpu = enable_gpu
    pc.set_maxstep(10)
    h.finitialize(-65)
    pc.psolve(h.dt)

    assert R_std == pp.gasconst  # mod2c needs nrnunits.lib.in
    assert abs(erev_std - pp.erev) <= (
        1e-13 if coreneuron.gpu else 0
    )  # GPU has tiny numerical differences
    assert ghk_std == pp.ghk


if __name__ == "__main__":
    try:
        model = test_units()
    except:
        traceback.print_exc()
        # Make the CTest test fail
        sys.exit(42)
    # The test doesn't exit without this.
    if enable_gpu:
        h.quit()
