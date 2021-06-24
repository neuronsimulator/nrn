from neuron import h, crxd as rxd
import numpy
import __main__

name = __main__.__file__
if name[-3:] == ".py":
    name = name[:-3]
h.load_file("stdrun.hoc")

dend1 = h.Section()
dend1.diam = 2
dend1.nseg = 101
dend1.L = 50

dend2 = h.Section()
dend2.diam = 2
dend2.nseg = 101
dend2.L = 50
dend2.connect(dend1)

diff_constant = 1
h.cvode_active(True)
r = rxd.Region(h.allsec(), dx=0.25)

rxd.set_solve_type([dend1], dimension=3)

ca = rxd.Species(
    r,
    d=diff_constant,
    atolscale=0.1,
    initial=lambda node: 1
    if (0.8 < node.x and node.segment in dend1)
    or (node.x < 0.2 and node.segment in dend2)
    else 0,
)
bistable_reaction = rxd.Rate(ca, -ca * (1 - ca) * (0.01 - ca))
h.finitialize()

if __name__ == "__main__":
    from matplotlib import pyplot

    for t in [25, 50, 75, 100, 125]:
        h.continuerun(t)
        pyplot.plot(
            [nd.x for nd in ca.nodes if nd.segment in dend1]
            + [1 + nd.x for nd in ca.nodes if nd.segment in dend2],
            [nd.concentration for nd in ca.nodes if nd.segment in dend1]
            + [nd.concentration for nd in ca.nodes if nd.segment in dend2],
            ".",
        )
    pyplot.tight_layout()
    pyplot.savefig("{0}.png".format(name))
    pyplot.show()

else:
    for t in [25, 50, 75, 100, 125]:
        h.continuerun(t)
