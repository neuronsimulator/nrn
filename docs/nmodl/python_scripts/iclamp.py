from neuron import h

from utils.cell import Cell


class hhCellIClamp(Cell):
    def _create_cell(self):
        h("""create soma""")
        h.load_file("stdrun.hoc")
        h.soma.L = 5.6419
        h.soma.diam = 5.6419
        h.soma.insert("hh")
        ic = h.IClamp(h.soma(0.5))
        ic.delay = 0.5
        ic.dur = 0.1
        ic.amp = 0.3

    def record(self):
        v = h.Vector()
        v.record(h.soma(0.5)._ref_v, sec=h.soma)
        tv = h.Vector()
        tv.record(h._ref_t, sec=h.soma)
        nc = h.NetCon(h.soma(0.5)._ref_v, None, sec=h.soma)
        spikestime = h.Vector()
        nc.record(spikestime)
        self.record_vectors["spikes"] = spikestime.to_python()


if __name__ == "__main__":
    hh_IClamp_cell = hhCellIClamp()
    hh_IClamp_cell.record()
    hh_IClamp_cell.simulate(1, 0.1)
    hh_IClamp_cell.output()
    h.delete_section(sec=h.soma)
    del hh_IClamp_cell
