# tests of GUI with hoc variables that go out of scope or move

from neuron import h, gui

h(
    """
var1 = 0.0
proc act1() { print "hoc var1 = ", var1 }
proc actvec() { print "hoc vec.x[0] = ", $o1.x[0] }
proc actcell() {forall {print secname(), "  ", g_pas(.5)}}
"""
)


class Cell:
    def __init__(self, id):
        self.id = id
        s = self.soma = h.Section(name="soma", cell=self)
        s.diam = 10.0
        s.L = 100.0 / h.PI / s(0.5).diam
        s.insert("pas")
        s.g_pas = 0.001
        s.e_pas = -65.0

    def __str__(self):
        return "Cell_%d" % self.id


class GUI:
    def __init__(self, cell):
        self.cell = cell
        self.vec = h.Vector(1)  # for hoc ref variable self.vec._ref_x[0]
        self.build()
        self.map()

    def build(self):
        self.box = h.HBox()
        self.box.intercept(1)
        self.box.ref(self)

        h.xpanel("")
        h.xstatebutton("hoc var1", h._ref_var1, "act1()")
        h.xcheckbox("hoc var1", h._ref_var1, "act1()")
        h.xmenu("hoc var1")
        h.xcheckbox("hoc var1", h._ref_var1, "act1()")
        h.xmenu()
        h.xslider(h._ref_var1, 0, 1, 0, 1)
        h.xvalue("hoc var1", h._ref_var1, 1, "act1()")

        h.xstatebutton("vec.x[0]", self.vec._ref_x[0], "actvec(%s)" % self.vec)
        h.xcheckbox("vec.x[0]", self.vec._ref_x[0], "actvec(%s)" % self.vec)
        h.xmenu("vec.x[0]")
        h.xcheckbox("vec.x[0]", self.vec._ref_x[0], "actvec(%s)" % self.vec)
        h.xmenu()
        h.xslider(self.vec._ref_x[0], 0, 1, 0, 1)
        h.xvalue("vec.x[0]", self.vec._ref_x[0], 1, "actvec(%s)" % self.vec)

        h.xstatebutton(
            "cell.soma(.5).pas.g", self.cell.soma(0.5).pas._ref_g, "actcell()"
        )
        h.xcheckbox("cell.soma(.5).pas.g", self.cell.soma(0.5).pas._ref_g, "actcell()")
        h.xmenu("cell.soma(.5).pas.g")
        h.xcheckbox("cell.soma(.5).pas.g", self.cell.soma(0.5).pas._ref_g, "actcell()")
        h.xmenu()
        h.xslider(self.cell.soma(0.5).pas._ref_g, 0, 1, 0, 1)
        h.xvalue("cell.soma(.5).pas.g", self.cell.soma(0.5).pas._ref_g, 1, "actcell()")
        h.xpanel()

        self.box.intercept(0)

    def map(self):
        self.box.map("GUI test with %s" % str(self.cell))

    def act1(self, vec):
        print("hoc vec.x[0] = ", vec.x[0])


def test1():
    cells = [Cell(i) for i in range(5)]
    gui = GUI(cells[3])
    h("delete var1")  # does NOT gray out the items
    gui.vec = None
    gui.cell = None
    # sliders do not get grayed out
    return gui


if __name__ == "__main__":
    gui = test1()
