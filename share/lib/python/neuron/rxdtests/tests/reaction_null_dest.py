from neuron import h, rxd

h.load_file("stdrun.hoc")

cell = h.Section(name="cell")
r = rxd.Region([cell])

Ca = rxd.Species(r, name="ca", charge=2, initial=60e-3)

reaction_decay = rxd.Reaction(Ca, 0 * Ca, 1e-8)
tvec = h.Vector().record(h._ref_t)
cavec = h.Vector().record(Ca.nodes[0]._ref_concentration)
h.finitialize(-70)
h.continuerun(10)

if __name__ == "__main__":
    from matplotlib import pyplot

    pyplot.plot(tvec, cavec)
    pyplot.show()
