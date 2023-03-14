from neuron.units import µm, mM, ms, mV
import sys

sys.path.append("..")

from testutils import compare_data, tol


def test_multi_gridding_2(neuron_instance):
    h, rxd, data, save_path = neuron_instance

    axon2 = h.Section(name="axon2")
    axon2.L = 5 * µm
    axon2.diam = 5 * µm
    axon2.nseg = 110
    axon2.pt3dadd(0, 0, 10, 5)
    axon2.pt3dadd(0, 0, 15, 5)

    def my_initial(node):
        if node.x < 0.5:
            return 1 * mM
        else:
            return 0 * mM

    cyt2 = rxd.Region([axon2], nrn_region="i", name="cyt2", dx=0.25)
    ca2 = rxd.Species(cyt2, name="ca2", charge=2, initial=my_initial, d=1)

    magic2 = rxd.Rate(ca2, -ca2 * (1 - ca2) * (0.3 - ca2))

    rxd.set_solve_type(dimension=3)

    h.finitialize(-65 * mV)
    h.continuerun(0.1 * ms)

    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol
