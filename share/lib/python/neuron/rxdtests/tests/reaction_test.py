from neuron import h, crxd as rxd

h.load_file("stdrun.hoc")

cell = h.Section(name="cell")
r = rxd.Region([cell])


def Declare_Parameters(**kwargs):
    """helper function enabling clean declaration of parameters in top namespace"""
    for key, value in kwargs.items():
        globals()[key] = rxd.Parameter(r, name=key, initial=value)


def Declare_Species(**kwargs):
    """helper function enabling clean declaration of species in top namespace"""
    for key, value in kwargs.items():
        globals()[key] = rxd.Species(r, name=key, initial=value)


k3 = rxd.Parameter(r, name="k3", initial=1.2)
k4 = rxd.Parameter(r, name="k4", initial=0.6)

P2 = rxd.Species(r, name="P2", initial=0.0614368)
T2 = rxd.Species(r, name="T2", initial=0.0145428)
C = rxd.Species(r, name="C", initial=0.207614)

h.finitialize(-65)
h.continuerun(72)
