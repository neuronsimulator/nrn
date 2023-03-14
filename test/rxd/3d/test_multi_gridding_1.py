from neuron.units import µm, mM, ms, mV
import sys

sys.path.append("..")
from testutils import compare_data, tol


def test_multi_gridding_1(neuron_instance):
    h, rxd, data, save_path = neuron_instance

    axon = h.Section(name="axon")
    axon.L = 5 * µm
    axon.diam = 5 * µm
    axon.nseg = 110
    axon.pt3dadd(5, 5, 0, 5)
    axon.pt3dadd(5, 5, 5, 5)

    import random

    random.seed(1)

    def my_initial(node):
        return random.random()

    cyt = rxd.Region([axon], nrn_region="i", name="cyt", dx=0.125)
    ca = rxd.Species(cyt, name="ca", charge=2, initial=my_initial, d=1)

    magic = rxd.Rate(ca, -ca * (1 - ca) * (0.2 - ca))

    rxd.set_solve_type(dimension=3)

    h.finitialize(-65 * mV)
    h.continuerun(0.1 * ms)

    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol
