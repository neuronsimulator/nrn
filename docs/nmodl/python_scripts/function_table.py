from neuron import h
import numpy as np

from utils.cell import Cell


class k3stCell(Cell):
    def _create_cell(self):
        self.section = h.Section()
        self.section.insert("k3st")
        tau1_values = []
        voltage_values = []
        for i in range(10):
            tau1_values.append(i*0.25)
            voltage_values.append(-70+10*i)
        tau1_vector = h.Vector(tau1_values)
        voltage_vector = h.Vector(voltage_values)
        h.table_tau1_k3st(tau1_vector, voltage_vector)
        h.table_tau2_k3st(100)

    def record(self):
        tvec = h.Vector()
        tvec.record(h._ref_t, sec=self.section)
        tau1_vec = h.Vector()
        tau2_vec = h.Vector()
        tau1_vec.record(self.section(0.5).k3st._ref_tau1_rec, sec=self.section)
        tau2_vec.record(self.section(0.5).k3st._ref_tau2_rec, sec=self.section)
        self.record_vectors["tau1_rec"] = tau1_vec
        self.record_vectors["tau2_rec"] = tau2_vec


if __name__ == "__main__":
    table_cell = k3stCell()
    table_cell.record()
    table_cell.simulate(1, 0.1)
    table_cell.output()
    del table_cell
