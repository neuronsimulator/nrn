"""test of pure diffusion

Robert A McDougal
March 2013 - June 2014
"""

from neuron import h, crxd as rxd
import numpy
import __main__

name = __main__.__file__
if name[-3:] == ".py":
    name = name[:-3]

h.load_file("stdrun.hoc")

dend = h.Section()
dend.diam = 2
dend.nseg = 101
dend.L = 100

diff_constant = 1

r = rxd.Region(h.allsec())
ca = rxd.Species(
    r, d=diff_constant, initial=lambda node: 1 if 0.4 < node.x < 0.6 else 0
)

h.finitialize()

if __name__ == "__main__":
    from matplotlib import pyplot

    for t in [25, 50, 75, 100, 125]:
        h.continuerun(t)
        pyplot.plot([seg.x for seg in dend], ca.states)

    pyplot.tight_layout()
    pyplot.savefig("{0}.png".format(name))
else:
    for t in [25, 50, 75, 100, 125]:
        h.continuerun(t)
