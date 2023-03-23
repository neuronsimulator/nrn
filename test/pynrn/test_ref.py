# _ref_var valid as long as var in existence

from neuron import h

pc = h.ParallelContext()
cvode = h.CVode()


class Cell:  # with many random vars so easy to compare _ref_var and var
    def __init__(self, id):
        self.id = id
        r = h.Random()
        r.Random123(id, 0, 0)
        r.uniform(0.8, 1.2)

        def rv(val):
            return val * r.repick()

        refvars = []

        def ref(ob, name, val):  # val == None means don't check value
            if val is not None:
                setattr(ob, name, val)
            refvars.append(((ob, name, val), getattr(ob, "_ref_" + name)))

        soma = h.Section(name="soma", cell=self)
        dend = h.Section(name="dend", cell=self)
        dend.nseg = 5
        dend.connect(soma(0.5))

        soma.insert("hh")
        soma.L = rv(10)
        for seg in soma:
            ref(seg, "v", None)
            ref(seg, "diam", rv(10))
            ref(seg.hh, "gnabar", rv(0.12))
            ref(seg.hh, "gkbar", rv(0.036))
            ref(seg.hh, "gl", rv(0.0003))

        dend.insert("hh")
        dend.L = rv(100)
        for seg in dend:
            ref(seg, "v", None)
            ref(seg, "diam", rv(1))
            ref(seg.hh, "gnabar", rv(0.12))
            ref(seg.hh, "gkbar", rv(0.036))
            ref(seg.hh, "gl", rv(0.0003))
            if cvode.use_fast_imem():
                ref(seg, "i_membrane_", None)

        ic = h.IClamp(soma(0.5))
        ref(ic, "delay", rv(1))
        ref(ic, "dur", rv(1))
        ref(ic, "amp", rv(1))

        self.r = r
        self.refvars = refvars
        self.soma = soma
        self.dend = dend
        self.ic = ic

    def __str__(self):
        return "Cell_" + str(self.id)

    def compare(self):
        for i, item in enumerate(self.refvars):
            # deferenced handle is same as fully qualified var value
            if item[0][1] == "i_membrane_" and item[1][0] != getattr(item[0][0], item[0][1]):
                # note and update i_membrane_ failures
                x = getattr(item[0][0], "_ref_"+item[0][1])
                print(item[1], item[0][1], item[1][0], getattr(item[0][0], item[0][1]))
                print("at this moment, a new data_handle and value would be ",
                    x, x[0]);
                self.refvars[i] = (item[0], x)
            else:
                assert item[1][0] == getattr(item[0][0], item[0][1])
            # value has not changed from its initial value
            if item[0][2] is not None:
                assert item[1][0] == item[0][2]


def test_handle():
    cvode.use_fast_imem(1)

    cells = [Cell(id) for id in range(5)]

    def compare():
        print("\ncompare")
        for cell in cells:
            if cell:
                cell.compare()
        h.topology()

    compare()  # at least we start out ok

    def run(tstop):
        pc.set_maxstep(10)
        h.finitialize(-65)
        pc.psolve(3)
        compare()

    run(3)  # no permutations yet
    cells[2] = None
    run(3)
    cells[2] = Cell(10)
    run(3)
    cells = cells[2:-1]
    run(3)

    cvode.use_fast_imem(0)


if __name__ == "__main__":
    test_handle()
