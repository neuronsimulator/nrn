from neuron import h, rxd
from math import pi

h.load_file("stdrun.hoc")

sec = h.Section()
# the extracellular space
ecs = rxd.Extracellular(
    -55, -55, -55, 55, 55, 55, dx=10, volume_fraction=0.2, tortuosity=1.6
)

# Who?
x = rxd.Species([ecs], name="x", d=0.0, charge=1, initial=0)


def callbackfun():
    return 1000


x[ecs].node_by_location(-20, 0, 0).include_flux(1000)

x[ecs].node_by_location(0, 0, 0).include_flux(callbackfun)

x[ecs].node_by_location(20, 0, 0).include_flux(sec(0.5)._ref_v)

h.finitialize(1000)

h.continuerun(10)

print("float", x[ecs].node_by_location(-20, 0, 0).concentration)
print("callb", x[ecs].node_by_location(0, 0, 0).concentration)
print("refer", x[ecs].node_by_location(20, 0, 0).concentration)
