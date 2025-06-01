from neuron import n

from utils.cell import Cell


class hhCellIClamp(Cell):
    def _create_cell(self):
        h("""create soma""")
        n.load_file("stdrun.hoc")
        n.soma.L = 5.6419
        n.soma.diam = 5.6419
        n.soma.insert("hh")
        ic = n.IClamp(n.soma(0.5))
        ic.delay = 0.5
        ic.dur = 0.1
        ic.amp = 0.3

    def record(self):
        v = n.Vector()
        v.record(n.soma(0.5)._ref_v, sec=n.soma)
        tv = n.Vector()
        tv.record(h._ref_t, sec=n.soma)
        nc = n.NetCon(n.soma(0.5)._ref_v, None, sec=n.soma)
        spikestime = n.Vector()
        nc.record(spikestime)
        self.record_vectors["spikes"] = spikestime.to_python()


if __name__ == "__main__":
    hh_IClamp_cell = hhCellIClamp()
    hh_IClamp_cell.record()
    hh_IClamp_cell.simulate(1, 0.1)
    hh_IClamp_cell.output()
    h.delete_section(sec=n.soma)
    del hh_IClamp_cell
