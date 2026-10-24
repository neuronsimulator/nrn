# A Python section deleted from HOC after its _pysec name was used: the name
# goes with it, and the section's later deallocation does not abort.
from neuron import h


class Cell:
    def __str__(self):
        return "pysec_delete_cell"


def test_pysec_delete():
    cell = Cell()
    soma = h.Section(name="soma", cell=cell)
    h("_pysec.pysec_delete_cell.soma.L = 5")
    assert soma.L == 5
    h.delete_section(sec=soma)
    del soma  # aborted in nrnpy_pysecname2sec_remove

    soma = h.Section(name="soma", cell=cell)
    h("_pysec.pysec_delete_cell.soma.L = 7")
    assert soma.L == 7


if __name__ == "__main__":
    test_pysec_delete()
