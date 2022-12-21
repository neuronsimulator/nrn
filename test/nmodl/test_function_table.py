import os
import distutils.util

from neuron.expect_hocerr import set_quiet

import numpy as np

from function_table import k3stCell


def test_function_table():
    k3st_cell = k3stCell()
    k3st_cell.record()
    coreneuron_enable = bool(
        distutils.util.strtobool(os.environ.get("NRN_CORENEURON_ENABLE", "false"))
    )
    coreneuron_gpu = bool(
        distutils.util.strtobool(os.environ.get("CORENRN_ENABLE_GPU", "false"))
    )
    k3st_cell.simulate(1, 0.1, coreneuron_enable, coreneuron_gpu)
    assert np.isclose([k3st_cell.record_vectors["tau1_rec"][2]], [0.097022])
    assert np.isclose([k3st_cell.record_vectors["tau2_rec"][2]], [100])


if __name__ == "__main__":
    set_quiet(False)
    test_function_table()
