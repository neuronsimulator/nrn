import numpy as np

from neuron.expect_hocerr import set_quiet

from function_table import k3stCell


def test_function_table():
    k3st_cell = k3stCell()
    k3st_cell.record()
    k3st_cell.simulate(1, 0.1)
    assert np.isclose([k3st_cell.record_vectors["tau1_rec"][2]], [8.1])
    assert np.isclose([k3st_cell.record_vectors["tau2_rec"][2]], [100])


if __name__ == "__main__":
    set_quiet(False)
    test_function_table()
