from neuron.units import µm, mM, ms, mV
import sys

sys.path.append("..")
from testutils import compare_data, tol


def test_multi_gridding_mix(neuron_instance):
    h, rxd, data, save_path = neuron_instance
    axon = h.Section(name="axon")
    axon.L = 2 * µm
    axon.diam = 2 * µm
    axon.nseg = 5
    axon.pt3dadd(5, 5, 0, 2)
    axon.pt3dadd(5, 5, 2, 2)

    axon2 = h.Section(name="axon2")
    axon2.L = 2 * µm
    axon2.diam = 2 * µm
    axon2.nseg = 5
    axon2.pt3dadd(0, 0, 10, 2)
    axon2.pt3dadd(0, 0, 12, 2)

    def my_initial(node):
        if node.x < 0.5:
            return 1 * mM
        else:
            return 0 * mM

    import random

    random.seed(1)

    def my_initial1(node):
        return random.random()

    cyt = rxd.Region([axon], nrn_region="i", name="cyt", dx=0.125)
    ca = rxd.Species(cyt, name="ca", charge=2, initial=my_initial1, d=1)

    cyt2 = rxd.Region([axon2], nrn_region="i", name="cyt2", dx=0.25)
    ca2 = rxd.Species(cyt2, name="ca2", charge=2, initial=my_initial, d=1)
    rate1 = rxd.Rate(ca, -ca * (1 - ca) * (0.2 - ca))
    rate2 = rxd.Rate(ca2, -ca2 * (1 - ca2) * (0.3 - ca2))

    rxd.set_solve_type(dimension=3)

    h.finitialize(-65 * mV)
    h.continuerun(0.1 * ms)

    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol
