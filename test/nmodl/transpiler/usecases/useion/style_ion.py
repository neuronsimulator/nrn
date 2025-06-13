import numpy as np
from neuron import gui
from neuron import h


def nernst(ca):
    return h.nernst(ca.cai, ca.cao, h.ion_charge("ca_ion"))


def simulate(mech_name, compute_eca=nernst):
    s = h.Section()
    s.insert(mech_name)
    s.nseg = 1

    h.stdinit()

    ca = s(0.5).ca_ion
    eca_expected = compute_eca(ca)

    assert (
        np.abs(ca.eca - eca_expected) < 1e-10
    ), f"{ca.eca} - {eca_expected} = {ca.eca - eca_expected}"
