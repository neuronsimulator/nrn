import os
from neuron.tests.utils.strtobool import strtobool

from neuron.expect_hocerr import set_quiet

import numpy as np

from disc import DiscCell


def test_disc():
    disc_cell = DiscCell()
    disc_cell.record()
    coreneuron_enable = bool(
        strtobool(os.environ.get("NRN_CORENEURON_ENABLE", "false"))
    )
    coreneuron_gpu = bool(strtobool(os.environ.get("CORENRN_ENABLE_GPU", "false")))
    disc_cell.simulate(1, 0.1, coreneuron_enable, coreneuron_gpu)

    assert np.isclose([disc_cell.record_vectors["a"][2]], [8.1])


if __name__ == "__main__":
    set_quiet(False)
    test_disc()
