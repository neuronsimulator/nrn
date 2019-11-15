import pytest
from testutils import tol, compare_data


def test_pure_diffusion(neuron_instance):
    #expected_data_filename = 'pure_diffusion.dat'
    h, rxd, data = neuron_instance
    dend = h.Section()
    dend.diam = 2
    dend.nseg = 101
    dend.L = 100

    diff_constant = 1

    r = rxd.Region([dend])
    print('==============')
    print(rxd.rxd.section1d._rxd_sec_lookup)
    print(r._secs1d[0].nseg)
    print(rxd.species._all_defined_species)
    print(rxd.species._all_defined_species[0]())
    ca = rxd.Species(r, d=diff_constant,
                     initial=lambda node: 1 if 0.4 < node.x < 0.6 else 0)

    h.finitialize()

    for t in [25, 50, 75, 100, 125]:
        h.continuerun(t)
    max_err = compare_data(data)

    assert(max_err < tol)

def test_pure_diffusion_cvode(neuron_instance):
    #expected_data_filename = 'pure_diffusion_cvode.dat'
    h, rxd, data = neuron_instance
    dend = h.Section()
    dend.diam = 2
    dend.nseg = 101
    dend.L = 100

    diff_constant = 1

    r = rxd.Region(h.allsec())
    ca = rxd.Species(r, d=diff_constant, initial=lambda node: 1 if 0.4 < node.x < 0.6 else 0)

    # enable CVode and set atol
    h.CVode().active(1)
    h.CVode().atol(1e-6)

    h.finitialize()

    for t in [25, 50, 75, 100, 125]:
        h.continuerun(t)
    max_err = compare_data(data)

    assert(max_err < tol)

