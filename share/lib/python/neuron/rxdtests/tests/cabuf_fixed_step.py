from neuron import h, crxd as rxd, gui

# reaction rates
kf = 0.02
kb = 0.01
# initial concentrations
ca0 = 1
b0 = 0.5
cab0 = 0.001
# diffusion coefficients
Dca = 1
Db = 0
Dcab = 0

# create sections
soma = h.Section(name="soma")
dend = h.Section(name="dend")
soma.L = 100
soma.diam = 1
soma.nseg = 101
dend.L = 100
dend.diam = 1
dend.nseg = 101
soma.connect(dend)


r = rxd.Region(h.allsec())
Ca = rxd.Species(r, name="Ca", d=Dca)
Buf = rxd.Species(r, name="Buf", d=Db)
CaBuf = rxd.Species(r, name="CaBuf", d=Dcab)

buffering = rxd.Reaction(Ca[r] + Buf[r], CaBuf[r], kf, kb)


#
# set initial concentrations to ca0, b0, cab0 in soma,
# and to 0.001 in dend
#
Ca.initial = lambda node: (ca0 if node.sec == soma else 0.001)
Buf.initial = lambda node: (b0 if node.sec == soma else 0.001)
CaBuf.initial = lambda node: (cab0 if node.sec == soma else 0.001)


h.init()
species_0_trace = h.Vector()
species_0_trace.record(Ca.nodes(soma)(0.5)[0]._ref_concentration)

species_1_trace = h.Vector()
species_1_trace.record(Buf.nodes(soma)(0.5)[0]._ref_concentration)

species_2_trace = h.Vector()
species_2_trace.record(CaBuf.nodes(soma)(0.5)[0]._ref_concentration)


times = h.Vector()
times.record(h._ref_t)
#
# simulate 1 sec
#
h.tstop = 1000
h.run()

if __name__ == "__main__":
    from matplotlib import pyplot

    #
    # plot
    #
    labels = ["Ca", "Buf", "CaBuf"]
    for i, trace in enumerate([species_0_trace, species_1_trace, species_2_trace]):
        pyplot.plot(times.as_numpy(), trace.as_numpy(), label=labels[i])
    pyplot.legend()
    pyplot.title("Ca+Buf->CaBuf")
    pyplot.xlabel("Time (ms)")
    pyplot.ylabel("Concentration (mM)")
    pyplot.tight_layout()
    pyplot.show()
