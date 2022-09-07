from neuron import h
import numpy as np

from utils.cell import Cell

class TableCell(Cell):

    def create_cell(self):
        self.section = h.Section()
        self.section.insert("table")

    def record(self):
        tvec = h.Vector()
        tvec.record(h._ref_t, sec=self.section)
        avec = h.Vector()
        avec.record(self.section(0.5)._ref_ainf_table, sec=self.section)
        self.record_vectors["ainf"] = avec

if __name__ == '__main__':
    disc_cell = TableCell()
    disc_cell.create_cell()
    disc_cell.record()
    disc_cell.simulate(1, 0.1)
    disc_cell.output()
