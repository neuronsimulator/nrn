from neuron import n

from utils.cell import Cell


class cadifusCell(Cell):
    def _create_cell(self):
        self.section = n.Section()
        self.section.insert("cadifus")
        self.section(0.001).ca_cadifus[0] = 1e-2

    def record(self):
        tvec = n.Vector()
        tvec.record(n._ref_t, sec=self.section)
        cai_vec = n.Vector()
        cai_vec.record(self.section(0.5).cadifus._ref_ca[0], sec=self.section)
        self.record_vectors["ca_ion[0]"] = cai_vec


if __name__ == "__main__":
    cadifus_cell = cadifusCell()
    cadifus_cell.record()
    cadifus_cell.simulate(1, 0.1)
    cadifus_cell.output()
    del cadifus_cell
