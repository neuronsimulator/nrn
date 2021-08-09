from neuron import h, rxd

h.load_file("stdrun.hoc")


class Cell:
    def __init__(self):
        self.soma = h.Section()


cell_1 = Cell()
cell_2 = Cell()
ecs = rxd.Extracellular(
    -55, -55, -55, 55, 55, 55, dx=10, volume_fraction=0.2, tortuosity=1.6
)

soma_cell_1 = rxd.Region([cell_1.soma], nrn_region="i", geometry=rxd.Shell(0.8, 1))
soma_cell_2 = rxd.Region([cell_2.soma], nrn_region="i", geometry=rxd.Shell(0.8, 1))
species_x = rxd.Species(
    [ecs, soma_cell_1, soma_cell_2],
    name="species_x",
    d=1,
    charge=1,
    initial=1,
    ecs_boundary_conditions=0,
)
Species_y = rxd.Species(
    [soma_cell_1, ecs],
    name="species_y",
    d=1,
    charge=1,
    initial=1,
    ecs_boundary_conditions=0,
)
Species_z = rxd.Species(
    [soma_cell_1], name="species_z", d=1, charge=1, initial=1, ecs_boundary_conditions=0
)
reaction_1 = rxd.Reaction(
    Species_y > Species_z + species_x, 0.058, regions=[soma_cell_1]
)

h.finitialize()
h.continuerun(10)
