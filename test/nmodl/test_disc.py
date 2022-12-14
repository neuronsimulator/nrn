from neuron.expect_hocerr import set_quiet

import numpy as np

from disc import DiscCell


def test_disc():
    disc_cell = DiscCell()
    disc_cell.record()
    disc_cell.simulate(1, 0.1)

    assert np.isclose([disc_cell.record_vectors["a"][2]], [8.1])


if __name__ == "__main__":
    set_quiet(False)
    test_disc()
