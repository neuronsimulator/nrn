from neuron import h, rxd

h.load_file("stdrun.hoc")

from neuron.units import mM, ms

sim_time = 100

axon_terminal = h.Section(name="axon_terminal")
# synaptic_cleft = rxd.Region([axon_terminal], nrn_region='o')
axon_terminal_region = rxd.Region([axon_terminal], nrn_region="i")

ca_intra = rxd.Species(
    axon_terminal_region, name="ca", charge=2, d=0, initial=5e-4, atolscale=1e-6
)
F = rxd.Species(axon_terminal_region, name="F", initial=0.001)
FA = rxd.Species(axon_terminal_region, name="FA", initial=0)

kb = rxd.Parameter(axon_terminal_region, name="kb", value=1e16 / (mM**4 * ms))
ku = rxd.Parameter(axon_terminal_region, name="ku", value=0.1 / ms)


myreaction = rxd.Reaction(
    F + 4 * ca_intra, FA, kb, ku
)  # , regions=axon_terminal_region)

tvec = h.Vector().record(h._ref_t)
Fvec = h.Vector().record(F.nodes._ref_concentration)
FAvec = h.Vector().record(FA.nodes._ref_concentration)
h.finitialize(-65)
h.continuerun(10)

if __name__ == "__main__":
    from matplotlib import pyplot

    pyplot.plot(tvec, Fvec)
    pyplot.plot(tvec, FAvec)
    pyplot.show()
