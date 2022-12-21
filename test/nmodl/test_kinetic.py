import os
import distutils.util

from neuron.expect_hocerr import set_quiet

import numpy as np

from kinetic import cadifusCell


def test_kinetic():
    cadifus_cell = cadifusCell()
    cadifus_cell.record()
    coreneuron_enable = bool(
        distutils.util.strtobool(os.environ.get("NRN_CORENEURON_ENABLE", "false"))
    )
    coreneuron_gpu = bool(
        distutils.util.strtobool(os.environ.get("CORENRN_ENABLE_GPU", "false"))
    )
    cadifus_cell.simulate(1, 0.1, coreneuron_enable, coreneuron_gpu)
    assert np.isclose([cadifus_cell.record_vectors["ca_ion[0]"][2]], [5e-05])


if __name__ == "__main__":
    set_quiet(False)
    test_kinetic()
