from neuron import h

from utils.cell import Cell


class DiscCell(Cell):
    def _create_cell(self):
        self.section = h.Section()
        self.section.insert("disc")
        h.k_disc = 0.1
        h.a0_disc = 10

    def record(self):
        tvec = h.Vector()
        tvec.record(h._ref_t, sec=self.section)
        avec = h.Vector()
        avec.record(self.section(0.5)._ref_a_disc, sec=self.section)
        self.record_vectors["a"] = avec


if __name__ == "__main__":
    disc_cell = DiscCell()
    disc_cell.record()
    disc_cell.simulate(1, 0.1)
    disc_cell.output()
    del disc_cell
