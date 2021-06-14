from neuron import h, crxd as rxd
from math import pi

h.load_file("stdrun.hoc")

rxd.options.enable.extracellular = True
# create cell1 where `x` will be created and leak out
cell1 = h.Section(name="cell1")
cell1.pt3dclear()
cell1.pt3dadd(-20, 0, 0, 10)
cell1.pt3dadd(-10, 0, 0, 10)
cell1.nseg = 11
cell1.insert("pump")

# create cell2 where `x` will be pumped in and accumulate
cell2 = h.Section(name="cell2")
cell2.pt3dclear()
cell2.pt3dadd(10, 0, 0, 10)
cell2.pt3dadd(20, 0, 0, 10)
cell2.nseg = 11
cell2.insert("pump")

# Where?
# the intracellular spaces
cyt = rxd.Region(
    h.allsec(),
    name="cyt",
    nrn_region="i",
    geometry=rxd.FractionalVolume(0.9, surface_fraction=1.0),
)

org = rxd.Region(h.allsec(), name="org", geometry=rxd.FractionalVolume(0.1))

cyt_org_membrane = rxd.Region(
    h.allsec(), name="mem", geometry=rxd.ScalableBorder(pi / 2.0, on_cell_surface=False)
)


# the extracellular space
ecs = rxd.Extracellular(
    -55, -55, -55, 55, 55, 55, dx=10, volume_fraction=0.2, tortuosity=1.6
)

# Who?
x = rxd.Species([cyt, org, cyt_org_membrane, ecs], name="x", d=1.0, charge=1, initial=0)
Xcyt = x[cyt]
Xorg = x[org]


# What? - produce X in cell 1
# parameter to limit production to cell 1
cell1_param = rxd.Parameter(
    org, initial=lambda node: 1.0 if node.segment.sec == cell1 else 0
)

# production with a rate following Michaels Menton kinetics
createX = rxd.Rate(Xorg, cell1_param[org] * 1.0 / (10.0 + Xorg))

# leak between organelles and cytosol
cyt_org_leak = rxd.MultiCompartmentReaction(
    Xcyt, Xorg, 1e4, 1e4, membrane=cyt_org_membrane
)


# record the concentrations in the cells
t_vec = h.Vector()
t_vec.record(h._ref_t)
cell1_X = h.Vector()
cell1_X.record(cell1(0.5)._ref_xi)
cell1_Xorg = h.Vector()
cell1_Xorg.record(Xorg.nodes(cell1)(0.5)[0]._ref_concentration)
cell1V = h.Vector()
cell1V.record(cell1(0.5)._ref_v)


cell2_X = h.Vector()
cell2_X.record(cell2(0.5)._ref_xi)
cell2_Xorg = h.Vector()
cell2_Xorg.record(Xorg.nodes(cell2)(0.5)[0]._ref_concentration)
cell2V = h.Vector()
cell2V.record(cell2(0.5)._ref_v)


# run and plot the results
h.finitialize(-65)
h.continuerun(1000)

if __name__ == "__main__":
    from matplotlib import pyplot

    fig = pyplot.figure()
    pyplot.subplot(2, 2, 1)
    pyplot.plot(t_vec, cell1_X, label="cyt")
    pyplot.plot(t_vec, cell1_Xorg, label="org")
    pyplot.legend()
    pyplot.xlabel("t (ms)")
    pyplot.ylabel("cell 1 x (mM)")

    pyplot.subplot(2, 2, 2)
    pyplot.plot(t_vec, cell2_X, label="cyt")
    pyplot.plot(t_vec, cell2_Xorg, label="org")
    pyplot.xlabel("t (ms)")
    pyplot.ylabel("cell 2 x (mM)")

    pyplot.subplot(2, 2, 3)
    pyplot.plot(t_vec, cell1V, label="cell1")
    pyplot.xlabel("t (ms)")
    pyplot.ylabel("cell 1 V$_m$ (mV)")

    pyplot.subplot(2, 2, 4)
    pyplot.plot(t_vec, cell2V, label="cell2")
    pyplot.xlabel("t (ms)")
    pyplot.ylabel("cell 2 V$_m$ (mV)")

    # pyplot.subplot(3, 1, 2)
    # pyplot.imshow(x[ecs].states3d.mean(2).T, extent=x[ecs].extent('xy'), origin="lower", aspect='equal')
    fig.tight_layout()
    pyplot.show()
