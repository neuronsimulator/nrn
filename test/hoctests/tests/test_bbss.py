from neuron import h


class Cell:
    def __init__(self):
        self.s = h.Section("soma", self)
        self.s.insert("extracellular")


def bbsave():
    h.finitialize(-65)  # segfault if not present
    bbss = h.BBSaveState()
    bbss.save("temp.bbss")


def test_nocell():  # not wrapped in cell (no output in temp.bbss)
    s = h.Section("soma")
    s.insert("extracellular")
    bbsave()


def test_cell1():  # no gid (no output in temp.bbss)
    cell = Cell()
    bbsave()


def test_cell2():
    pc = h.ParallelContext()
    cell = Cell()
    gid = 1
    pc.set_gid2node(gid, pc.id())
    pc.cell(gid, h.NetCon(cell.s(0.5)._ref_v, None))
    bbsave()


if __name__ == "__main__":
    test_nocell()
    test_cell1()
    test_cell2()
