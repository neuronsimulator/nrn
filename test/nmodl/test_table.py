import os
import distutils.util

from neuron.expect_hocerr import set_quiet

from table import TableCell


def test_table():
    table_cell = TableCell()
    table_cell.record()
    coreneuron_enable = bool(
        distutils.util.strtobool(os.environ.get("NRN_CORENEURON_ENABLE", "false"))
    )
    coreneuron_gpu = bool(
        distutils.util.strtobool(os.environ.get("CORENRN_ENABLE_GPU", "false"))
    )
    table_cell.simulate(1, 0.1, coreneuron_enable, coreneuron_gpu)

    print(table_cell.record_vectors)
    assert len(table_cell.record_vectors) != 0


if __name__ == "__main__":
    set_quiet(False)
    test_table()
