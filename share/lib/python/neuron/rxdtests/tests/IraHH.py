# coding: utf-8

# In[1]:


from neuron import h, rxd
import neuron.units
from neuron.units import sec, mM, ms
from neuron.crxd import v
from neuron.crxd.rxdmath import exp, log
from math import pi

h.load_file("stdrun.hoc")


# In[2]:


dir(neuron.units)


# In[3]:


# parameters
h.celsius = 6.3
e = 1.60217662e-19
scale = 1e-14 / e
gnabar = 30e-3 * scale  # molecules/um2 ms mV
gnabar_l = 0.0247 * scale
gkbar = 25e-3 * scale
gkbar_l = 0.05e-3 * scale
gclbar_l = 0.1e-3 * scale
gl = 0.0003 * scale
ukcc2 = 0.3 * mM / sec
unkcc1 = 0.1 * mM / sec
p_max = 0.8 * mM / sec
alpha = 5.3
epsilon = 0.17 * sec * (10**-1)
g_gliamax = 5.0 * mM / sec
e_kmax = 0.25 * sec * (10**-1)
oa_bath = 32.0 / alpha
o_bath = 32.0 * mM
beta_o = 5
avo = rxd.constants.NA()
q10 = 3.0 ** ((h.celsius - 6.3) / 10.0)

# sodium activation 'm'
alpha_m = (0.32 * (v + 54.0)) / (1.0 - exp(-(v + 54.0) / 4.0))
beta_m = (0.28 * (v + 27.0)) / (exp((v + 27.0) / 5.0) - 1.0)
mtau = 1.0 / (q10 * (alpha_m + beta_m))
minf = alpha_m / (alpha_m + beta_m)

# sodium inactivation 'h'
alpha_h = 0.128 * exp(-(v + 50.0) / 18.0)
beta_h = 4.0 / (1.0 + exp(-(v + 27.0) / 5.0))
htau = 1.0 / (q10 * (alpha_h + beta_h))
hinf = alpha_h / (alpha_h + beta_h)

# potassium activation 'n'
alpha_n = (0.032 * (v + 52.0)) / (1.0 - exp(-(v + 52.0) / 5.0))
beta_n = 0.5 * exp(-(v + 57.0) / 40.0)
ntau = 1.0 / (q10 * (alpha_n + beta_n))
ninf = alpha_n / (alpha_n + beta_n)


# In[4]:


soma = h.Section("soma")
soma.pt3dclear()
soma.pt3dadd(-15, 0, 0, 14)
soma.pt3dadd(15, 0, 0, 14)
soma.nseg = 11

glia = h.Section("glia")
glia.pt3dclear()
glia.pt3dadd(-15, 0, 0, 30)
glia.pt3dadd(15, 0, 0, 30)
glia.nseg = 11

vi = 2.0 * pi * (soma.diam / 2.0) ** 2 * soma.L
vo = 100.0 * vi / beta_o

surface_area = 2 * pi * soma.diam / 2.0 * soma.L
volume = 2.0 * pi * (soma.diam / 2) ** 2 * soma.L
glia_surface_area = 2.0 * pi * glia.diam / 2.0 * glia.L
glia_volume = 2.0 * pi * (glia.diam / 2.0) ** 2 * glia.L
volume_scale = avo * volume / surface_area * 10**-18
glia_volume_scale = avo * glia_volume / glia_surface_area * 10**-18


# In[5]:


# initial cli: 6.6; initial clo: 119
# nai = 10 nao = 147
# ki = 125 ko = 2.9


# In[6]:


def concentration(i, o):
    return lambda nd: i if isinstance(nd, rxd.node.Node1D) else o


# In[7]:


# REGIONS------------------------------------------------------------------------------------------------------------------------

# intracellular/extracellular regions
cyt = rxd.Region(soma, name="cyt", nrn_region="i")
mem = rxd.Region(soma, name="cell_mem", geometry=rxd.membrane())
gcyt = rxd.Region(glia, name="cyt", nrn_region="i")
gmem = rxd.Region(glia, name="cell_mem", geometry=rxd.membrane())
dx = vo ** (1.0 / 3.0)
ecs = rxd.Extracellular(-2 * dx, -2 * dx, -2 * dx, 2 * dx, 2 * dx, 2 * dx, dx=dx)
# ecs = rxd.Extracellular(-100, -100, -100, 100, 100, 100, dx=33)
# SPECIES/PARAMETERS------------------------------------------------------------------------------------------------------------------------

# intracellular/extracellular species (Na+, K+, Cl-)
na = rxd.Species(
    [cyt, mem, ecs], name="na", d=1, charge=1, initial=concentration(15.5, 161.5)
)
k = rxd.Species(
    [cyt, mem, ecs], name="k", d=1, charge=1, initial=concentration(142.5, 7.8)
)
cl = rxd.Species(
    [cyt, mem, ecs], name="cl", d=1, charge=-1, initial=concentration(6.0, 130)
)
glial_na = rxd.Parameter(
    [gcyt, gmem], name="na", charge=1, value=18
)  # Na in book is 55 for astrocyte
glial_k = rxd.Parameter([gcyt, gmem], name="k", charge=1, value=80)
dump = rxd.Parameter([cyt, mem, gcyt, gmem], name="dump")
ki, ko, nai, nao, cli, clo, gnai, gki = (
    k[cyt],
    k[ecs],
    na[cyt],
    na[ecs],
    cl[cyt],
    cl[ecs],
    glial_na[gcyt],
    glial_k[gcyt],
)
# i = in cytosol (cyt) o = in extracellular (ecs)
# extracellular oxygen concentration
o2ecs = rxd.Species([ecs], name="o2ecs", initial=oa_bath)

# STATES-------------------------------------------------------------------------------------------------------------------------

# gating variables (m, h, n)
mgate = rxd.State([cyt, mem], name="mgate", initial=0.00787013592322398)
hgate = rxd.State([cyt, mem], name="hgate", initial=0.9981099795551048)
ngate = rxd.State([cyt, mem], name="ngate", initial=0.02284760152971809)

# ALL EQUATIONS------------------------------------------------------------------------------------------------------------------

gna = gnabar * mgate**3 * hgate
gk = gkbar * ngate**4
fko = 1.0 / (1.0 + exp(16.0 - ko))
nkcc1 = unkcc1 * fko * (log((ki * cli) / (ko * clo)) + log((nai * cli) / (nao * clo)))
kcc2 = ukcc2 * log((ki * cli) / (ko * clo))
ena = 26.64 * log(nao / nai)  # nerst equation - reversal potentials
ek = 26.64 * log(ko / ki)  # ^^^^^
ecl = 26.64 * log(cli / clo)  # ^^^
p = p_max / (1.0 + exp((20.0 - oa_bath * alpha) / 3))
pump = (p / (1.0 + exp((25.0 - nai) / 3))) * (1.0 / (1.0 + exp(3.5 - ko)))
gliapump = (
    (1.0 / 3.0) * (p / (1.0 + exp((25.0 - gnai) / 3.0))) * (1.0 / (1.0 + exp(3.5 - ko)))
)
g_glia = g_gliamax / (1.0 + exp(-oa_bath * alpha - 2.5) / 0.2)
glia12 = g_glia / 1.0 + exp((18.0 - ko) / 2.5)

# RATES--------------------------------------------------------------------------------------------------------------------------

# m_gate = rxd.Rate(mgate, (minf - mgate)/mtau)
# h_gate = rxd.Rate(hgate, (hinf - hgate)/htau)
# n_gate = rxd.Rate(ngate, (ninf - ngate)/ntau)

# dm/dt
m_gate = rxd.Rate(mgate, (alpha_m * (1.0 - mgate)) - (beta_m * mgate))

# dh/dt
h_gate = rxd.Rate(hgate, (alpha_h * (1.0 - hgate)) - (beta_h * hgate))

# dn/dt
n_gate = rxd.Rate(ngate, (alpha_n * (1.0 - ngate)) - (beta_n * ngate))

# d[O2]o/dt
o2_ecs = rxd.Rate(o2ecs, (epsilon * (oa_bath - o2ecs)))

# CURRENTS/LEAKS ----------------------------------------------------------------------------------------------------------------

# sodium (Na) current
na_current = rxd.MultiCompartmentReaction(
    nai, nao, gna * (v - ena), mass_action=False, membrane=mem, membrane_flux=True
)

# potassium (K) current
k_current = rxd.MultiCompartmentReaction(
    ki, ko, gk * (v - ek), mass_action=False, membrane=mem, membrane_flux=True
)

# chlorine (Cl) current
cl_current = rxd.MultiCompartmentReaction(
    cli, clo, gclbar_l * (v - ecl), mass_action=False, membrane=mem, membrane_flux=True
)

# nkcc1 (Na+/K+/2Cl- cotransporter)
nkcc1_current = rxd.MultiCompartmentReaction(
    2 * clo + ko + nao,
    2 * cli + ki + nai,
    nkcc1 * volume_scale,
    mass_action=False,
    membrane=mem,
    membrane_flux=True,
)

# kcc2 (K+/Cl- cotransporter)
kcc2_current = rxd.MultiCompartmentReaction(
    clo + ko,
    cli + ki,
    kcc2 * volume_scale,
    membrane=mem,
    mass_action=False,
    membrane_flux=True,
)

# sodium leak
na_leak = rxd.MultiCompartmentReaction(
    nai, nao, gnabar_l * (v - ena), mass_action=False, membrane=mem, membrane_flux=True
)

# potassium leak
k_leak = rxd.MultiCompartmentReaction(
    ki, ko, gkbar_l * (v - ek), mass_action=False, membrane=mem, membrane_flux=True
)

# Na+/K+ pump current in neuron (2K+ in, 3Na+ out)
pump_current = rxd.MultiCompartmentReaction(
    ki,
    ko,
    -2.0 * pump * volume_scale,
    mass_action=False,
    membrane=mem,
    membrane_flux=True,
)
pump_current_other_half = rxd.MultiCompartmentReaction(
    nai,
    nao,
    3.0 * pump * volume_scale,
    mass_action=False,
    membrane=mem,
    membrane_flux=True,
)
oxygen = rxd.MultiCompartmentReaction(
    o2ecs[ecs], dump[cyt], pump * volume_scale, mass_action=False, membrane=mem
)
goxygen = rxd.MultiCompartmentReaction(
    o2ecs[ecs],
    dump[gcyt],
    gliapump * glia_volume_scale,
    mass_action=False,
    membrane=gmem,
)

# Na+/K+ pump current in glia (2K+ in, 3Na+ out)
gliapump_current = rxd.MultiCompartmentReaction(
    2 * gki,
    2 * ko,
    -gliapump * glia_volume_scale,
    mass_action=False,
    membrane=gmem,
    membrane_flux=True,
)
gliapump_current_other_half = rxd.MultiCompartmentReaction(
    3 * gnai,
    3 * nao,
    gliapump * glia_volume_scale,
    mass_action=False,
    membrane=gmem,
    membrane_flux=True,
)


# In[8]:


stimA = h.IClamp(soma(0.5))
stimA.delay = 20
stimA.amp = 1
stimA.dur = 0.2

t = h.Vector().record(h._ref_t)
soma_v = h.Vector().record(soma(0.5)._ref_v)
soma_nai = h.Vector().record(soma(0.5)._ref_nai)
soma_ki = h.Vector().record(soma(0.5)._ref_ki)
soma_cli = h.Vector().record(soma(0.5)._ref_cli)

soma_nao = h.Vector().record(soma(0.5)._ref_nao)
soma_ko = h.Vector().record(soma(0.5)._ref_ko)
soma_clo = h.Vector().record(soma(0.5)._ref_clo)


h.CVode().active(True)
h.finitialize(-70)
# h.dt = 0.025
# h.continuerun(100)
# h.dt = 1
h.continuerun(100)
