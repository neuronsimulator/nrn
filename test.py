from neuron import h

pc = h.ParallelContext()

# Default Node order for simulation given a particular construction order.
# The beginning of wisdom...


class Cell:
    def __init__(self, id):
        self.id = id
        d = self.den = h.Section(name="d", cell=self)
        d.nseg = id
        for i, seg in enumerate(d.allseg()):
            seg.v = i + 10 * id

        pc.set_gid2node(id, pc.id())
        pc.cell(id, h.NetCon(d(1)._ref_v, None, sec=d))

    def __str__(self):
        return "Cell_%d" % self.id


cells = [Cell(4 - id) for id in range(1, 4)]


def printv():
    h.psection()  # call to v_setup_vectors side effect
    s = {}  # associate root sections with thread id.
    for i in range(pc.nthread()):
        for sec in pc.get_partition(i):
            s[sec] = i
    print(s)
    for sec in h.allsec():
        for seg in sec.allseg():
            print(seg, seg.v, seg.node_index(), str(s[sec] if sec in s else ""))


# printv()

# Iteration order (sec,seg) is associated with construction order
# (provided nseg set after each section construction?)
# and the only difference in simulation order is that all root nodes are at the beginning.
# (provided there is only one thread)

"""
Cell_1.d(0) 10.0 0
Cell_1.d(0.5) 11.0 3
Cell_1.d(1) 12.0 4
Cell_2.d(0) 20.0 1
Cell_2.d(0.25) 21.0 5
Cell_2.d(0.75) 22.0 6
Cell_2.d(1) 23.0 7
Cell_3.d(0) 30.0 2
Cell_3.d(0.166667) 31.0 8
Cell_3.d(0.5) 32.0 9
Cell_3.d(0.833333) 33.0 10
Cell_3.d(1) 34.0 11
"""

##################################


def set_nthread(n):
    pc.nthread(n)
    print("nthread %d" % pc.nthread())
    printv()


"""
set_nthread(2)  # this one is a puzzle as cell 1 and 2 are in thread 0
# By round robin, I expected that 1 and 3 would be in
# thread 0.
set_nthread(3)
set_nthread(1)
"""

pc.optimize_node_order(1)
set_nthread(1)
