from neuron import h, rxd
from neuron.rxd import v
from neuron.rxd.rxdmath import exp, log
import numpy

h.load_file("stdrun.hoc")

# set the number of rxd threads
rxd.nthread(2)

# set the seed
numpy.random.seed(63245)

# enable extracellular rxd (only necessary for NEURON prior to 7.7)
rxd.options.enable.extracellular = True

# Model parameters
e = 1.60217662e-19
scale = 1e-14 / e
h.celsius = 37

# Persistent Na current parameters
gnap = 0.4e-3 * scale
thmp = -40
thhp = -48
sigmp = 6
sighp = -6
nap_minf = 1.0 / (1.0 + exp(-(v - thmp) / sigmp))
nap_hinf = 1.0 / (1.0 + exp(-(v - thhp) / sighp))

# Fast Na current parameters
gna = 3e-3 * scale
thm = -34
sigm = 5

# K current parameters
gk = 5e-3 * scale
thn = -55.0
sgn = 14.0
taun0 = 0.05
taun1 = 0.27
thnt = -40
sn = -12
phin = 0.8
minf = 1.0 / (1.0 + exp(-(v - thm) / sigm))
ninf = 1.0 / (1.0 + exp(-(v - thn) / sgn))
taun = taun0 + taun1 / (1 + exp(-(v - thnt) / sn))

# K-leak parameters
gl = 0.1e-3 * scale
el = -70

# leak parameters
pas_gl = 0.3
pas_el = -70

# Simulation parameters
Lx, Ly, Lz = 500, 500, 100  # size of the extracellular space mu m^3
Ncell = 2  # number of neurons
somaR = 40

# Extracellular rxd
# Where? -- define the extracellular space
ecs = rxd.Extracellular(
    -Lx / 2.0,
    -Ly / 2.0,
    -Lz / 2.0,
    Lx / 2.0,
    Ly / 2.0,
    Lz / 2.0,
    dx=10,
    volume_fraction=0.2,
    tortuosity=1.6,
)

# Who? -- define the species
k = rxd.Species(
    ecs,
    name="k",
    d=2.62,
    charge=1,
    initial=lambda nd: 40 if nd.x3d**2 + nd.y3d**2 + nd.z3d**2 < 50**2 else 3.5,
)

na = rxd.Species(ecs, name="na", d=1.78, charge=1, initial=134)
ko, nao = k[ecs], na[ecs]

# What?
# No extracellular reactions, just diffusion.


class Neuron:
    """A neuron with soma and fast and persistent sodium
    currents, potassium currents, passive leak and potassium leak and an
    accumulation mechanism."""

    def __init__(self, x, y, z, rec=False):
        self.x = x
        self.y = y
        self.z = z

        self.soma = h.Section(name="soma", cell=self)
        # add 3D points to locate the neuron in the ECS
        self.soma.pt3dadd(x, y, z + somaR, 2.0 * somaR)
        self.soma.pt3dadd(x, y, z - somaR, 2.0 * somaR)

        # Where? -- define the intracellular space and membrane
        self.cyt = rxd.Region(self.soma, name="cyt", nrn_region="i")
        self.mem = rxd.Region(self.soma, name="mem", geometry=rxd.membrane())
        cell = [self.cyt, self.mem]

        # Who? -- the relevant ions and gates
        self.k = rxd.Species(cell, name="k", d=2.62, charge=1, initial=125)
        self.na = rxd.Species(cell, name="na", d=1.78, charge=1, initial=10)
        self.n = rxd.State(cell, name="n", initial=0.25512)
        self.ki, self.nai = self.k[self.cyt], self.na[self.cyt]

        # What? -- gating variables and ion currents
        self.n_gate = rxd.Rate(self.n, phin * (ninf - self.n) / taun)

        # Nernst potentials
        ena = 1e3 * h.R * (h.celsius + 273.15) * log(nao / self.nai) / h.FARADAY
        ek = 1e3 * h.R * (h.celsius + 273.15) * log(ko / self.ki) / h.FARADAY

        # Persistent Na current
        self.nap_current = rxd.MultiCompartmentReaction(
            self.nai,
            nao,
            gnap * nap_minf * nap_hinf * (v - ena),
            mass_action=False,
            membrane=self.mem,
            membrane_flux=True,
        )
        # Na current
        self.na_current = rxd.MultiCompartmentReaction(
            self.nai,
            nao,
            gna * minf**3 * (1.0 - self.n) * (v - ena),
            mass_action=False,
            membrane=self.mem,
            membrane_flux=True,
        )
        # K current
        self.k_current = rxd.MultiCompartmentReaction(
            self.ki,
            ko,
            gk * self.n**4 * (v - ek),
            mass_action=False,
            membrane=self.mem,
            membrane_flux=True,
        )
        # K leak
        self.k_leak = rxd.MultiCompartmentReaction(
            self.ki,
            ko,
            gl * (v - ek),
            mass_action=False,
            membrane=self.mem,
            membrane_flux=True,
        )
        # passive leak
        self.soma.insert("pas")
        self.soma(0.5).pas.g = pas_gl
        self.soma(0.5).pas.e = pas_el

        if rec:  # record membrane potential (shown in figure 1C)
            self.somaV = h.Vector()
            self.somaV.record(self.soma(0.5)._ref_v, rec)


# Randomly distribute the neurons
all_neurons = [
    Neuron(
        (numpy.random.random() * 2.0 - 1.0) * (Lx / 2.0),
        (numpy.random.random() * 2.0 - 1.0) * (Ly / 2.0),
        (numpy.random.random() * 2.0 - 1.0) * (Lz / 2.0),
        100,
    )
    for i in range(0, Ncell)
]


# initialize and set the intracellular concentrations
h.finitialize(-70)


h.dt = 1  # use a large time step as we are not focusing on spiking behaviour
# but on slower diffusion

h.continuerun(100)

if __name__ == "__main__":
    from matplotlib import pyplot

    pyplot.imshow(k[ecs].states3d.mean(2), extent=k[ecs].extent("xy"))
    for sec in h.allsec():
        pyplot.plot(
            [sec.x3d(i) for i in range(sec.n3d())],
            [sec.y3d(i) for i in range(sec.n3d())],
            "o",
        )

    pyplot.colorbar()
