from neuron import h
from neuron.expect_hocerr import expect_err, set_quiet

set_quiet(False)
pc = h.ParallelContext()


class Cell:
    def __init__(self, id):
        self.id = id
        self.secs = [h.Section(name="d_" + str(i), cell=self) for i in range(3)]
        s = self.secs
        for i in range(1, len(s)):
            s[i].connect(s[i - 1](1))
            s[i].nseg = 11
            s[i].insert("hh")

    def __str__(self):
        return "Cell_" + str(self.id)


def prroots():
    print("prroots")
    sr = h.SectionList()
    sr.allroots()
    for s in sr:
        print(s)


def prpart():
    for ith in range(pc.nthread()):
        sl = pc.get_partition(ith)
        for sec in sl:
            print(ith, sec, sec.cell().id)


def assertpart(parts="default"):
    if str(parts) == "default":  # not round-robin but root order
        roots = h.SectionList()
        roots.allroots()
        roots = [root for root in roots]
        i = 0
        for ith in range(pc.nthread()):
            sl = pc.get_partition(ith)
            for sec in sl:
                assert sec == roots[i]
                i += 1
    else:  # equal to the parts
        assert len(parts) == pc.nthread()
        for ith in range(pc.nthread()):
            sl = pc.get_partition(ith)
            a = [sec for sec in pc.get_partition(ith)]
            b = [sec for sec in parts[ith]]
            assert a == b


def test_default():
    assertpart("default")

    cells = [Cell(i) for i in range(5)]
    assertpart("default")

    pc.nthread(3)
    assertpart("default")

    pc.nthread(2)
    assertpart("default")


def test_parts():
    cells = [Cell(i) for i in range(10)]
    r = h.Random()
    r.Random123(1, 0, 0)
    nt = 3
    pc.nthread(nt)
    r.discunif(0, nt - 1)
    parts = [h.SectionList() for _ in range(nt)]
    for cell in cells:
        parts[int(r.repick())].append(cell.secs[0])
    for i in range(nt):
        pc.partition(i, parts[i])
    assertpart(parts)

    def run(tstop):
        pc.thread_ctime()  # all theads 0
        pc.set_maxstep(10)
        h.finitialize(-65)
        pc.psolve(tstop)

    run(20)
    print("ith ncell thread_ctime")
    for ith in range(pc.nthread()):
        print(ith, len([1 for _ in parts[ith]]), pc.thread_ctime(ith))


if __name__ == "__main__":
    test_default()
    test_parts()
