from neuron.expect_hocerr import set_quiet

from table import TableCell

def test_table():
    table_cell = TableCell()
    table_cell.create_cell()
    table_cell.record()
    table_cell.simulate(1, 0.1)

    print(table_cell.record_vectors)
    assert len(table_cell.record_vectors) != 0

if __name__ == "__main__":
    set_quiet(False)
    test_table()
