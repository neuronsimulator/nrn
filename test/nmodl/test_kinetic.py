import numpy as np

from neuron.expect_hocerr import set_quiet

from kinetic import cadifusCell


def test_kinetic():
    cadifus_cell = cadifusCell()
    cadifus_cell.record()
    cadifus_cell.simulate(1, 0.1)
    assert np.isclose([cadifus_cell.record_vectors["ca_ion[0]"][2]], [5e-05])


if __name__ == "__main__":
    set_quiet(False)
    test_function_table()
