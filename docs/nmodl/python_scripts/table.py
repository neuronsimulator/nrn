from neuron import h

from utils.cell import Cell


class TableCell(Cell):
    def _create_cell(self):
        self.section = h.Section()
        self.section.insert("table")

    def record(self):
        tvec = h.Vector()
        tvec.record(h._ref_t, sec=self.section)
        avec = h.Vector()
        avec.record(self.section(0.5)._ref_ainf_table, sec=self.section)
        self.record_vectors["ainf"] = avec


if __name__ == "__main__":
    table_cell = TableCell()
    table_cell.record()
    table_cell.simulate(1, 0.1)
    table_cell.output()
    del table_cell
